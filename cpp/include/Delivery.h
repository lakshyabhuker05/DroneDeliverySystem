#ifndef DELIVERY_H
#define DELIVERY_H

#include <string>
#include <sstream>
#include <ctime>

enum class DeliveryStatus { PENDING, ASSIGNED, IN_TRANSIT, DELIVERED, CANCELLED };

inline std::string deliveryStatusToString(DeliveryStatus s) {
    switch (s) {
        case DeliveryStatus::PENDING:    return "PENDING";
        case DeliveryStatus::ASSIGNED:   return "ASSIGNED";
        case DeliveryStatus::IN_TRANSIT: return "IN_TRANSIT";
        case DeliveryStatus::DELIVERED:  return "DELIVERED";
        case DeliveryStatus::CANCELLED:  return "CANCELLED";
    }
    return "UNKNOWN";
}

inline DeliveryStatus deliveryStatusFromString(const std::string& s) {
    if (s == "PENDING") return DeliveryStatus::PENDING;
    if (s == "ASSIGNED") return DeliveryStatus::ASSIGNED;
    if (s == "IN_TRANSIT") return DeliveryStatus::IN_TRANSIT;
    if (s == "DELIVERED") return DeliveryStatus::DELIVERED;
    return DeliveryStatus::CANCELLED;
}

class Delivery {
public:
    int id;
    int customerId;
    std::string packageName;
    double weightKg;
    std::string pickupCity;
    std::string destinationCity;
    int droneId;              // -1 if not yet assigned
    DeliveryStatus status;
    double estimatedMinutes;  // computed via Dijkstra distance / drone speed
    double distanceKm;
    int priority;             // 1 = normal, higher = more urgent
    time_t createdAt;

    Delivery()
        : id(0), customerId(0), weightKg(0), droneId(-1),
          status(DeliveryStatus::PENDING), estimatedMinutes(0), distanceKm(0),
          priority(1), createdAt(std::time(nullptr)) {}

    Delivery(int id_, int customerId_, std::string packageName_, double weight_,
              std::string pickup_, std::string destination_, int priority_ = 1)
        : id(id_), customerId(customerId_), packageName(std::move(packageName_)),
          weightKg(weight_), pickupCity(std::move(pickup_)),
          destinationCity(std::move(destination_)), droneId(-1),
          status(DeliveryStatus::PENDING), estimatedMinutes(0), distanceKm(0),
          priority(priority_), createdAt(std::time(nullptr)) {}

    std::string toJSON() const {
        std::ostringstream os;
        os << "{"
           << "\"id\":" << id << ","
           << "\"customerId\":" << customerId << ","
           << "\"package\":\"" << packageName << "\","
           << "\"weight\":" << weightKg << ","
           << "\"pickup\":\"" << pickupCity << "\","
           << "\"destination\":\"" << destinationCity << "\","
           << "\"droneId\":" << droneId << ","
           << "\"status\":\"" << deliveryStatusToString(status) << "\","
           << "\"eta\":" << estimatedMinutes << ","
           << "\"distance\":" << distanceKm << ","
           << "\"priority\":" << priority << ","
           << "\"createdAt\":" << createdAt
           << "}";
        return os.str();
    }

    std::string toRow() const {
        std::ostringstream os;
        os << id << "|" << customerId << "|" << packageName << "|" << weightKg << "|"
           << pickupCity << "|" << destinationCity << "|" << droneId << "|"
           << deliveryStatusToString(status) << "|" << estimatedMinutes << "|"
           << distanceKm << "|" << priority << "|" << createdAt;
        return os.str();
    }
};

#endif // DELIVERY_H
