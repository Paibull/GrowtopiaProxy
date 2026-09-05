#include "../Logger/Logger.hpp"
#include "../Functions/Functions.hpp"
#include "../Http/Http.hpp"
#include "../Network/Network.hpp"

#include <thread>

static BOOL WINAPI ConsoleHandler(DWORD signal) {
    switch (signal) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            System::editHosts("");          /* also flushes the DNS cache */
            FastLog::Logger::instance().stop();
            return FALSE;                   /* let the default handler finish the exit */
        default:
            return FALSE;
    }
}

auto main() -> int {
    if (!System::IsTrueAdmin() && !System::RelaunchAsAdmin()) {
        LOG_ERROR("Couldn't relaunched the program as admin!");
        return EXIT_FAILURE;
    }
    System::EnableDebugPrivilege();
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetProcessPriorityBoost(GetCurrentProcess(), TRUE);

    if (enet_initialize() != 0) {
        LOG_ERROR("ENet initialization failed!");
        return -1;
    }
    atexit(enet_deinitialize);

    System::editHosts("127.0.0.1");

    FastLog::Logger::instance().start();
    FastLog::Logger::set_thread_name("MAIN");

    if (System::findProcess(L"Growtopia.exe") != -1) {
        LOG_WARN("Growtopia.exe is already running; leaving it alone.");
        LOG_WARN("Go to the menu and press 'Play Online' to route it through the proxy.");
    }

    LOG_WARN("Setting up all threads");

    std::thread http_thread(&HttpManager::Injector, &Http);
    std::thread network_thread(&NetworkManager::Injector, &Network);

    HANDLE hHttp = (HANDLE)http_thread.native_handle();
    HANDLE hNetwork = (HANDLE)network_thread.native_handle();

    SetThreadPriority(hHttp, THREAD_PRIORITY_BELOW_NORMAL);
    SetThreadPriority(hNetwork, THREAD_PRIORITY_TIME_CRITICAL);
    SetThreadAffinityMask(hNetwork, 1 << 2);

    http_thread.join();
    network_thread.join();
    
    LOG_WARN("All threads finished");

    FastLog::Logger::instance().stop();

    return EXIT_SUCCESS;
}