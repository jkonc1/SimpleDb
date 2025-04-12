#include "io/ipc_socket.h"
#include "jobs/job_queue.h"
#include "helper/logger.h"
#include "helper/sigint.h"
#include "db/database_manager.h"

#ifdef UNIX

#include "io/asio_uds_socket.h"
using SocketInterface = AsioUDSSocket;

#elif WINDOWS

#include "io/windows_named_pipe.h"
using SocketInterface = WindowsNamedPipe;

#endif

#include <iostream>

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
        return;
    }
}

void show_usage() noexcept {
    std::cerr << "Usage: Dumbatase <database_path> <socket_path>" << std::endl;
}

int main(int argc, char* argv[]){
    if(argc != 3){
        show_usage();
        return 1;
    }
    
    std::string db_path = argv[1];
    std::string socket_path = argv[2];
    
    DatabaseManager db(db_path);
    
    try{
        db.load();
    }
    catch(const std::exception& e){
        logger::log("Failed to load database: " + std::string(e.what()));
        return 1;
    }
    
    JobQueue job_queue;
    
    std::unique_ptr<IPCSocket> socket;
    
    try{
        socket = std::make_unique<SocketInterface>(socket_path);
    }
    catch(const std::exception& e){
        std::cerr << "Error creating socket: " << e.what() << std::endl;
        return 1;
    }
    
    auto callback = [&job_queue, &db = db.get()](std::unique_ptr<IPCConnection>&& connection){
        job_queue.add_job(
            [connection = std::move(connection), &db]() mutable noexcept {
                process_connection(std::move(connection), db);
            }
        );
    };
    
    set_sigint_handler([&socket]{
        logger::log("Received SIGINT");
        socket->stop();
    });
    
    try{
    socket->listen(callback);
    }
    catch(const std::exception& e){
        logger::log("Failed to bind to socket: " + std::string(e.what()));
        return 1;
    }

    logger::log("Waiting for running jobs to finish");
    job_queue.finish();
    logger::log("Exiting");
}
