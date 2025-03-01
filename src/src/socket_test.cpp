#include "io/ipc_socket.h"
#include <cmath>

#ifdef UNIX

#include "io/asio_uds_socket.h"
using SocketInterface = AsioUDSSocket;

const std::string SOCKET_PATH = "/tmp/db_socket";

#elif WINDOWS

#include "io/windows_named_pipe.h"
using SocketInterface = WindowsNamedPipe;

const std::string SOCKET_PATH = "\\\\.\\pipe\\db_pipe";

#endif

#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>

void callback(std::unique_ptr<IPCConnection>&& connection){
    connection->send("Test");
    std::string response = connection->receive();
    std::cout << "received " << response << std::endl;
    connection->send("Hello " + response);
}

std::function<void()> sigint_handler;

void handle_sigint(int) {
    sigint_handler();
}

int main(){
    std::unique_ptr<IPCSocket> socket = std::make_unique<SocketInterface>(SOCKET_PATH);
    
    sigint_handler = [&socket] {
        socket->stop();
    };
    
    ::signal(SIGINT, handle_sigint);
    
    socket->listen(callback);
    
    std::cout << "Exiting" << std::endl;
}