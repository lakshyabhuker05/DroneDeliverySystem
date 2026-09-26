#ifndef DELIVERY_MANAGER_H
#define DELIVERY_MANAGER_H

#include "Delivery.h"
#include "DataStructures.h"
#include "Graph.h"
#include "DroneManager.h"
#include <vector>
#include <string>

// Comparator for the urgent-delivery priority queue: higher `priority` value
// AND older `createdAt` (aging, to avoid starvation) is served first.
struct DeliveryPriorityLess {
    bool operator()(const Delivery& a, const Delivery& b) const {
        if (a.priority != b.priority) return a.priority < b.priority; // higher priority wins
        return a.createdAt > b.createdAt; // earlier request wins on tie
    }
};

// Undo-able action record, pushed onto the undo stack on every mutating op.
struct UndoAction {
    enum class Type { CANCEL, ASSIGN, BOOK } type;
    int deliveryId;
    Delivery previousState; // state to restore to
};

class DeliveryManager {
private:
    HashMap<int, Delivery> deliveries;              // O(1) lookup by ID
    LinkedQueue<int> pendingQueue;                  // normal FIFO requests (delivery IDs)
    PriorityQueue<Delivery, DeliveryPriorityLess> urgentQueue; // urgent requests
    HashMap<int, LinkedList<int>*> historyByCustomer; // customerId -> linked list of delivery IDs
    LinkedStack<UndoAction> undoStack;               // undo support
    std::vector<std::string> packageCatalog;         // Vector -> simple package/category storage

    Graph* routeGraph;
    DroneManager* droneManager;

    int nextId;
    std::string dataFile;

    void persist() const;
    void load();
    void addToHistory(int customerId, int deliveryId);

public:
    DeliveryManager(Graph* graph, DroneManager* droneMgr, const std::string& dataFile_ = "data/deliveries.txt");
    ~DeliveryManager() { persist(); }

    // priority: 1 = normal (goes to FIFO pendingQueue), >=2 = urgent (priority queue)
    int bookDelivery(int customerId, const std::string& packageName, double weightKg,
                      const std::string& pickup, const std::string& destination, int priority);

    bool cancelDelivery(int deliveryId, int requestingCustomerId, bool isAdmin);

    // Pops the next request (urgent queue drained first, then FIFO queue),
    // computes the shortest route via Dijkstra, finds & assigns the best
    // drone, and updates status -> ASSIGNED.
    bool assignNextDelivery();

    // Directly assign a specific delivery to a specific drone (admin override).
    bool assignSpecific(int deliveryId, int droneId);

    bool markInTransit(int deliveryId);
    bool markDelivered(int deliveryId);

    bool undoLastAction(); // pops undoStack and restores previous state

    bool getDelivery(int id, Delivery& out) const;
    std::vector<Delivery> historyForCustomer(int customerId) const; // most-recent-first
    std::vector<Delivery> allDeliveries() const;

    size_t pendingCount() const;
    size_t completedCount() const;
    size_t totalCount() const { return deliveries.size(); }

    void addPackageType(const std::string& name) { packageCatalog.push_back(name); }
    const std::vector<std::string>& packageTypes() const { return packageCatalog; }
};

#endif // DELIVERY_MANAGER_H
