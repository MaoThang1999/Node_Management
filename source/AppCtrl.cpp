
#include <thread>
#include <chrono>
#include "AppCtrl.h"


/**
 * @brief Initializes all modules (NodeManagament, IOHandler).
 * @return PROGRAMSTATE::EXECUTE on success .
 */
PROGRAMSTATE initialize(){

    SharedContext ctx;
    int ret = initUDPSock(ctx);
    if(ret == SUCCESS_CODE){
        IOHandler::getInstall()->setUDPSocket(ctx);
    }else{
        MAIN_LOG("Set UDP Sock Fail");
    }

    ret = NodeManagament::getInstall()->start();
    if(ret){
        MAIN_LOG("Start NodeManagament Done");
    }else{
        MAIN_LOG("Start NodeManagament fail");
    }
    ret = IOHandler::getInstall()->start();
    if(ret){
        MAIN_LOG("Start IOHandler Done");
    }else{
        MAIN_LOG("Start IOHandler fail");
    }

    
    return PROGRAMSTATE::EXECUTE;
}

/**
 * @brief Displays menu, reads user input, and performs corresponding actions.
 * @return PROGRAMSTATE::EXECUTE to stay in loop, or PROGRAMSTATE::FINALLY to quit.
 */
PROGRAMSTATE execute(){
    printOption();
    PROGRAMSTATE ret = PROGRAMSTATE::EXECUTE;
    std::string input;
    std::getline(std::cin, input);

    if (input == "1") {
        if(NodeManagament::getInstall()->start()){
            MAIN_LOG("Start NodeManagament Done");
        }else{
            MAIN_LOG("NodeManagament had started before");
        }
        ret =  PROGRAMSTATE::EXECUTE;
    }else if(input == "2"){
        if(IOHandler::getInstall()->start()){
            MAIN_LOG("Start IOHandler Done");
        }else{
            MAIN_LOG("IOHandler had started before");
        }
        ret =  PROGRAMSTATE::EXECUTE;

    }else if(input == "3"){
        if(NodeManagament::getInstall()->stop()){
            MAIN_LOG("Stop NodeManagament Done");
        }else{
            MAIN_LOG("NodeManagament had stoped before");
        }
        ret =  PROGRAMSTATE::EXECUTE;

    }else if(input == "4"){

        if(IOHandler::getInstall()->stop()){
            MAIN_LOG("Stop IOHandler Done");
        }else{
            MAIN_LOG("IOHandler had Stoped before");
        }
        ret =  PROGRAMSTATE::EXECUTE;

    }else if(input == "5"){
        bool NodeManager_Running = NodeManagament::getInstall()->isRunning();
        bool IOHandler_Running = IOHandler::getInstall()->isRunning();
        MAIN_LOG("NodeManagament " << (NodeManager_Running ? "is running" : "stop") << ", IOHandler " <<  (IOHandler_Running ? "is running" : "stop") );
        ret =  PROGRAMSTATE::EXECUTE;

    }else if(input == "q"){
        ret =  PROGRAMSTATE::FINALLY;
    }else{
        // Invalid input – ignore and loop again
    }
    std::cout << std::endl;
    return ret;
}

/**
 * @brief Performs cleanup before shutdown.
 * @return PROGRAMSTATE::SHUTDOWN to exit.
 */
PROGRAMSTATE finalize(){
    IOHandler::getInstall()->stop();
    NodeManagament::getInstall()->stop();
    SharedContext ctx = IOHandler::getInstall()->getUDPSocket();
    closeUDPSock(ctx);
    return PROGRAMSTATE::SHUTDOWN;
}


/**
 * @brief Prints the interactive menu options to console.
 */
void printOption(){
    std::cout   << "-------------------------------------------------------------" << std::endl
                << "Please choose a option" << std::endl
                << "Enter 1 to start NodeManagament" << std::endl
                << "Enter 2 to start IOHandler" << std::endl
                << "Enter 3 to stop NodeManagament" << std::endl
                << "Enter 4 to stop IOHandler" << std::endl
                << "Enter 5 to get system status" << std::endl
                << "Enter q to quit app" << std::endl;
}

/**
 * @brief Initializes the debug console: opens debug.log (truncates previous content)
 *        and launches a PowerShell window with title "NoM Log" that tails the log.
 */
void InitDebugConsole() {
    
    // Open file debug.log (Mode append)
    g_debugFile.open("debug.log", std::ios::out );
    if (!g_debugFile) {
        //std::cerr << "Cannot open debug.log" << std::endl;
        return;
    }
    
    // Create new console 
    system("start powershell -NoExit -Command \"$Host.UI.RawUI.WindowTitle = 'NoM Log'; Get-Content debug.log -Wait\"");
}

/**
 * @brief Closes the PowerShell debug console window by sending WM_CLOSE.
 *        The window title must match exactly 'NoM Log'.
 */
void CloseDebugConsole() {
   
    HWND hWnd = FindWindowW(NULL, L"NoM Log");
    if (hWnd != NULL) {
        // Close Window
        SendMessageW(hWnd, WM_CLOSE, 0, 0);
    }
}

/**
 * @brief Main entry point – runs the state machine.
 */
int main(){
    MAIN_LOG("Start NoM App");
    PROGRAMSTATE retCode = PROGRAMSTATE::INIT;
    InitDebugConsole();
    while(true){
        
        switch(retCode){
            case (PROGRAMSTATE::INIT) :
            {
                retCode = initialize();
                break;
            }
            case (PROGRAMSTATE::EXECUTE) :
            {
                retCode = execute();
                break;
            }
            case (PROGRAMSTATE::FINALLY) :
            {
                retCode = finalize();
                break;
            }
            case (PROGRAMSTATE::SHUTDOWN) :
            {
                CloseDebugConsole();
                return 0;
            }
            default:
                break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(CYCLE_TIME));
    }
    //CloseDebugConsole();
}