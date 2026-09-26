#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <string>
#include <sstream>
#include <functional>

class Customer {
public:
    int id;
    std::string name;
    std::string email;
    std::string passwordHash; // simple salted hash, see Utils.h
    std::string address;
    std::string phone;

    Customer() : id(0) {}
    Customer(int id_, std::string name_, std::string email_, std::string passwordHash_,
              std::string address_, std::string phone_)
        : id(id_), name(std::move(name_)), email(std::move(email_)),
          passwordHash(std::move(passwordHash_)), address(std::move(address_)), phone(std::move(phone_)) {}

    std::string toJSON() const {
        std::ostringstream os;
        os << "{"
           << "\"id\":" << id << ","
           << "\"name\":\"" << name << "\","
           << "\"email\":\"" << email << "\","
           << "\"address\":\"" << address << "\","
           << "\"phone\":\"" << phone << "\""
           << "}";
        return os.str();
    }

    std::string toRow() const {
        std::ostringstream os;
        os << id << "|" << name << "|" << email << "|" << passwordHash << "|" << address << "|" << phone;
        return os.str();
    }
};

#endif // CUSTOMER_H
