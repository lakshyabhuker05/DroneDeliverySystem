#include "CustomerManager.h"
#include "Utils.h"
#include <fstream>

CustomerManager::CustomerManager(const std::string& dataFile_) : nextId(1), dataFile(dataFile_) {
    load();
}

void CustomerManager::load() {
    std::ifstream in(dataFile);
    if (!in.is_open()) return;
    std::string line;
    int maxId = 0;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto tokens = utils::split(line, '|');
        if (tokens.size() < 6) continue;
        Customer c(std::stoi(tokens[0]), tokens[1], tokens[2], tokens[3], tokens[4], tokens[5]);
        byId.put(c.id, c);
        emailToId.put(c.email, c.id);
        if (c.id > maxId) maxId = c.id;
    }
    nextId = maxId + 1;
}

void CustomerManager::persist() const {
    std::ofstream out(dataFile, std::ios::trunc);
    if (!out.is_open()) return;
    for (const auto& kv : byId.items()) out << kv.second.toRow() << "\n";
}

int CustomerManager::registerCustomer(const std::string& name, const std::string& email,
                                       const std::string& plainPassword, const std::string& address,
                                       const std::string& phone) {
    if (!utils::isValidEmail(email)) return -1;
    if (emailToId.contains(email)) return -1; // already registered
    if (name.empty() || plainPassword.size() < 4) return -1;

    Customer c(nextId, name, email, utils::simpleHash(plainPassword), address, phone);
    byId.put(c.id, c);
    emailToId.put(email, c.id);
    int id = nextId;
    nextId++;
    persist();
    return id;
}

int CustomerManager::login(const std::string& email, const std::string& plainPassword) const {
    int id;
    if (!emailToId.get(email, id)) return -1;
    Customer c;
    if (!byId.get(id, c)) return -1;
    if (c.passwordHash != utils::simpleHash(plainPassword)) return -1;
    return id;
}

bool CustomerManager::getCustomer(int id, Customer& out) const {
    return byId.get(id, out);
}

bool CustomerManager::removeCustomer(int id) {
    Customer c;
    if (!byId.get(id, c)) return false;
    byId.remove(id);
    emailToId.remove(c.email);
    persist();
    return true;
}

std::vector<Customer> CustomerManager::allCustomers() const {
    std::vector<Customer> out;
    for (const auto& kv : byId.items()) out.push_back(kv.second);
    return out;
}
