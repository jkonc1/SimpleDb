cmake_minimum_required(VERSION 3.16)
project(Dumbatase)

if(WIN32)
  add_definitions(-DWINDOWS)
  message("Windows detected")
elseif(UNIX)
  add_definitions(-DUNIX)
  message("Unix detected")
else()
  message(FATAL_ERROR "Unsupported operating system.")
endif()

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-32-0 # Replace with the latest version
)
FetchContent_MakeAvailable(asio)

include_directories(include)
include_directories(${asio_SOURCE_DIR}/asio/include)

file(GLOB_RECURSE sources "src/*.cpp")

add_executable(Dumbatase ${sources})
