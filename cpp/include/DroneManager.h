#ifndef DRONE_MANAGER_H
#define DRONE_MANAGER_H

#include "Drone.h"
#include "DataStructures.h"
#include <vector>
#include <string>

/*******************************************************************************
 * DroneManager
 * Storage strategy:
 *   - BinarySearchTree<int, Drone>  -> canonical storage, keyed by Drone ID.
 *        Gives sorted (in-order) listing for admin dashboard / reports and
 *        O(log n) insert/search/delete.
 *   - HashMap<int, Drone>           -> mirror index for O(1) average lookup
 *        by ID (used heavily by DeliveryManager when assigning drones).
 * Both are kept in sync on every mutation.
 ******************************************************************************/
class DroneManager {
private:
    BinarySearchTree<int, Drone> tree;
    HashMap<int, Drone> index;
    int nextId;
    std::string dataFile;

    void persist() const;
    void load();

public:
    explicit DroneManager(const std::string& dataFile_ = "data/drones.txt");
    ~DroneManager() { persist(); }

    int addDrone(const std::string& model, double battery, double speed,
                 double maxWeight, const std::string& location);
    bool removeDrone(int id);
    bool updateDrone(int id, const Drone& updated);
    bool getDrone(int id, Drone& out) const;
    bool setStatus(int id, DroneStatus status);
    bool setBattery(int id, double battery);

    std::vector<Drone> allDronesSortedById() const; // via BST in-order
    std::vector<Drone> availableDrones(double minWeight) const;

    // Finds the nearest available drone (by current battery/status) that can
    // carry the given weight, preferring the one already at (or nearest to)
    // the pickup city -- used by DeliveryManager for auto-assignment.
    int findBestDroneForPickup(const std::string& pickupCity, double weightKg) const;

    size_t totalDrones() const { return tree.size(); }
    size_t activeDrones() const;
    void savePersisted() const { persist(); }
};

#endif // DRONE_MANAGER_H
