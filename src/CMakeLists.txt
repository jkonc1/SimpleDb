cmake_minimum_required(VERSION 3.16)
project(Dumbatase)

find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
  message(STATUS "Found Clang-Tidy: ${CLANG_TIDY_EXE}")
  set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
else()
  message(WARNING "Clang-Tidy not found.")
endif()

if(WIN32)
  add_definitions(-DWINDOWS)
  message("Windows detected")
elseif(UNIX)
  add_definitions(-DUNIX)
  message("Unix detected")
else()
  message(FATAL_ERROR "Unsupported operating system.")
endif()

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

if(CODE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Building with code coverage flags")
        add_compile_options(--coverage -O0)
        add_link_options(--coverage)
    else()
        message(WARNING "Code coverage only supported with GCC or Clang")
    endif()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
list(FILTER sources EXCLUDE REGEX "src/main.cpp")

file(GLOB_RECURSE test_sources "tests/**/*.cpp")

include_directories(tests/include)

add_executable(Dumbatase ${sources} src/main.cpp)

add_executable(DumbataseTests ${sources} ${test_sources} tests/test_main.cpp)
set_target_properties(DumbataseTests PROPERTIES CXX_CLANG_TIDY "") # disable tidy
