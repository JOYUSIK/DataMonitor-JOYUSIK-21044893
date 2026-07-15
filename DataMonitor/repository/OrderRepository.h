#pragma once
#include <vector>
#include <string>
#include "model/Order.h"

class OrderRepository {
public:
    std::vector<Order> findAll(const std::string& dataPath) const;
};
