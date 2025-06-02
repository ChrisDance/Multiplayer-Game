#pragma once
#include <csignal>
#include <atomic>

class Shutdown
{
private:
    static inline std::atomic<bool> shutdown_requested{false};

public:
    static void signal_handler(int signal)
    {
        if (signal == SIGINT || signal == SIGTERM)
        {
            shutdown_requested = true;
        }
    }

    static void setup()
    {
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
    }

    static bool should_shutdown()
    {
        return shutdown_requested.load();
    }
};
