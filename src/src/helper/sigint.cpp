#include "helper/sigint.h"
#include "helper/logger.h"

#include <csignal>

static std::function<void()> sigint_handler; //NOLINT

void handle_sigint(int) { //NOLINT
    static bool already_called = false;
    
    if(already_called){
        logger::log("Received SIGINT for the second time, exiting forcefully.");
        logger::log("Database won't be saved.");
        
        std::exit(1);
    }
    already_called = true;
    
    sigint_handler();
}

void set_sigint_handler(std::function<void()> handler){
    sigint_handler = std::move(handler);
    
    ::signal(SIGINT, handle_sigint);
}
