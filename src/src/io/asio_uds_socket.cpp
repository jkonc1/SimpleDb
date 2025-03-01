#ifdef UNIX

#include "io/asio_uds_socket.h"
#include <iostream>

AsioUDSSocket::AsioUDSSocket(const std::string& path) : path(path), io(), acceptor(io, asio::local::stream_protocol::endpoint(path)) {
}

std::unique_ptr<IPCConnection> AsioUDSSocket::accept(){
    asio::local::stream_protocol::socket socket(io);
    
    try{
        acceptor.accept(socket);
    }
    catch(const asio::system_error& e){
        if(e.code() == asio::error::would_block){
            return nullptr;
        }
        
        throw;
    }
    
    return std::move(std::make_unique<AsioUDSConnection>(std::move(socket)));
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