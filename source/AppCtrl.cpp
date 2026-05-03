#include "AppCtrl.h"
#include <thread>
#include <chrono>

int main(){

    while(true){
        NodeManagament::getInstall()->start();
        IOHandler::getInstall()->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(CYCLE_TIME));
    }

    


}