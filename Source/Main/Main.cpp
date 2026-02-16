#include "../Logger/Logger.hpp"
#include "../Functions/Functions.hpp"
#include "../Http/Http.hpp"
#include "../Network/Network.hpp"

#include <thread>

auto main() -> int {
    if (!System::IsTrueAdmin() && !System::RelaunchAsAdmin()) {
        LOG_ERROR("Couldn't relaunched the program as admin!");
        return EXIT_FAILURE;
    }
    System::EnableDebugPrivilege();
    if (System::findProcess(L"Growtopia.exe")) {
        System::endProcess(L"Growtopia.exe");
        Sleep(1500);
    }

    FastLog::Logger::instance().start();
    FastLog::Logger::set_thread_name("MAIN");

    LOG_WARN("Setting up all threads");

    std::thread http_thread(&HttpManager::Injector, &Http);
    std::thread network_thread(&NetworkManager::Injector, &Network);

    http_thread.join();
    network_thread.join();
    
    LOG_WARN("All threads finished");

    FastLog::Logger::instance().stop();

    return EXIT_SUCCESS;
}

/* 
    @important -> The informations that you need to read before starting to this project
    @info -> Just to give you some extra informations
    @todo -> I will code these things later when I have time
    @debug -> The text of the some important packets (The keys are changed)

    Letter from Batu Tekoz (the creator of this proxy) {
        Hi guys,

        I wanted to code a 'Growtopia Proxy' to help out some people who doesn't have any experience on this journey. 
        If you would like to use my project, feel free to use and ask questions on discord (@xoidnl)
        Sorry for some of the 'Shit Codes' but I have finished this project in 2 days :)

        I hope you will enjoy the debugging survey of Growtopia client and server packets.

        Batu Tekoz,
        batutekoz@gmail.com,
        @xoidnl on discord,
        @batutekoz on instagram
    }
*/