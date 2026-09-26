#include "DeliveryManager.h"
#include "Utils.h"
#include <fstream>
#include <iostream>

DeliveryManager::DeliveryManager(Graph* graph, DroneManager* droneMgr, const std::string& dataFile_)
    : routeGraph(graph), droneManager(droneMgr), nextId(1), dataFile(dataFile_) {
    packageCatalog = {"Documents", "Electronics", "Medicines", "Groceries", "Clothing", "Spare Parts"};
    load();
}

void DeliveryManager::load() {
    std::ifstream in(dataFile);
    if (!in.is_open()) return;
    std::string line;
    int maxId = 0;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto t = utils::split(line, '|');
        if (t.size() < 12) continue;
        Delivery d;
        d.id = std::stoi(t[0]);
        d.customerId = std::stoi(t[1]);
        d.packageName = t[2];
        d.weightKg = std::stod(t[3]);
        d.pickupCity = t[4];
        d.destinationCity = t[5];
        d.droneId = std::stoi(t[6]);
        d.status = deliveryStatusFromString(t[7]);
        d.estimatedMinutes = std::stod(t[8]);
        d.distanceKm = std::stod(t[9]);
        d.priority = std::stoi(t[10]);
        d.createdAt = static_cast<time_t>(std::stoll(t[11]));

        deliveries.put(d.id, d);
        addToHistory(d.customerId, d.id);
        if (d.status == DeliveryStatus::PENDING) {
            if (d.priority >= 2) urgentQueue.push(d);
            else pendingQueue.enqueue(d.id);
        }
        if (d.id > maxId) maxId = d.id;
    }
    nextId = maxId + 1;
}

void DeliveryManager::persist() const {
    std::ofstream out(dataFile, std::ios::trunc);
    if (!out.is_open()) return;
    for (const auto& kv : deliveries.items()) out << kv.second.toRow() << "\n";
}

void DeliveryManager::addToHistory(int customerId, int deliveryId) {
    LinkedList<int>* list;
    if (!historyByCustomer.get(customerId, list)) {
        list = new LinkedList<int>();
        historyByCustomer.put(customerId, list);
    }
    list->append(deliveryId);
}

int DeliveryManager::bookDelivery(int customerId, const std::string& packageName, double weightKg,
                                    const std::string& pickup, const std::string& destination, int priority) {
    if (weightKg <= 0 || weightKg > 50) return -1;      // basic input validation
    if (!routeGraph->hasCity(pickup) || !routeGraph->hasCity(destination)) return -1;
    if (pickup == destination) return -1;

    Delivery d(nextId, customerId, packageName, weightKg, pickup, destination, priority);

    // Pre-compute route distance immediately so customers see an ETA estimate
    // even before a drone is formally assigned.
    Graph::RouteResult route = routeGraph->shortestRoute(pickup, destination);
    if (route.found) d.distanceKm = route.distanceKm;

    deliveries.put(d.id, d);
    addToHistory(customerId, d.id);

    if (priority >= 2) urgentQueue.push(d);
    else pendingQueue.enqueue(d.id);

    UndoAction action{UndoAction::Type::BOOK, d.id, Delivery()}; // "previous state" = didn't exist
    undoStack.push(action);

    int id = d.id;
    nextId++;
    persist();
    return id;
}

bool DeliveryManager::cancelDelivery(int deliveryId, int requestingCustomerId, bool isAdmin) {
    Delivery d;
    if (!deliveries.get(deliveryId, d)) return false;
    if (!isAdmin && d.customerId != requestingCustomerId) return false;
    if (d.status == DeliveryStatus::DELIVERED || d.status == DeliveryStatus::CANCELLED) return false;

    UndoAction action{UndoAction::Type::CANCEL, deliveryId, d}; // save previous state
    undoStack.push(action);

    if (d.status == DeliveryStatus::ASSIGNED && d.droneId != -1) {
        droneManager->setStatus(d.droneId, DroneStatus::AVAILABLE);
    }
    d.status = DeliveryStatus::CANCELLED;
    deliveries.put(deliveryId, d);
    persist();
    return true;
}

bool DeliveryManager::assignNextDelivery() {
    Delivery d;
    bool got = false;

    // Urgent requests are always drained first.
    if (!urgentQueue.isEmpty()) {
        got = urgentQueue.pop(d);
    } else {
        int id;
        if (pendingQueue.dequeue(id)) {
            if (deliveries.get(id, d)) got = true;
        }
    }
    if (!got) return false;

    // re-fetch latest state (could have been cancelled in the meantime)
    Delivery latest;
    if (!deliveries.get(d.id, latest) || latest.status != DeliveryStatus::PENDING) {
        return assignNextDelivery(); // skip stale/cancelled entries, try next
    }

    int droneId = droneManager->findBestDroneForPickup(latest.pickupCity, latest.weightKg);
    if (droneId == -1) {
        // no drone available right now -- push back to the front of its queue
        if (latest.priority >= 2) urgentQueue.push(latest);
        else pendingQueue.enqueue(latest.id);
        return false;
    }

    return assignSpecific(latest.id, droneId);
}

bool DeliveryManager::assignSpecific(int deliveryId, int droneId) {
    Delivery d;
    if (!deliveries.get(deliveryId, d)) return false;
    Drone drone;
    if (!droneManager->getDrone(droneId, drone)) return false;

    UndoAction action{UndoAction::Type::ASSIGN, deliveryId, d};
    undoStack.push(action);

    Graph::RouteResult route = routeGraph->shortestRoute(d.pickupCity, d.destinationCity);
    if (route.found) {
        d.distanceKm = route.distanceKm;
        d.estimatedMinutes = (route.distanceKm / drone.speedKmph) * 60.0;
    }
    d.droneId = droneId;
    d.status = DeliveryStatus::ASSIGNED;
    deliveries.put(deliveryId, d);

    droneManager->setStatus(droneId, DroneStatus::ASSIGNED);
    persist();
    return true;
}

bool DeliveryManager::markInTransit(int deliveryId) {
    Delivery d;
    if (!deliveries.get(deliveryId, d)) return false;
    if (d.status != DeliveryStatus::ASSIGNED) return false;
    d.status = DeliveryStatus::IN_TRANSIT;
    deliveries.put(deliveryId, d);
    persist();
    return true;
}

bool DeliveryManager::markDelivered(int deliveryId) {
    Delivery d;
    if (!deliveries.get(deliveryId, d)) return false;
    if (d.status != DeliveryStatus::IN_TRANSIT && d.status != DeliveryStatus::ASSIGNED) return false;
    d.status = DeliveryStatus::DELIVERED;
    if (d.droneId != -1) {
        droneManager->setStatus(d.droneId, DroneStatus::AVAILABLE);
    }
    deliveries.put(deliveryId, d);
    persist();
    return true;
}

bool DeliveryManager::undoLastAction() {
    UndoAction action;
    if (!undoStack.pop(action)) return false;

    if (action.type == UndoAction::Type::BOOK) {
        // undo a booking = remove it (soft: mark cancelled) - it never truly existed
        Delivery d;
        if (deliveries.get(action.deliveryId, d)) {
            d.status = DeliveryStatus::CANCELLED;
            deliveries.put(action.deliveryId, d);
        }
    } else {
        // undo a CANCEL or an ASSIGN -> restore the previous saved state
        Delivery prev = action.previousState;
        if (action.type == UndoAction::Type::ASSIGN && prev.droneId != -1) {
            // releasing the drone that this assignment had taken over
            Delivery current;
            if (deliveries.get(action.deliveryId, current) && current.droneId != -1) {
                droneManager->setStatus(current.droneId, DroneStatus::AVAILABLE);
            }
        }
        deliveries.put(action.deliveryId, prev);
        if (prev.status == DeliveryStatus::ASSIGNED && prev.droneId != -1) {
            droneManager->setStatus(prev.droneId, DroneStatus::ASSIGNED);
        }
    }
    persist();
    return true;
}

bool DeliveryManager::getDelivery(int id, Delivery& out) const {
    return deliveries.get(id, out);
}

std::vector<Delivery> DeliveryManager::historyForCustomer(int customerId) const {
    LinkedList<int>* list;
    std::vector<Delivery> out;
    if (!historyByCustomer.get(customerId, list)) return out;
    for (int id : list->toVectorMostRecentFirst()) {
        Delivery d;
        if (deliveries.get(id, d)) out.push_back(d);
    }
    return out;
}

std::vector<Delivery> DeliveryManager::allDeliveries() const {
    std::vector<Delivery> out;
    for (const auto& kv : deliveries.items()) out.push_back(kv.second);
    return out;
}

size_t DeliveryManager::pendingCount() const {
    size_t c = 0;
    for (const auto& kv : deliveries.items())
        if (kv.second.status == DeliveryStatus::PENDING || kv.second.status == DeliveryStatus::ASSIGNED
            || kv.second.status == DeliveryStatus::IN_TRANSIT) c++;
    return c;
}

size_t DeliveryManager::completedCount() const {
    size_t c = 0;
    for (const auto& kv : deliveries.items())
        if (kv.second.status == DeliveryStatus::DELIVERED) c++;
    return c;
}
