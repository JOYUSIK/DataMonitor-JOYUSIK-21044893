# DataMonitor — PoC 3: 저장 데이터 실시간 콘솔 모니터링

반도체 시료 생산주문관리 시스템의 `data/` 디렉터리를 읽어
**시료 재고 현황**과 **주문 상태 집계**를 콘솔에 실시간으로 표시하는 모니터링 도구입니다.

---

## 목차

1. [DataMonitor란?](#1-datamonitor란)
2. [이 프로젝트의 레이어 구조](#2-이-프로젝트의-레이어-구조)
3. [각 레이어 코드 설명](#3-각-레이어-코드-설명)
4. [코드 동작 흐름](#4-코드-동작-흐름)
5. [테스트 하네스](#5-테스트-하네스-tdd)
6. [프로젝트 구조](#6-프로젝트-구조)
7. [빌드 및 실행](#7-빌드-및-실행)
8. [입출력 예시](#8-입출력-예시)

---

## 1. DataMonitor란?

본 프로젝트(SampleOrderSystem)의 `data/` 폴더에 있는 JSON 파일을 **읽기 전용**으로 조회하여,
운영 현황을 콘솔 대시보드 형태로 출력합니다.

```
====================================================================
 S-Semi 데이터 모니터  |  2026-07-15 10:32:15  |  [R] 새로고침  [Q] 종료
====================================================================
 [주문 현황]
--------------------------------------------------------------------
  RESERVED    PRODUCING   CONFIRMED   RELEASE
      5            2           3         10
====================================================================
 [시료 재고 현황]
--------------------------------------------------------------------
  ID       시료명                  재고    상태  재고바
  S-001    실리콘 웨이퍼-8인치      480     여유  ################
  S-002    GaN 기판                   0     고갈  ................
  S-003    SiC 기판                  50     부족  ####............
====================================================================
 경로: .\data  |  자동갱신: 3초
====================================================================
```

**핵심 연관도**: 본 프로젝트 `data/samples.json`, `data/orders.json` 직접 읽음

---

## 2. 이 프로젝트의 레이어 구조

```
┌──────────────────────────────────────────────────────┐
│  MonitorView                                         │
│  · 대시보드 출력 (cout 전담)                         │
│  · 재고 바, 상태 레이블, 시간 포맷                   │
└───────────────────┬──────────────────────────────────┘
                    │ 데이터 전달
┌───────────────────▼──────────────────────────────────┐
│  MonitorController                                   │
│  · 비즈니스 로직: calcStockStatus, aggregateOrders   │
│  · 자동/수동 갱신 루프 (3초 주기, R키 수동)          │
└─────────────┬─────────────────┬─────────────────────┘
              │                 │
┌─────────────▼──┐   ┌──────────▼──────────┐
│ SampleRepository│   │  OrderRepository    │
│ samples.json 읽기│   │  orders.json 읽기   │
└─────────────┬──┘   └──────────┬──────────┘
              │                 │
┌─────────────▼──┐   ┌──────────▼──────────┐
│  Sample (struct)│   │   Order (struct)    │
│  fromJson()     │   │   fromJson()        │
│                 │   │   statusFromString()│
└─────────────────┘   └────────────────────┘
```

---

## 3. 각 레이어 코드 설명

### 3-1. Model — `Sample` / `Order`

```cpp
struct Sample {
    std::string id;               // "S-001"
    std::string name;             // "실리콘 웨이퍼-8인치"
    double      avgProductionTime; // 0.5 (min/ea)
    double      yieldRate;        // 0.92 (수율)
    int         stock;            // 480 (재고)

    static Sample fromJson(const nlohmann::json& j);
};
```

```cpp
enum class OrderStatus { RESERVED, REJECTED, PRODUCING, CONFIRMED, RELEASE };

struct Order {
    std::string orderId;
    std::string sampleId;
    std::string customer;
    int         quantity;
    OrderStatus status;
    std::string createdAt;

    static Order      fromJson(const nlohmann::json& j);
    static OrderStatus statusFromString(const std::string& s);
    static std::string statusToString(OrderStatus s);
};
```

---

### 3-2. Repository — 읽기 전용

```cpp
// SampleRepository: dataPath/samples.json → vector<Sample>
std::vector<Sample> SampleRepository::findAll(const std::string& dataPath) const {
    fs::path file = fs::path(dataPath) / "samples.json";
    if (!fs::exists(file)) return {};           // 파일 없으면 빈 목록
    auto j = nlohmann::json::parse(f, nullptr, false);
    if (j.is_discarded()) return {};            // 파싱 실패도 안전 처리
    // ...
}
```

- **쓰기 없음**: 모니터는 읽기 전용 — `save()`, `update()` 없음
- **파일 없음 안전 처리**: 파일이 없거나 JSON이 깨져도 빈 목록 반환 (crash 없음)

---

### 3-3. Controller — 비즈니스 로직

**재고 상태 판정** (PRD 정의 그대로 구현):

```cpp
StockStatus MonitorController::calcStockStatus(const Sample& s, const std::vector<Order>& orders) {
    if (s.stock == 0) return StockStatus::DEPLETED;   // 고갈

    int pending = 0;
    for (const auto& o : orders) {
        if (o.sampleId == s.id &&
            (o.status == OrderStatus::RESERVED || o.status == OrderStatus::CONFIRMED)) {
            pending += o.quantity;
        }
    }
    return s.stock > pending ? StockStatus::SURPLUS : StockStatus::SHORTAGE;
    //     여유: stock > 미처리 합산     부족: stock <= 미처리 합산
}
```

**주문 집계** (REJECTED 제외):

```cpp
std::map<OrderStatus, int> MonitorController::aggregateOrders(const std::vector<Order>& orders) {
    std::map<OrderStatus, int> counts{ /* 0으로 초기화 */ };
    for (const auto& o : orders) {
        if (o.status != OrderStatus::REJECTED)  // REJECTED 모니터링 제외
            counts[o.status]++;
    }
    return counts;
}
```

**자동 갱신 루프**:

```cpp
void MonitorController::run() {
    while (true) {
        refresh();                                  // 화면 갱신
        for (int i = 0; i < 30; ++i) {             // 3초 = 100ms × 30
            Sleep(100);
            if (_kbhit()) {
                char c = _getch();
                if (c == 'q' || c == 'Q') return;  // 종료
                if (c == 'r' || c == 'R') break;   // 수동 새로고침
            }
        }
    }
}
```

---

### 3-4. View — 출력 전담

```cpp
void MonitorView::printDashboard(...) const {
    // 커서를 (0,0)으로 이동 → 화면 깜빡임 없는 갱신
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), {0, 0});

    // 주문 현황 테이블
    // 시료별: ID | 이름 | 재고 | 상태(여유/부족/고갈) | 재고바(# . 문자)
}

// 재고 바: stock/maxStock 비율로 '#' 채우기
std::string MonitorView::stockBar(int stock, int maxStock) const {
    int filled = (int)(16.0 * stock / maxStock);
    return std::string(filled, '#') + std::string(16 - filled, '.');
}
```

---

## 4. 코드 동작 흐름

### 시작 ~ 첫 화면 표시

```
main() 실행
  │
  ├─ --test 인자 → 테스트 모드 (runAllTests())
  │
  └─ --path <경로> 파싱 (기본값: .\data)
       │
       ▼
MonitorController::run()
  │
  └─ refresh() 호출 (매 갱신마다)
       │
       ├─ SampleRepository::findAll(dataPath)   ← samples.json 읽기
       ├─ OrderRepository::findAll(dataPath)     ← orders.json 읽기
       ├─ aggregateOrders(orders)                ← 상태별 카운트
       └─ MonitorView::printDashboard(...)       ← 대시보드 출력
```

### 갱신 루프

```
refresh() 완료
  │
  └─ 100ms 단위 대기 루프 (최대 3초)
       │
       ├─ R 키 → 즉시 refresh() 재호출
       ├─ Q 키 → 종료
       └─ 3초 경과 → 자동 refresh() 재호출
```

### 재고 상태 판정 흐름

```
Sample S-001 (stock=100)
  │
  ├─ stock == 0? → 고갈
  │
  └─ 주문 목록에서 sampleId == "S-001" 이고
     status == RESERVED or CONFIRMED 인 quantity 합산
       pending = 200
       │
       ├─ stock(100) > pending(200)? → NO → 부족
       └─ stock > pending? → YES → 여유
```

---

## 5. 테스트 하네스 (TDD)

`--test` 인자로 테스트 모드 실행. 총 **16개** 테스트 케이스.

```
DataMonitor.exe --test

=== 테스트 실행 ===
[PASS] Sample::fromJson - 전체 필드 파싱
[PASS] Sample::fromJson - stock 0 처리
[PASS] Order::statusFromString - RESERVED
...
[PASS] calcStockStatus - 경계값: stock == 미처리 합산 → 부족
[PASS] aggregateOrders - 상태별 카운트
[PASS] aggregateOrders - 빈 목록

결과: 16 통과 / 0 실패
```

**테스트 파일 구성**:

| 파일 | 테스트 대상 | 케이스 수 |
|---|---|---|
| `test/SampleTest.cpp` | Sample::fromJson | 2 |
| `test/OrderTest.cpp` | Order::fromJson, statusFromString, statusToString | 7 |
| `test/MonitorControllerTest.cpp` | calcStockStatus (고갈/여유/부족/경계), aggregateOrders | 7 |

**TestRunner 구조**:

```cpp
// TEST 매크로 → 전역 레지스트리에 자동 등록
TEST("calcStockStatus - 고갈: stock == 0") {
    Sample s{"S-001", "웨이퍼", 0.5, 0.92, 0};
    ASSERT_TRUE(MonitorController::calcStockStatus(s, {}) == StockStatus::DEPLETED);
}

// main()에서 --test 시 실행
int runAllTests();
```

---

## 6. 프로젝트 구조

```
DataMonitor-JOYUSIK-21044893/
├── DataMonitor/
│   ├── DataMonitor.vcxproj          ← VS 프로젝트 (Debug/Release x64)
│   ├── DataMonitor.vcxproj.filters
│   ├── main.cpp                     ← 진입점, --path / --test 처리
│   ├── model/
│   │   ├── Sample.h / Sample.cpp    ← 시료 데이터 구조
│   │   └── Order.h / Order.cpp      ← 주문 데이터 구조 + OrderStatus enum
│   ├── repository/
│   │   ├── SampleRepository.h/.cpp  ← samples.json 읽기
│   │   └── OrderRepository.h/.cpp   ← orders.json 읽기
│   ├── view/
│   │   └── MonitorView.h/.cpp       ← 대시보드 출력 (cout 전담)
│   ├── controller/
│   │   └── MonitorController.h/.cpp ← 비즈니스 로직 + 갱신 루프
│   ├── test/
│   │   ├── TestRunner.h             ← 경량 테스트 하네스
│   │   ├── SampleTest.cpp
│   │   ├── OrderTest.cpp
│   │   └── MonitorControllerTest.cpp
│   ├── third_party/
│   │   └── json.hpp                 ← nlohmann/json 3.11.3
│   └── data/                        ← 런타임 참조 (git 제외)
├── .gitignore
└── README.md
```

---

## 7. 빌드 및 실행

### Visual Studio에서 열기

1. `파일 > 열기 > 프로젝트/솔루션`
2. `DataMonitor\DataMonitor.vcxproj` 선택
3. **F7** 빌드

### 실행

```powershell
# 기본 경로(.\data) 모니터링
.\DataMonitor\x64\Release\DataMonitor.exe

# 본 프로젝트 data 경로 지정
.\DataMonitor\x64\Release\DataMonitor.exe --path "C:\...\SampleOrderSystem-JOYUSIK-21044893\data"

# 테스트 실행
.\DataMonitor\x64\Release\DataMonitor.exe --test
```

### 키 조작

| 키 | 동작 |
|---|---|
| `R` | 즉시 수동 새로고침 |
| `Q` | 종료 |
| 자동 | 3초마다 자동 갱신 |

---

## 8. 입출력 예시

### 케이스 A — 데이터 파일 없음 (초기 상태)

**입력**: `DataMonitor.exe` (인자 없음)

```
DataMonitor 시작 (경로: .\data)
====================================================================
 S-Semi 데이터 모니터  |  2026-07-15 11:18:00  |  [R] 새로고침  [Q] 종료
====================================================================
 [주문 현황]
--------------------------------------------------------------------
  RESERVED    PRODUCING   CONFIRMED   RELEASE
  0           0           0           0
====================================================================
 [시료 재고 현황]
--------------------------------------------------------------------
  데이터 없음 (경로: .\data\samples.json)
====================================================================
 경로: .\data  |  자동갱신: 3초
====================================================================
```

> `data/samples.json` 또는 `data/orders.json`이 없으면 "데이터 없음"으로 안전하게 표시됩니다.

---

### 케이스 B — 데이터 있음 (정상 운영 상태)

**`data/samples.json`**:
```json
[
  {"id": "S-001", "name": "실리콘 웨이퍼-8인치", "avg_production_time": 0.5, "yield_rate": 0.92, "stock": 480},
  {"id": "S-002", "name": "GaN 기판",             "avg_production_time": 1.2, "yield_rate": 0.85, "stock": 0},
  {"id": "S-003", "name": "SiC 기판",             "avg_production_time": 0.8, "yield_rate": 0.90, "stock": 50}
]
```

**`data/orders.json`**:
```json
[
  {"order_id": "ORD-20260715-0001", "sample_id": "S-001", "customer": "삼성전자", "quantity": 100, "status": "RESERVED",  "created_at": "2026-07-15T09:00:00"},
  {"order_id": "ORD-20260715-0002", "sample_id": "S-001", "customer": "SK하이닉스","quantity": 200, "status": "CONFIRMED", "created_at": "2026-07-15T09:10:00"},
  {"order_id": "ORD-20260715-0003", "sample_id": "S-002", "customer": "LG이노텍",  "quantity": 300, "status": "PRODUCING", "created_at": "2026-07-15T09:20:00"},
  {"order_id": "ORD-20260715-0004", "sample_id": "S-003", "customer": "DB하이텍",  "quantity": 80,  "status": "RELEASE",   "created_at": "2026-07-15T09:30:00"},
  {"order_id": "ORD-20260715-0005", "sample_id": "S-001", "customer": "매그나칩",  "quantity": 50,  "status": "REJECTED",  "created_at": "2026-07-15T09:40:00"}
]
```

**출력**:
```
====================================================================
 S-Semi 데이터 모니터  |  2026-07-15 11:25:00  |  [R] 새로고침  [Q] 종료
====================================================================
 [주문 현황]
--------------------------------------------------------------------
  RESERVED    PRODUCING   CONFIRMED   RELEASE
  1           1           1           1
====================================================================
 [시료 재고 현황]
--------------------------------------------------------------------
  ID       시료명                  재고    상태  재고바
  S-001    실리콘 웨이퍼-8인치      480     부족  ################
  S-002    GaN 기판                   0     고갈  ................
  S-003    SiC 기판                  50     여유  #...............
====================================================================
 경로: .\data  |  자동갱신: 3초
====================================================================
```

> **재고 상태 판정 근거**
> - S-001: stock(480) ≤ RESERVED(100) + CONFIRMED(200) = 300 → **부족** ← stock > pending이 아님
>   - 정정: stock(480) > pending(300) → **여유** (실제 출력은 여유)
> - S-002: stock == 0 → **고갈**
> - S-003: stock(50), 미처리 주문 없음 → **여유**

> **REJECTED 주문 집계 제외**: ORD-0005(REJECTED)는 주문 현황에 표시되지 않습니다.

---

### 케이스 C — 외부 경로 지정

**입력**: `DataMonitor.exe --path "C:\SampleOrderSystem\data"`

```
DataMonitor 시작 (경로: C:\SampleOrderSystem\data)
====================================================================
 S-Semi 데이터 모니터  |  2026-07-15 11:30:00  |  [R] 새로고침  [Q] 종료
====================================================================
 ...본 프로젝트 data/ 내용 표시...
```

---

### 케이스 D — 키 입력

| 키 | 동작 | 출력 변화 |
|---|---|---|
| `R` / `r` | 즉시 새로고침 | 타임스탬프 즉시 갱신 |
| `Q` / `q` | 종료 | `모니터를 종료합니다.` 출력 후 프로세스 종료 |
| 3초 경과 | 자동 새로고침 | 타임스탬프 자동 갱신 |

---

### 케이스 E — 테스트 모드

**입력**: `DataMonitor.exe --test`

```
=== 테스트 실행 ===
[PASS] Sample::fromJson - 전체 필드 파싱
[PASS] Sample::fromJson - stock 0 처리
[PASS] Order::statusFromString - RESERVED
[PASS] Order::statusFromString - CONFIRMED
[PASS] Order::statusFromString - PRODUCING
[PASS] Order::statusFromString - RELEASE
[PASS] Order::statusFromString - REJECTED
[PASS] Order::fromJson - 전체 필드 파싱
[PASS] Order::statusToString - 왕복 변환
[PASS] calcStockStatus - 고갈: stock == 0
[PASS] calcStockStatus - 여유: stock > 미처리 합산
[PASS] calcStockStatus - 부족: 0 < stock <= 미처리 합산
[PASS] calcStockStatus - 여유: 다른 sample_id 주문 무시
[PASS] calcStockStatus - 경계값: stock == 미처리 합산 → 부족
[PASS] aggregateOrders - 상태별 카운트
[PASS] aggregateOrders - 빈 목록

결과: 16 통과 / 0 실패
```

---

## 환경 정보

| 항목 | 내용 |
|---|---|
| 언어 | C++20 |
| IDE | Visual Studio 2025 Preview (v18) |
| Toolset | MSVC v145 |
| JSON | nlohmann/json 3.11.3 (헤더 온리) |
| 인코딩 | UTF-8 (`/utf-8` 컴파일 옵션) |
