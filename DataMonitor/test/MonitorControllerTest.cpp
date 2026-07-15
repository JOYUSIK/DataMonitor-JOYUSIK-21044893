#include "TestRunner.h"
#include "controller/MonitorController.h"

// --- 재고 상태 판정 테스트 ---

TEST("calcStockStatus - 고갈: stock == 0") {
    Sample s{"S-001", "웨이퍼", 0.5, 0.92, 0};
    ASSERT_TRUE(MonitorController::calcStockStatus(s, {}) == StockStatus::DEPLETED);
}

TEST("calcStockStatus - 여유: stock > 미처리 합산") {
    Sample s{"S-001", "웨이퍼", 0.5, 0.92, 500};
    std::vector<Order> orders = {
        {"ORD-001", "S-001", "고객A", 100, OrderStatus::RESERVED, ""},
        {"ORD-002", "S-001", "고객B", 150, OrderStatus::CONFIRMED, ""},
    };
    // stock(500) > pending(250) → 여유
    ASSERT_TRUE(MonitorController::calcStockStatus(s, orders) == StockStatus::SURPLUS);
}

TEST("calcStockStatus - 부족: 0 < stock <= 미처리 합산") {
    Sample s{"S-001", "웨이퍼", 0.5, 0.92, 100};
    std::vector<Order> orders = {
        {"ORD-001", "S-001", "고객A", 200, OrderStatus::RESERVED, ""},
    };
    // stock(100) <= pending(200) → 부족
    ASSERT_TRUE(MonitorController::calcStockStatus(s, orders) == StockStatus::SHORTAGE);
}

TEST("calcStockStatus - 여유: 다른 sample_id 주문 무시") {
    Sample s{"S-001", "웨이퍼", 0.5, 0.92, 50};
    std::vector<Order> orders = {
        {"ORD-001", "S-002", "고객A", 9999, OrderStatus::RESERVED, ""},
    };
    // S-002 주문은 S-001에 영향 없음 → 여유
    ASSERT_TRUE(MonitorController::calcStockStatus(s, orders) == StockStatus::SURPLUS);
}

TEST("calcStockStatus - 경계값: stock == 미처리 합산 → 부족") {
    Sample s{"S-001", "웨이퍼", 0.5, 0.92, 200};
    std::vector<Order> orders = {
        {"ORD-001", "S-001", "고객A", 200, OrderStatus::RESERVED, ""},
    };
    // stock(200) == pending(200) → 부족 (초과하지 않으면 부족)
    ASSERT_TRUE(MonitorController::calcStockStatus(s, orders) == StockStatus::SHORTAGE);
}

// --- 주문 상태 집계 테스트 ---

TEST("aggregateOrders - 상태별 카운트") {
    std::vector<Order> orders = {
        {"O1", "S-001", "A", 10, OrderStatus::RESERVED,  ""},
        {"O2", "S-001", "B", 10, OrderStatus::RESERVED,  ""},
        {"O3", "S-001", "C", 10, OrderStatus::CONFIRMED, ""},
        {"O4", "S-001", "D", 10, OrderStatus::PRODUCING, ""},
        {"O5", "S-001", "E", 10, OrderStatus::RELEASE,   ""},
        {"O6", "S-001", "F", 10, OrderStatus::REJECTED,  ""},  // 제외
    };
    auto counts = MonitorController::aggregateOrders(orders);
    ASSERT_EQ(counts[OrderStatus::RESERVED],  2);
    ASSERT_EQ(counts[OrderStatus::CONFIRMED], 1);
    ASSERT_EQ(counts[OrderStatus::PRODUCING], 1);
    ASSERT_EQ(counts[OrderStatus::RELEASE],   1);
    ASSERT_EQ(counts[OrderStatus::REJECTED],  0); // REJECTED 집계 제외
}

TEST("aggregateOrders - 빈 목록") {
    auto counts = MonitorController::aggregateOrders({});
    ASSERT_EQ(counts[OrderStatus::RESERVED], 0);
}
