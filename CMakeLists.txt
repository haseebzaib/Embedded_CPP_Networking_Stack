# CMakeLists.txt — Linux-only build for NetworkingStack

cmake_minimum_required(VERSION 3.16)
project(NetworkingStack LANGUAGES CXX)

# C++ standard & warnings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dependencies
include(FetchContent)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 10.1.1
)
FetchContent_MakeAvailable(fmt)

# Sources
set(SOURCES
  NetworkingStack.cpp
  hal/pc_linux_hal.cpp
  hal/pc_timer_hal.cpp
  hal/pc_logging_hal.cpp
  net_stack/network_stack.cpp
  net_stack/arp_cache.cpp
)

add_executable(Networking ${SOURCES})

# Includes
target_include_directories(Networking PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compile options & features
target_compile_features(Networking PRIVATE cxx_std_20)
target_compile_options(Networking PRIVATE -Wall -Wextra -Wconversion)

# Links
target_link_libraries(Networking PRIVATE fmt::fmt)

# Helpful note for raw sockets
message(STATUS "Run with: sudo ./Networking  OR  grant caps:")
message(STATUS "  sudo setcap cap_net_raw,cap_net_admin=eip ${CMAKE_BINARY_DIR}/Networking")
