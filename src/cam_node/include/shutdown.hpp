#pragma once
#include <csignal>

inline volatile std::sig_atomic_t shutdown_requested = 0;

inline void handle_shutdown(int) {
    shutdown_requested = 1;
}