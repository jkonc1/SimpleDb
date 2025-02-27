#include "io/ipc_socket.h"

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

constexpr int POLL_PERIOD_MS = 500;

int main(){
    std::unique_ptr<IPCSocket> socket = std::make_unique<SocketInterface>(SOCKET_PATH);
    
    while(true){
        auto connection = socket->accept();

        while (connection == NULL) {
            connection = socket->accept();
            std::this_thread::sleep_for(std::chrono::milliseconds(POLL_PERIOD_MS));
        }
        
        connection->send("Test");
        std::string response = connection->receive();
        std::cout << "received " << response << std::endl;
        connection->send("Hello " + response);
    }
}