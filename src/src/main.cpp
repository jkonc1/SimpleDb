#include "io/ipc_socket.h"
#include "jobs/job_queue.h"

#include "db/database.h"

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
#include <unistd.h>

constexpr int POLL_PERIOD_MS = 500;

constexpr std::string DATABASE_PATH = "/tmp/db";

void process_connection(std::unique_ptr<IPCConnection>&& connection){
    connection->send("Test");
    std::string response = connection->receive();
    std::cout << "received " << response << std::endl;
    connection->send("Hello " + response);
    
    sleep(10);
}

int main(){
    Database db(DATABASE_PATH);
    JobQueue job_queue;
    
    std::unique_ptr<IPCSocket> socket = std::make_unique<SocketInterface>(SOCKET_PATH);
    
    while(true){
        auto connection = socket->accept();

        while (connection == NULL) {
            connection = socket->accept();
            std::this_thread::sleep_for(std::chrono::milliseconds(POLL_PERIOD_MS));
        }
        
        job_queue.add_job([connection = std::move(connection)]() mutable {
            process_connection(std::move(connection));
        });
        break;
        
    }
    
    job_queue.finish();
}