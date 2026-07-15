#include "TestRunner.h"
#include "model/Sample.h"

TEST("Sample::fromJson - 전체 필드 파싱") {
    auto j = nlohmann::json{
        {"id", "S-001"},
        {"name", "실리콘 웨이퍼-8인치"},
        {"avg_production_time", 0.5},
        {"yield_rate", 0.92},
        {"stock", 480}
    };
    Sample s = Sample::fromJson(j);
    ASSERT_STR_EQ(s.id, "S-001");
    ASSERT_STR_EQ(s.name, "실리콘 웨이퍼-8인치");
    ASSERT_EQ(s.stock, 480);
    ASSERT_TRUE(s.yieldRate > 0.91 && s.yieldRate < 0.93);
    ASSERT_TRUE(s.avgProductionTime > 0.49 && s.avgProductionTime < 0.51);
}

TEST("Sample::fromJson - stock 0 처리") {
    auto j = nlohmann::json{
        {"id", "S-002"}, {"name", "GaN 기판"},
        {"avg_production_time", 1.0}, {"yield_rate", 0.85}, {"stock", 0}
    };
    Sample s = Sample::fromJson(j);
    ASSERT_EQ(s.stock, 0);
}
