#ifndef REPORT_MANAGER_H
#define REPORT_MANAGER_H

#include "Delivery.h"
#include "DeliveryManager.h"
#include "DroneManager.h"
#include <string>
#include <vector>

enum class ReportRange { DAILY, WEEKLY, MONTHLY };

class ReportManager {
private:
    DeliveryManager* deliveryManager;
    DroneManager* droneManager;

public:
    ReportManager(DeliveryManager* dm, DroneManager* drm) : deliveryManager(dm), droneManager(drm) {}

    // Sorts (merge sort, stable, by createdAt desc) then filters by range.
    std::vector<Delivery> generate(ReportRange range) const;

    // Sorts all deliveries by ID (quick sort) then binary-searches for one.
    bool findDeliveryFast(const std::vector<Delivery>& sortedById, int id, Delivery& out) const;

    std::string toCSV(const std::vector<Delivery>& rows) const;
    std::string toJSON(const std::vector<Delivery>& rows) const;

    bool exportToFile(ReportRange range, const std::string& path) const;

    // Aggregate dashboard statistics (used by both console dashboard and the
    // frontend /api/dashboard endpoint).
    struct DashboardStats {
        int totalDeliveries;
        int activeDrones;
        int totalDrones;
        double avgBattery;
        int pendingDeliveries;
        int completedDeliveries;
        int availableDrones;
    };
    DashboardStats dashboardStats() const;
};

#endif // REPORT_MANAGER_H
