#ifndef WINDOWS_NAMED_PIPE_H
#define WINDOWS_NAMED_PIPE_H

#include "io/ipc_socket.h"

#include <asio.hpp>
#include <windows.h> //fuj

class WindowsNamedPipeConnection : public IPCConnection{
public:
    WindowsNamedPipeConnection(asio::windows::stream_handle stream);
    
    virtual void send(const std::string& message) override;
    
    virtual std::string receive() override;
private:
    asio::windows::stream_handle pipe;
};

class WindowsNamedPipe : public IPCSocket {
public:
    WindowsNamedPipe(const std::string& path);
    
    void listen(std::function<void(std::unique_ptr<IPCConnection>&&)>) override;
    
    void stop() override;
private:
    void start_accepting(std::function<void(std::unique_ptr<IPCConnection>&&)>);
    
    std::string path;
    asio::io_context io;
    std::atomic<bool> listening = false;
};

#endif