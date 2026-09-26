#ifndef DRONE_H
#define DRONE_H

#include <string>
#include <sstream>

enum class DroneStatus { AVAILABLE, ASSIGNED, CHARGING, MAINTENANCE, OFFLINE };

inline std::string droneStatusToString(DroneStatus s) {
    switch (s) {
        case DroneStatus::AVAILABLE:   return "AVAILABLE";
        case DroneStatus::ASSIGNED:    return "ASSIGNED";
        case DroneStatus::CHARGING:    return "CHARGING";
        case DroneStatus::MAINTENANCE: return "MAINTENANCE";
        case DroneStatus::OFFLINE:     return "OFFLINE";
    }
    return "UNKNOWN";
}

inline DroneStatus droneStatusFromString(const std::string& s) {
    if (s == "AVAILABLE") return DroneStatus::AVAILABLE;
    if (s == "ASSIGNED") return DroneStatus::ASSIGNED;
    if (s == "CHARGING") return DroneStatus::CHARGING;
    if (s == "MAINTENANCE") return DroneStatus::MAINTENANCE;
    return DroneStatus::OFFLINE;
}

class Drone {
public:
    int id;
    std::string model;
    DroneStatus status;
    double batteryPercent;   // 0 - 100
    double speedKmph;        // km/h
    double maxWeightKg;      // kg
    std::string currentLocation; // city name (must match a Graph node)
    bool availability;

    Drone() : id(0), status(DroneStatus::OFFLINE), batteryPercent(100),
               speedKmph(40), maxWeightKg(5), currentLocation("Hub"), availability(true) {}

    Drone(int id_, std::string model_, double battery_, double speed_,
          double maxWeight_, std::string location_)
        : id(id_), model(std::move(model_)), status(DroneStatus::AVAILABLE),
          batteryPercent(battery_), speedKmph(speed_), maxWeightKg(maxWeight_),
          currentLocation(std::move(location_)), availability(true) {}

    bool canCarry(double weightKg) const {
        return availability && status == DroneStatus::AVAILABLE &&
               weightKg <= maxWeightKg && batteryPercent >= 15.0;
    }

    std::string toJSON() const {
        std::ostringstream os;
        os << "{"
           << "\"id\":" << id << ","
           << "\"model\":\"" << model << "\","
           << "\"status\":\"" << droneStatusToString(status) << "\","
           << "\"battery\":" << batteryPercent << ","
           << "\"speed\":" << speedKmph << ","
           << "\"maxWeight\":" << maxWeightKg << ","
           << "\"location\":\"" << currentLocation << "\","
           << "\"available\":" << (availability ? "true" : "false")
           << "}";
        return os.str();
    }

    // pipe-delimited row for flat-file persistence
    std::string toRow() const {
        std::ostringstream os;
        os << id << "|" << model << "|" << droneStatusToString(status) << "|"
           << batteryPercent << "|" << speedKmph << "|" << maxWeightKg << "|"
           << currentLocation << "|" << (availability ? 1 : 0);
        return os.str();
    }
};

#endif // DRONE_H
