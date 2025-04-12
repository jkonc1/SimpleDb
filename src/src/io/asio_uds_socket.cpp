#ifdef UNIX

#include "io/asio_uds_socket.h"
#include <iostream>

AsioUDSSocket::AsioUDSSocket(const std::string& path) : path(path), io(), acceptor(io, asio::local::stream_protocol::endpoint(path)) {
}

void AsioUDSSocket::start_accepting(std::function<void(std::unique_ptr<IPCConnection>&&)> callback) {
    acceptor.async_accept([this, callback](const asio::error_code& ec, asio::local::stream_protocol::socket socket){
        if(!ec){
            auto connection = std::make_unique<AsioUDSConnection>(std::move(socket));
            callback(std::move(connection));
        }
        else{
            std::cerr << "Error accepting connection: " << ec.message() << std::endl;
        }
        
        start_accepting(callback);
    });
}

void AsioUDSSocket::listen(std::function<void(std::unique_ptr<IPCConnection>&&)> callback){
    start_accepting(callback);
    
    io.run();
}

void AsioUDSSocket::stop(){
    std::cout << "Stopping socket" << std::endl;
    io.stop();
}

AsioUDSSocket::~AsioUDSSocket() {
    acceptor.close();
    std::cerr << "Socket closed" << std::endl;
    std::remove(path.c_str());
}

AsioUDSConnection::AsioUDSConnection(asio::local::stream_protocol::socket&& socket)
        : socket(std::move(socket)) {
}

void AsioUDSConnection::send(const std::string& message) {
    asio::write(socket, asio::buffer(message));
}

std::string AsioUDSConnection::receive() {
    asio::streambuf buffer;
    asio::read_until(socket, buffer, '\n');
    return std::string(asio::buffers_begin(buffer.data()), asio::buffers_begin(buffer.data()) + buffer.size());
}

#endif