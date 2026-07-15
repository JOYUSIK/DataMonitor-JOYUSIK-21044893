#include "TestRunner.h"
#include "model/Order.h"

TEST("Order::statusFromString - RESERVED") {
    ASSERT_TRUE(Order::statusFromString("RESERVED") == OrderStatus::RESERVED);
}

TEST("Order::statusFromString - CONFIRMED") {
    ASSERT_TRUE(Order::statusFromString("CONFIRMED") == OrderStatus::CONFIRMED);
}

TEST("Order::statusFromString - PRODUCING") {
    ASSERT_TRUE(Order::statusFromString("PRODUCING") == OrderStatus::PRODUCING);
}

TEST("Order::statusFromString - RELEASE") {
    ASSERT_TRUE(Order::statusFromString("RELEASE") == OrderStatus::RELEASE);
}

TEST("Order::statusFromString - REJECTED") {
    ASSERT_TRUE(Order::statusFromString("REJECTED") == OrderStatus::REJECTED);
}

TEST("Order::fromJson - 전체 필드 파싱") {
    auto j = nlohmann::json{
        {"order_id", "ORD-20260715-0001"},
        {"sample_id", "S-001"},
        {"customer", "삼성전자 파운드리"},
        {"quantity", 200},
        {"status", "RESERVED"},
        {"created_at", "2026-07-15T09:32:15"}
    };
    Order o = Order::fromJson(j);
    ASSERT_STR_EQ(o.orderId, "ORD-20260715-0001");
    ASSERT_STR_EQ(o.sampleId, "S-001");
    ASSERT_EQ(o.quantity, 200);
    ASSERT_TRUE(o.status == OrderStatus::RESERVED);
}

TEST("Order::statusToString - 왕복 변환") {
    ASSERT_STR_EQ(Order::statusToString(OrderStatus::CONFIRMED), "CONFIRMED");
    ASSERT_STR_EQ(Order::statusToString(OrderStatus::PRODUCING), "PRODUCING");
}
