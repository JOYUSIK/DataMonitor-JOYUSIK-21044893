#include "view/MonitorView.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>
#define NOMINMAX
#include <windows.h>
#include <algorithm>

static const std::string LINE(68, '=');
static const std::string THIN(68, '-');
static const int BAR_WIDTH = 16;

void MonitorView::printDashboard(
    const std::string& dataPath,
    const std::vector<Sample>& samples,
    const std::vector<Order>& orders,
    const std::map<OrderStatus, int>& counts) const
{
    // 화면 지우기
    COORD coord = {0, 0};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);

    auto get = [&](OrderStatus s) {
        auto it = counts.find(s);
        return it != counts.end() ? it->second : 0;
    };

    std::cout << "\n" << LINE << "\n";
    std::cout << " S-Semi 데이터 모니터"
              << "  |  " << currentTime()
              << "  |  [R] 새로고침  [Q] 종료\n";
    std::cout << LINE << "\n";

    // 주문 현황
    std::cout << " [주문 현황]\n";
    std::cout << THIN << "\n";
    std::cout << std::left
              << "  " << std::setw(12) << "RESERVED"
              << std::setw(12) << "PRODUCING"
              << std::setw(12) << "CONFIRMED"
              << "RELEASE\n";
    std::cout << "  " << std::setw(12) << get(OrderStatus::RESERVED)
              << std::setw(12) << get(OrderStatus::PRODUCING)
              << std::setw(12) << get(OrderStatus::CONFIRMED)
              << get(OrderStatus::RELEASE) << "\n";
    std::cout << LINE << "\n";

    // 시료 재고 현황
    std::cout << " [시료 재고 현황]\n";
    std::cout << THIN << "\n";

    if (samples.empty()) {
        std::cout << "  데이터 없음 (경로: " << dataPath << "\\samples.json)\n";
    } else {
        int maxStock = 0;
        for (const auto& s : samples) maxStock = std::max(maxStock, s.stock);

        std::cout << std::left
                  << "  " << std::setw(8)  << "ID"
                  << std::setw(22) << "시료명"
                  << std::setw(8)  << "재고"
                  << std::setw(6)  << "상태"
                  << "재고바\n";
        std::cout << THIN << "\n";

        for (const auto& s : samples) {
            auto status = MonitorController::calcStockStatus(s, orders);
            std::cout << "  "
                      << std::setw(8)  << s.id
                      << std::setw(22) << s.name.substr(0, 20)
                      << std::setw(8)  << s.stock
                      << std::setw(6)  << statusLabel(status)
                      << stockBar(s.stock, maxStock) << "\n";
        }
    }

    std::cout << LINE << "\n";
    std::cout << " 경로: " << dataPath
              << "  |  자동갱신: 3초\n";
    std::cout << LINE << "\n";
}

void MonitorView::printExit() const {
    std::cout << "\n 모니터를 종료합니다.\n";
}

std::string MonitorView::currentTime() const {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    struct tm tm_info;
    localtime_s(&tm_info, &t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);
    return buf;
}

std::string MonitorView::stockBar(int stock, int maxStock) const {
    if (maxStock <= 0) return std::string(BAR_WIDTH, '.');
    int filled = static_cast<int>(static_cast<double>(BAR_WIDTH) * stock / maxStock);
    filled = std::clamp(filled, 0, BAR_WIDTH);
    return std::string(filled, '#') + std::string(BAR_WIDTH - filled, '.');
}

std::string MonitorView::statusLabel(StockStatus s) const {
    return MonitorController::stockStatusToString(s);
}
