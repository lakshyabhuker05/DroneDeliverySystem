#include "ReportManager.h"
#include "Sorting.h"
#include "Searching.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <ctime>

std::vector<Delivery> ReportManager::generate(ReportRange range) const {
    std::vector<Delivery> all = deliveryManager->allDeliveries();

    // MERGE SORT: order by creation time, most recent first.
    dsa::mergeSort<Delivery>(all, [](const Delivery& a, const Delivery& b) {
        return a.createdAt > b.createdAt;
    });

    time_t now = std::time(nullptr);
    double windowSeconds = 24.0 * 3600;
    switch (range) {
        case ReportRange::DAILY:   windowSeconds = 24.0 * 3600;      break;
        case ReportRange::WEEKLY:  windowSeconds = 7 * 24.0 * 3600;  break;
        case ReportRange::MONTHLY: windowSeconds = 30 * 24.0 * 3600; break;
    }

    std::vector<Delivery> filtered;
    for (const auto& d : all) {
        if (difftime(now, d.createdAt) <= windowSeconds) filtered.push_back(d);
    }
    return filtered;
}

bool ReportManager::findDeliveryFast(const std::vector<Delivery>& sortedById, int id, Delivery& out) const {
    // BINARY SEARCH requires the input to already be sorted ascending by ID.
    std::function<int(const Delivery&)> keyOf = [](const Delivery& d) { return d.id; };
    int idx = dsa::binarySearch<Delivery, int>(sortedById, id, keyOf);
    if (idx == -1) return false;
    out = sortedById[idx];
    return true;
}

std::string ReportManager::toCSV(const std::vector<Delivery>& rows) const {
    std::ostringstream os;
    os << "ID,CustomerID,Package,Weight(kg),Pickup,Destination,DroneID,Status,ETA(min),Distance(km),Priority,CreatedAt\n";
    for (const auto& d : rows) {
        os << d.id << "," << d.customerId << "," << d.packageName << "," << d.weightKg << ","
           << d.pickupCity << "," << d.destinationCity << "," << d.droneId << ","
           << deliveryStatusToString(d.status) << "," << d.estimatedMinutes << ","
           << d.distanceKm << "," << d.priority << "," << utils::formatTime(d.createdAt) << "\n";
    }
    return os.str();
}

std::string ReportManager::toJSON(const std::vector<Delivery>& rows) const {
    std::ostringstream os;
    os << "[";
    for (size_t i = 0; i < rows.size(); ++i) {
        os << rows[i].toJSON();
        if (i + 1 < rows.size()) os << ",";
    }
    os << "]";
    return os.str();
}

bool ReportManager::exportToFile(ReportRange range, const std::string& path) const {
    std::vector<Delivery> rows = generate(range);
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) return false;
    out << toCSV(rows);
    return true;
}

ReportManager::DashboardStats ReportManager::dashboardStats() const {
    DashboardStats stats{};
    std::vector<Drone> drones = droneManager->allDronesSortedById();
    stats.totalDrones = static_cast<int>(drones.size());
    stats.activeDrones = static_cast<int>(droneManager->activeDrones());
    double batterySum = 0;
    int availableCount = 0;
    for (const auto& d : drones) {
        batterySum += d.batteryPercent;
        if (d.status == DroneStatus::AVAILABLE) availableCount++;
    }
    stats.avgBattery = drones.empty() ? 0 : batterySum / drones.size();
    stats.availableDrones = availableCount;

    stats.totalDeliveries = static_cast<int>(deliveryManager->totalCount());
    stats.pendingDeliveries = static_cast<int>(deliveryManager->pendingCount());
    stats.completedDeliveries = static_cast<int>(deliveryManager->completedCount());
    return stats;
}
