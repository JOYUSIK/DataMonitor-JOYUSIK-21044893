#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& testRegistry() {
    static std::vector<TestCase> reg;
    return reg;
}

inline int runAllTests() {
    int passed = 0, failed = 0;
    std::cout << "\n=== 테스트 실행 ===\n";
    for (auto& tc : testRegistry()) {
        try {
            tc.fn();
            std::cout << "[PASS] " << tc.name << "\n";
            ++passed;
        } catch (const std::exception& e) {
            std::cout << "[FAIL] " << tc.name << " => " << e.what() << "\n";
            ++failed;
        }
    }
    std::cout << "\n결과: " << passed << " 통과 / " << failed << " 실패\n";
    return failed;
}

#define TEST(name) \
    static void CONCAT(test_, __LINE__)(); \
    static bool CONCAT(_reg_, __LINE__) = \
        (testRegistry().push_back({name, CONCAT(test_, __LINE__)}), true); \
    static void CONCAT(test_, __LINE__)()

#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a##b

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        throw std::runtime_error( \
            std::string("기대 [") + std::to_string(b) + "] 실제 [" + std::to_string(a) + "]"); \
    } \
} while(0)

#define ASSERT_STR_EQ(a, b) do { \
    if (std::string(a) != std::string(b)) { \
        throw std::runtime_error( \
            std::string("기대 '") + (b) + "' 실제 '" + (a) + "'"); \
    } \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) throw std::runtime_error("조건 실패: " #cond); \
} while(0)

#define ASSERT_FALSE(cond) do { \
    if (cond) throw std::runtime_error("거짓 기대 실패: " #cond); \
} while(0)
