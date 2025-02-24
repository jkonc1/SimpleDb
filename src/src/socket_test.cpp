#include "io/ipc_socket.h"

#ifdef UNIX

#include "io/asio_uds_socket.h"
using SocketInterface = AsioUDSSocket;

#elif WINDOWS

#include "io/windows_socket.h"
using SocketInterface = WindowsSocket;

#endif

const std::string SOCKET_PATH = "/tmp/db_socket";

int main(){
    std::unique_ptr<SocketInterface> socket = std::make_unique<SocketInterface>(SOCKET_PATH);
    
    while(true){
        auto connection = socket->accept();
        
        connection->send("Test");
        std::string response = connection->receive();
        connection->send("Hello " + response);
    }
}