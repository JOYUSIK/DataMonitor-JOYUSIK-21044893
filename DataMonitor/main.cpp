#include <iostream>
#include <string>
#define NOMINMAX
#include <windows.h>
#include "controller/MonitorController.h"
#include "test/TestRunner.h"

static const std::string DEFAULT_DATA_PATH = ".\\data";

static std::string parseDataPath(int argc, char* argv[]) {
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--path") return argv[i + 1];
    }
    return DEFAULT_DATA_PATH;
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 테스트 모드: DataMonitor.exe --test
    if (argc >= 2 && std::string(argv[1]) == "--test") {
        return runAllTests();
    }

    std::string dataPath = parseDataPath(argc, argv);
    std::cout << " DataMonitor 시작 (경로: " << dataPath << ")\n";

    MonitorController controller(dataPath);
    controller.run();
    return 0;
}
