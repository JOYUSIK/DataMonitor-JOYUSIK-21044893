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

    // 실제 콘솔 핸들인지 확인 (파이프/리다이렉트 환경 대응)
    bool isConsole = (GetFileType(hIn) == FILE_TYPE_CHAR);

    if (isConsole) {
        // 마우스/창 이벤트를 입력 버퍼에서 제거 (노이즈 차단)
        DWORD mode = 0;
        GetConsoleMode(hIn, &mode);
        SetConsoleMode(hIn, (mode & ~ENABLE_MOUSE_INPUT & ~ENABLE_WINDOW_INPUT)
                            | ENABLE_PROCESSED_INPUT);
    }

    MonitorView view;
    while (true) {
        refresh();

        // REFRESH_INTERVAL_MS 동안 100ms 단위로 폴링
        for (DWORD elapsed = 0; elapsed < REFRESH_INTERVAL_MS; elapsed += 100) {
            Sleep(100);

            if (!isConsole) continue;

            // 블로킹 없이 대기 중인 이벤트 수 확인
            DWORD numEvents = 0;
            if (!GetNumberOfConsoleInputEvents(hIn, &numEvents)) continue;

            // 정확히 numEvents 개만 읽기 (ReadConsoleInput 블로킹 방지)
            while (numEvents > 0) {
                INPUT_RECORD ir{};
                DWORD numRead = 0;
                ReadConsoleInput(hIn, &ir, 1, &numRead);
                if (numRead == 0) break;
                --numEvents;

                if (ir.EventType != KEY_EVENT)       continue;
                if (!ir.Event.KeyEvent.bKeyDown)     continue;

                char c = ir.Event.KeyEvent.uChar.AsciiChar;
                if (c == 'q' || c == 'Q') { view.printExit(); return; }
                if (c == 'r' || c == 'R') { elapsed = REFRESH_INTERVAL_MS; break; }
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
