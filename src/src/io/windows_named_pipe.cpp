#ifdef WINDOWS

#include "io/windows_named_pipe.h"

#include <iostream>

WindowsNamedPipe::WindowsNamedPipe(const std::string& path) : path(path), io() {
}

void WindowsNamedPipe::start_accepting(std::function<void(std::unique_ptr<IPCConnection>&&)> callback) {
    HANDLE pipe_handle = ::CreateNamedPipeA(
        path.c_str(),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        0, 0, 0, nullptr
    );

    if (pipe_handle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("CreateNamedPipe failed: " + std::to_string(GetLastError()));
    }

    auto new_pipe = std::make_shared<asio::windows::stream_handle>(io, pipe_handle);

    auto overlapped = std::make_shared<OVERLAPPED>();
    
    ::memset(overlapped.get(), 0, sizeof(OVERLAPPED));

    ConnectNamedPipe(pipe_handle, overlapped.get());
    
    if (GetLastError() == ERROR_IO_PENDING) {
        new_pipe->async_wait(asio::windows::stream_handle::wait_read, [this, new_pipe, callback](const asio::error_code& ec) {
            if (!ec) {
                auto connection = std::make_unique<WindowsNamedPipeConnection>(std::move(*new_pipe));
                callback(std::move(connection));
            } else {
                std::cerr << "ConnectNamedPipe failed async: " << ec.message() << std::endl;
            }

            start_accepting(callback);  // continue accepting
        });
    } else if (GetLastError() == ERROR_PIPE_CONNECTED) {
        auto connection = std::make_unique<WindowsNamedPipeConnection>(std::move(*new_pipe));
        callback(std::move(connection));
        start_accepting(callback);
    } else {
        std::cerr << "ConnectNamedPipe failed: " << err << std::endl;
        start_accepting(callback);  // try again
    }
}

void WindowsNamedPipe::listen(std::function<void(std::unique_ptr<IPCConnection>&&)>) {
    start_accepting(callback);
    
    io.run();
} 

void WindowsNamedPipe::stop(){
    io.stop();
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