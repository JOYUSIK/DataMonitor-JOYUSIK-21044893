#pragma once
#include <string>
#include "third_party/json.hpp"

enum class OrderStatus { RESERVED, REJECTED, PRODUCING, CONFIRMED, RELEASE };

struct Order {
    std::string orderId;
    std::string sampleId;
    std::string customer;
    int quantity;
    OrderStatus status;
    std::string createdAt;

    static Order fromJson(const nlohmann::json& j);
    static OrderStatus statusFromString(const std::string& s);
    static std::string statusToString(OrderStatus s);
};
