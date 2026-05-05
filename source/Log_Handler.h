#ifndef LOG_HANDLE_H
#define LOG_HANDLE_H

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <windows.h>
#include <fstream>

extern std::ofstream g_debugFile;

#define ENABLE_DEBUG_LOG
#ifdef ENABLE_DEBUG_LOG

#define DEBUG_LOG(msg) do { \
    if (g_debugFile.is_open()) { \
        std::ostringstream oss; \
        auto now = std::chrono::system_clock::now(); \
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000; \
        std::time_t tt = std::chrono::system_clock::to_time_t(now); \
        struct tm tm; \
        localtime_s(&tm, &tt); \
        oss << "ThreadID [" << GetCurrentThreadId() << "] " \
            << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count() \
            << " " << __FUNCTION__ << " (line " << __LINE__ << "): " << msg; \
        g_debugFile << oss.str() << std::endl; \
        g_debugFile.flush(); \
    } \
} while(0)

#else
#define DEBUG_LOG(msg, ...)
#endif // ENABLE_DEBUG_LOG


#define ENABLE_MAIN_LOG
#ifdef ENABLE_MAIN_LOG

#define MAIN_LOG(msg) do { \
    std::ostringstream oss; \
    oss << "ThreadID [" << GetCurrentThreadId() << "] " \
        << __FUNCTION__ << msg; \
    std::cout << oss.str() << std::endl; \
} while(0)

#else
#define MAIN_LOG(msg, ...)
#endif // ENABLE_MAIN_LOG



#endif /* LOG_HANDLE_H */