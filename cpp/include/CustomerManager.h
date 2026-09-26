#ifndef CUSTOMER_MANAGER_H
#define CUSTOMER_MANAGER_H

#include "Customer.h"
#include "DataStructures.h"
#include <string>
#include <vector>

/*******************************************************************************
 * CustomerManager
 * HashMap<int, Customer>          -> O(1) lookup by customer ID
 * HashMap<string, int>            -> O(1) lookup of ID by email (login index)
 ******************************************************************************/
class CustomerManager {
private:
    HashMap<int, Customer> byId;
    HashMap<std::string, int> emailToId;
    int nextId;
    std::string dataFile;

    void persist() const;
    void load();

public:
    explicit CustomerManager(const std::string& dataFile_ = "data/customers.txt");
    ~CustomerManager() { persist(); }

    // returns new customer id, or -1 if email already registered / invalid input
    int registerCustomer(const std::string& name, const std::string& email,
                          const std::string& plainPassword, const std::string& address,
                          const std::string& phone);

    // returns customer id on success, -1 on failure
    int login(const std::string& email, const std::string& plainPassword) const;

    bool getCustomer(int id, Customer& out) const;
    bool removeCustomer(int id);
    std::vector<Customer> allCustomers() const;
    size_t totalCustomers() const { return byId.size(); }
};

#endif // CUSTOMER_MANAGER_H
