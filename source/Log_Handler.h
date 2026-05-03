#ifndef LOG_HANDLE_H
#define LOG_HANDLE_H

#include <iostream>
#include <sstream>
#include <windows.h>

#define ENABLE_DEBUG_LOG

#ifdef ENABLE_DEBUG_LOG

#define DEBUG_LOG(msg) do { \
    std::ostringstream oss; \
    oss << "ThreadID [" << GetCurrentThreadId() << "] " \
        << __FUNCTION__ << " (line " << __LINE__ << "): " << msg; \
    std::cout << oss.str() << std::endl; \
} while(0)

#else
#define DEBUG_LOG(msg, ...)
#endif // ENABLE_DEBUG_LOG

#endif /* LOG_HANDLE_H */