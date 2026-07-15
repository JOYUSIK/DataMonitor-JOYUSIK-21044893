#include "repository/OrderRepository.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

std::vector<Order> OrderRepository::findAll(const std::string& dataPath) const {
    fs::path file = fs::path(dataPath) / "orders.json";
    if (!fs::exists(file)) return {};

    std::ifstream f(file);
    if (!f.is_open()) return {};

    auto j = nlohmann::json::parse(f, nullptr, false);
    if (j.is_discarded()) return {};

    std::vector<Order> result;
    for (const auto& entry : j) {
        result.push_back(Order::fromJson(entry));
    }
    return result;
}
