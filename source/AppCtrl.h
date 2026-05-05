#ifndef APP_CONTROL_H
#define APP_CONTROL_H


#include "NodeManagament.h"
#include "IOHandler.h"
#include <windows.h>

enum class PROGRAMSTATE {INIT, EXECUTE, FINALLY, SHUTDOWN};

std::ofstream g_debugFile;

// Program state transition functions
PROGRAMSTATE initialize();   // Initializes all modules
PROGRAMSTATE execute();      // Main interactive loop
PROGRAMSTATE finalize();     // Cleanup before shutdown
void printOption();          // Display menu options

// Debug console management
void InitDebugConsole();     // Creates a separate PowerShell window for debug logs
void CloseDebugConsole();    // Closes the debug console window

#define CYCLE_TIME 10
#endif /* APP_CONTROL_H */