#ifndef ASIO_UDS_SOCKET_H
#define ASIO_UDS_SOCKET_H

#include <asio.hpp>
#include <string>
#include <memory>
#include "io/ipc_socket.h"

class AsioUDSSocket : public IPCSocket{
public:
    AsioUDSSocket(const std::string& path);
    ~AsioUDSSocket();
    
    void listen(std::function<void(std::unique_ptr<IPCConnection>&&)> callback) override;
    void stop() override;
private:
    void start_accepting(std::function<void(std::unique_ptr<IPCConnection>&&)> callback);
    std::string path;
    asio::io_context io;
    asio::local::stream_protocol::acceptor acceptor;
};

class AsioUDSConnection : public IPCConnection{
public:
    AsioUDSConnection(asio::local::stream_protocol::socket socket);
    void send(const std::string& message) override;
    std::string receive() override;
private:
    asio::local::stream_protocol::socket socket;
};

#endif