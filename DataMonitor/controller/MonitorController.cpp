#include "controller/MonitorController.h"
#include "repository/SampleRepository.h"
#include "repository/OrderRepository.h"
#include "view/MonitorView.h"
#define NOMINMAX
#include <windows.h>

static const DWORD REFRESH_INTERVAL_MS = 3000;

MonitorController::MonitorController(const std::string& dataPath)
    : dataPath_(dataPath) {}

void MonitorController::run() {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

    // 마우스/창 이벤트를 ReadConsoleInput이 받을 수 있도록 모드 설정
    DWORD prevMode = 0;
    GetConsoleMode(hIn, &prevMode);
    SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS | ENABLE_PROCESSED_INPUT);

    MonitorView view;
    while (true) {
        refresh();

        // 최대 REFRESH_INTERVAL_MS 대기, 키 이벤트만 처리
        DWORD result = WaitForSingleObject(hIn, REFRESH_INTERVAL_MS);
        if (result == WAIT_OBJECT_0) {
            INPUT_RECORD records[8];
            DWORD numRead = 0;
            ReadConsoleInput(hIn, records, 8, &numRead);

            for (DWORD i = 0; i < numRead; ++i) {
                if (records[i].EventType != KEY_EVENT) continue;
                if (!records[i].Event.KeyEvent.bKeyDown)  continue;

                char c = records[i].Event.KeyEvent.uChar.AsciiChar;
                if (c == 'q' || c == 'Q') {
                    SetConsoleMode(hIn, prevMode);
                    view.printExit();
                    return;
                }
                // R 또는 다른 키 → 타임아웃 없이 즉시 새로고침
            }
        }
        // WAIT_TIMEOUT → 자동 새로고침
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
