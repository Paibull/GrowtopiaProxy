#include "Logger.hpp"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <share.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace FastLog {

    namespace {

        inline uint64_t timestamp_ms() noexcept {
            using namespace std::chrono;
            return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        }

        inline const char* level_color(Level lvl) noexcept {
            switch (lvl) {
            case Level::Info:    return "\033[32m";
            case Level::Warning: return "\033[33m";
            case Level::Error:   return "\033[31m";
            case Level::Debug:   return "\033[36m";
            }
            return "\033[0m";
        }

        inline const char* level_to_string(Level lvl) noexcept {
            switch (lvl) {
            case Level::Info:    return "INFO ";
            case Level::Warning: return "WARN ";
            case Level::Error:   return "ERROR";
            case Level::Debug:   return "DEBUG";
            }
            return "";
        }

        inline void format_timestamp(uint64_t ms, char* buf, size_t sz) noexcept {
            uint64_t total_sec = ms / 1000;
            uint64_t h = total_sec / 3600;
            uint64_t m = (total_sec / 60) % 60;
            uint64_t s = total_sec % 60;
            uint64_t millis = ms % 1000;
            std::snprintf(buf, sz, "%02llu:%02llu:%02llu.%03llu", h, m, s, millis);
        }

    } // namespace

    Logger& Logger::instance() {
        static Logger inst;
        return inst;
    }

    Logger::Logger() noexcept {}
    Logger::~Logger() { stop(); }

    void Logger::start() noexcept {
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
#endif
        /* Opened before the writer thread starts so it is never read half-set.
           Truncated per run: this is a diagnostic tail, not an archive. A failure
           to open costs the mirror and nothing else -- the console still works. */
        if (!file_) {
            /* _fsopen, not fopen_s: fopen_s opens with no sharing at all, which
               locks the file for the whole run -- unreadable while the proxy is
               up, which defeats the point of writing it. _SH_DENYWR lets anyone
               read the log live while keeping other writers out. */
            file_ = _fsopen("proxy.log", "w", _SH_DENYWR);
        }

        running_.store(true, std::memory_order_release);
        writer_ = std::thread(&Logger::writer_loop, this);

        HANDLE hWriter = (HANDLE)writer_.native_handle();
        SetThreadPriority(hWriter, THREAD_PRIORITY_BELOW_NORMAL);
    }

    void Logger::stop() noexcept {
        running_.store(false, std::memory_order_release);
        if (writer_.joinable()) writer_.join();

        /* After the join: the writer is the only thing that touches file_. */
        if (file_) {
            std::fclose(file_);
            file_ = nullptr;
        }
    }

    char*& Logger::thread_name_ptr() noexcept {
        thread_local char* ptr = nullptr;
        return ptr;
    }

    void Logger::set_thread_name(const char* name) noexcept {
        thread_local char tname[16]{ 0 };
#ifdef _WIN32
        strncpy_s(tname, name, sizeof(tname) - 1);
#else
        std::strncpy(tname, name, sizeof(tname) - 1);
#endif
        tname[15] = '\0';
        thread_name_ptr() = tname;
    }

    void Logger::push(Level lvl, const char* data, size_t size) noexcept {
        uint32_t index = write_index_.fetch_add(1, std::memory_order_relaxed);
        Slot& slot = queue_[index % QUEUE_SIZE];

        while (slot.ready.load(std::memory_order_acquire)) std::this_thread::yield();

        slot.level = lvl;
        slot.size = static_cast<uint32_t>(size);
        slot.timestamp = timestamp_ms();

        const char* tname = thread_name_ptr();
        if (tname) {
#ifdef _WIN32
            strncpy_s(slot.thread_name, tname, sizeof(slot.thread_name) - 1);
#else
            std::strncpy(slot.thread_name, tname, sizeof(slot.thread_name) - 1);
#endif
        }
        slot.thread_name[15] = '\0';
        std::memcpy(slot.data, data, size);
        slot.ready.store(true, std::memory_order_release);
    }

    void Logger::writer_loop() noexcept {
        unsigned idle_spins = 0;
        while (running_.load(std::memory_order_acquire) || read_index_.load() != write_index_.load()) {
            uint32_t index = read_index_.load(std::memory_order_relaxed);
            Slot& slot = queue_[index % QUEUE_SIZE];

            if (!slot.ready.load(std::memory_order_acquire)) {
                /* yield() returns immediately when nothing else wants the core,
                   so an idle logger used to burn one continuously -- 236 seconds
                   of CPU while sitting at the menu. Spin briefly so a burst is
                   still picked up without delay, then sleep, because nothing
                   here needs sub-millisecond latency. */
                if (++idle_spins < 64) std::this_thread::yield();
                else std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            idle_spins = 0;

            char ts[32]{};
            std::time_t t = static_cast<std::time_t>(slot.timestamp / 1000);
            std::tm tm_struct{};
            localtime_s(&tm_struct, &t);

            std::snprintf(ts, sizeof(ts), "%02d:%02d:%04d %02d:%02d:%02d",
                tm_struct.tm_mday,
                tm_struct.tm_mon + 1,
                tm_struct.tm_year + 1900,
                tm_struct.tm_hour,
                tm_struct.tm_min,
                tm_struct.tm_sec
            );

            const char* lvl = level_to_string(slot.level);
            const char* tname = slot.thread_name[0] ? slot.thread_name : "MAIN";

            const char* color = level_color(slot.level);
            std::fwrite(color, 1, std::strlen(color), stdout);

            std::printf("[%s] [%-5s] [%s] ", ts, lvl, tname);

            std::fwrite(slot.data, 1, slot.size, stdout);
            std::fwrite("\033[0m", 1, 4, stdout);
            std::fwrite("\n", 1, 1, stdout);

            if (file_) {
                std::fprintf(file_, "[%s] [%-5s] [%s] ", ts, lvl, tname);
                std::fwrite(slot.data, 1, slot.size, file_);
                std::fwrite("\n", 1, 1, file_);
                /* Flushed per line on purpose: the runs worth reading are the ones
                   that end in a crash or a kill, and a buffered tail is exactly the
                   part that would be lost. */
                std::fflush(file_);
            }

            slot.ready.store(false, std::memory_order_release);
            read_index_.fetch_add(1, std::memory_order_relaxed);
        }
    }

} // namespace FastLog