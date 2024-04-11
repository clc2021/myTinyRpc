#ifndef SERVICE_ADDRESS_H
#define SERVICE_ADDRESS_H
#include <iostream>
#include <string>

struct ServiceAddress
{
    std::string serviceName;
    std::string ip;
    uint16_t port;

    bool operator< (const ServiceAddress & other) const {
        return ip != other.ip ? ip < other.ip : port < other.port;
    }
};
#endif