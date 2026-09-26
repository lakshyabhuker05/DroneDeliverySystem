#include "DroneManager.h"
#include "Utils.h"
#include <fstream>
#include <iostream>

DroneManager::DroneManager(const std::string& dataFile_) : nextId(1), dataFile(dataFile_) {
    load();
}

void DroneManager::load() {
    std::ifstream in(dataFile);
    if (!in.is_open()) return;
    std::string line;
    int maxId = 0;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto tokens = utils::split(line, '|');
        if (tokens.size() < 8) continue;
        Drone d;
        d.id = std::stoi(tokens[0]);
        d.model = tokens[1];
        d.status = droneStatusFromString(tokens[2]);
        d.batteryPercent = std::stod(tokens[3]);
        d.speedKmph = std::stod(tokens[4]);
        d.maxWeightKg = std::stod(tokens[5]);
        d.currentLocation = tokens[6];
        d.availability = tokens[7] == "1";
        tree.insert(d.id, d);
        index.put(d.id, d);
        if (d.id > maxId) maxId = d.id;
    }
    nextId = maxId + 1;
}

void DroneManager::persist() const {
    std::ofstream out(dataFile, std::ios::trunc);
    if (!out.is_open()) return;
    for (const auto& kv : tree.inorder()) out << kv.second.toRow() << "\n";
}

int DroneManager::addDrone(const std::string& model, double battery, double speed,
                            double maxWeight, const std::string& location) {
    Drone d(nextId, model, battery, speed, maxWeight, location);
    tree.insert(d.id, d);
    index.put(d.id, d);
    int id = nextId;
    nextId++;
    persist();
    return id;
}

bool DroneManager::removeDrone(int id) {
    bool removed = tree.remove(id);
    if (removed) { index.remove(id); persist(); }
    return removed;
}

bool DroneManager::updateDrone(int id, const Drone& updated) {
    Drone existing;
    if (!tree.find(id, existing)) return false;
    tree.insert(id, updated);
    index.put(id, updated);
    persist();
    return true;
}

bool DroneManager::getDrone(int id, Drone& out) const {
    return index.get(id, out);
}

bool DroneManager::setStatus(int id, DroneStatus status) {
    Drone d;
    if (!index.get(id, d)) return false;
    d.status = status;
    d.availability = (status == DroneStatus::AVAILABLE);
    tree.insert(id, d);
    index.put(id, d);
    persist();
    return true;
}

bool DroneManager::setBattery(int id, double battery) {
    Drone d;
    if (!index.get(id, d)) return false;
    d.batteryPercent = battery;
    tree.insert(id, d);
    index.put(id, d);
    persist();
    return true;
}

std::vector<Drone> DroneManager::allDronesSortedById() const {
    std::vector<Drone> out;
    for (const auto& kv : tree.inorder()) out.push_back(kv.second);
    return out;
}

std::vector<Drone> DroneManager::availableDrones(double minWeight) const {
    std::vector<Drone> out;
    for (const auto& kv : tree.inorder()) {
        if (kv.second.canCarry(minWeight)) out.push_back(kv.second);
    }
    return out;
}

int DroneManager::findBestDroneForPickup(const std::string& pickupCity, double weightKg) const {
    int bestId = -1;
    double bestBattery = -1;
    for (const auto& kv : tree.inorder()) {
        const Drone& d = kv.second;
        if (!d.canCarry(weightKg)) continue;
        // Prefer a drone already stationed at the pickup city; among ties /
        // otherwise prefer the one with the most battery remaining.
        bool atPickup = (d.currentLocation == pickupCity);
        double score = d.batteryPercent + (atPickup ? 1000.0 : 0.0);
        if (score > bestBattery) { bestBattery = score; bestId = d.id; }
    }
    return bestId;
}

size_t DroneManager::activeDrones() const {
    size_t c = 0;
    for (const auto& kv : tree.inorder())
        if (kv.second.status != DroneStatus::OFFLINE && kv.second.status != DroneStatus::MAINTENANCE) c++;
    return c;
}
