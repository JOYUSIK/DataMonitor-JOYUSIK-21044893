#pragma once
#include <vector>
#include <map>
#include <string>
#include "model/Sample.h"
#include "model/Order.h"

enum class StockStatus { SURPLUS, SHORTAGE, DEPLETED };

class MonitorController {
public:
    MonitorController(const std::string& dataPath);

    void run();

    static StockStatus calcStockStatus(const Sample& s, const std::vector<Order>& orders);
    static std::map<OrderStatus, int> aggregateOrders(const std::vector<Order>& orders);
    static std::string stockStatusToString(StockStatus s);

private:
    std::string dataPath_;

    void refresh() const;
};
