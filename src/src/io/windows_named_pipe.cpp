#ifdef WINDOWS

#include "io/windows_named_pipe.h"

#include <iostream>

WindowsNamedPipe::WindowsNamedPipe(const std::string& path) : path(path), io() {
}

std::unique_ptr<IPCConnection> WindowsNamedPipe::accept() {
    HANDLE pipe_handle = ::CreateNamedPipe(
        path.c_str(),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 
        PIPE_UNLIMITED_INSTANCES, 
        0,                  
        0,               
        0, 
        nullptr       
    );


    if(pipe_handle == nullptr || pipe_handle == INVALID_HANDLE_VALUE){
        throw std::runtime_error((std:: string)"Failed to create pipe " + std::to_string(GetLastError()));
    }

    // wait for a client to connect

    // TODO can second argument be null?
    bool connection_status = ::ConnectNamedPipe(pipe_handle, nullptr);

    if (!connection_status) {
        throw std::runtime_error("Failure making connection on pipe");
    }

    asio::windows::stream_handle pipe(io, pipe_handle);


    return std::move(std::make_unique<WindowsNamedPipeConnection>(std::move(pipe)));
}

WindowsNamedPipeConnection::WindowsNamedPipeConnection(asio::windows::stream_handle pipe)
        : pipe(std::move(pipe)) {
}

void WindowsNamedPipeConnection::send(const std::string& message) {
    asio::write(pipe, asio::buffer(message + "\n"));
}

std::string WindowsNamedPipeConnection::receive() {
    asio::streambuf buffer;
    asio::read_until(pipe, buffer, '\n');
    return std::string(asio::buffers_begin(buffer.data()), asio::buffers_begin(buffer.data()) + buffer.size() -  2); //-2 to trim newline
}

#endif