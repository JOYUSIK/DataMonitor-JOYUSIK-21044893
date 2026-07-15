#pragma once
#include <vector>
#include <map>
#include <string>
#include "model/Sample.h"
#include "model/Order.h"
#include "controller/MonitorController.h"

class MonitorView {
public:
    void printDashboard(
        const std::string& dataPath,
        const std::vector<Sample>& samples,
        const std::vector<Order>& orders,
        const std::map<OrderStatus, int>& counts) const;

    void printExit() const;

private:
    std::string currentTime() const;
    std::string stockBar(int stock, int maxStock) const;
    std::string statusLabel(StockStatus s) const;
};
