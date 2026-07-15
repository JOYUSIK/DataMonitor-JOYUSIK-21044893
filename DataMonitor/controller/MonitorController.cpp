#include "controller/MonitorController.h"
#include "repository/SampleRepository.h"
#include "repository/OrderRepository.h"
#include "view/MonitorView.h"
#include <conio.h>
#define NOMINMAX
#include <windows.h>

static const int REFRESH_INTERVAL_SEC = 3;

MonitorController::MonitorController(const std::string& dataPath)
    : dataPath_(dataPath) {}

void MonitorController::run() {
    MonitorView view;
    while (true) {
        refresh();

        // REFRESH_INTERVAL_SEC 동안 100ms 단위로 키 감지
        for (int i = 0; i < REFRESH_INTERVAL_SEC * 10; ++i) {
            Sleep(100);
            if (_kbhit()) {
                char c = static_cast<char>(_getch());
                if (c == 'q' || c == 'Q') { view.printExit(); return; }
                if (c == 'r' || c == 'R') break;
            }
        }
    }
}

void MonitorController::refresh() const {
    SampleRepository sampleRepo;
    OrderRepository  orderRepo;

    auto samples = sampleRepo.findAll(dataPath_);
    auto orders  = orderRepo.findAll(dataPath_);
    auto counts  = aggregateOrders(orders);

    MonitorView view;
    view.printDashboard(dataPath_, samples, orders, counts);
}

StockStatus MonitorController::calcStockStatus(const Sample& s, const std::vector<Order>& orders) {
    if (s.stock == 0) return StockStatus::DEPLETED;

    int pending = 0;
    for (const auto& o : orders) {
        if (o.sampleId == s.id &&
            (o.status == OrderStatus::RESERVED || o.status == OrderStatus::CONFIRMED)) {
            pending += o.quantity;
        }
    }
    return s.stock > pending ? StockStatus::SURPLUS : StockStatus::SHORTAGE;
}

std::map<OrderStatus, int> MonitorController::aggregateOrders(const std::vector<Order>& orders) {
    std::map<OrderStatus, int> counts{
        {OrderStatus::RESERVED,  0},
        {OrderStatus::PRODUCING, 0},
        {OrderStatus::CONFIRMED, 0},
        {OrderStatus::RELEASE,   0},
        {OrderStatus::REJECTED,  0},
    };
    for (const auto& o : orders) {
        if (o.status != OrderStatus::REJECTED) {
            counts[o.status]++;
        }
    }
    return counts;
}

std::string MonitorController::stockStatusToString(StockStatus s) {
    switch (s) {
        case StockStatus::SURPLUS:  return "여유";
        case StockStatus::SHORTAGE: return "부족";
        case StockStatus::DEPLETED: return "고갈";
        default:                    return "알수없음";
    }
}
