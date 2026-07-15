#include "model/Order.h"
#include <stdexcept>

OrderStatus Order::statusFromString(const std::string& s) {
    if (s == "RESERVED")  return OrderStatus::RESERVED;
    if (s == "REJECTED")  return OrderStatus::REJECTED;
    if (s == "PRODUCING") return OrderStatus::PRODUCING;
    if (s == "CONFIRMED") return OrderStatus::CONFIRMED;
    if (s == "RELEASE")   return OrderStatus::RELEASE;
    throw std::runtime_error("알 수 없는 주문 상태: " + s);
}

std::string Order::statusToString(OrderStatus s) {
    switch (s) {
        case OrderStatus::RESERVED:  return "RESERVED";
        case OrderStatus::REJECTED:  return "REJECTED";
        case OrderStatus::PRODUCING: return "PRODUCING";
        case OrderStatus::CONFIRMED: return "CONFIRMED";
        case OrderStatus::RELEASE:   return "RELEASE";
        default:                     return "UNKNOWN";
    }
}

Order Order::fromJson(const nlohmann::json& j) {
    return {
        j.at("order_id").get<std::string>(),
        j.at("sample_id").get<std::string>(),
        j.at("customer").get<std::string>(),
        j.at("quantity").get<int>(),
        statusFromString(j.at("status").get<std::string>()),
        j.at("created_at").get<std::string>()
    };
}
