#include "io/ipc_socket.h"
#include "jobs/job_queue.h"
#include "helper/logger.h"

#include "db/database_manager.h"
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
#include <unistd.h>

std::function<void()> sigint_handler;

void handle_sigint(int) {
    sigint_handler();
}

constexpr std::string DATABASE_PATH = "/tmp/db";

void process_connection(std::unique_ptr<IPCConnection>&& connection, Database& db) noexcept {
    std::string query;
    
    try{
        query = connection->receive();
    }
    catch(const std::exception& e){
        logger::log("Failed to read query");
        return;
    }
    
    std::string response = db.process_query(query);
    
    try{
        connection->send(response);
    }
    catch(const std::exception& e){
        logger::log("Failed to send response");
    }
}

int main(){
    DatabaseManager db(DATABASE_PATH);
    db.load();
    JobQueue job_queue;
    
    std::unique_ptr<IPCSocket> socket = std::make_unique<SocketInterface>(SOCKET_PATH);
    
    auto callback = [&job_queue, &db = db.get()](std::unique_ptr<IPCConnection>&& connection){
        job_queue.add_job(
            [connection = std::move(connection), &db]() mutable {
                process_connection(std::move(connection), db);
            }
        );
    };
    
    job_queue.finish();
    
    sigint_handler = [&socket] {
        socket->stop();
    };
    
    ::signal(SIGINT, handle_sigint);
    
    socket->listen(callback);
    
    std::cout << "Exiting" << std::endl;
}