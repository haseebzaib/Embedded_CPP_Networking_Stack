# CMakeList.txt : CMake project for NetworkingStack, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)



project ("NetworkingStack")


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Good practice for portability


# TODO: Add tests and install targets if needed.
# --- Find Npcap Dependency ---
# We need to find the Npcap SDK headers and libraries.
# CMake doesn't know about Npcap by default, so we help it.

# Find the path to the main header file, pcap.h
find_path(NPCAP_INCLUDE_DIR pcap.h
    HINTS
        "D:/Program Files/npcap-sdk-1.15/Include"
)

# Find the library files we need to link against
find_library(NPCAP_WPCAP_LIBRARY wpcap
    HINTS
        "D:/Program Files/npcap-sdk-1.15/Lib/x64"
)
find_library(NPCAP_PACKET_LIBRARY Packet
    HINTS
         "D:/Program Files/npcap-sdk-1.15/Lib/x64"
)

# Group the found libraries into a single variable
set(NPCAP_LIBRARIES ${NPCAP_WPCAP_LIBRARY} ${NPCAP_PACKET_LIBRARY})

include(FetchContent)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.1.1
)

FetchContent_MakeAvailable(fmt)

# Check if we found everything and fail gracefully if not.
if(NOT NPCAP_INCLUDE_DIR OR NOT NPCAP_LIBRARIES)
    message(FATAL_ERROR "Npcap SDK not found. Please install the Npcap SDK "
                        "from https://npcap.com/#sdk and ensure it's in a standard location, "
                        "or set NPCAP_INCLUDE_DIR and NPCAP_LIBRARIES in the CMake cache.")
endif()

message(STATUS "Found Npcap includes: ${NPCAP_INCLUDE_DIR}")
message(STATUS "Found Npcap libraries: ${NPCAP_LIBRARIES}")


# --- Define our Executable Target ---

# List all the source files that will be compiled into our executable.
# We will add more to this list as we create them.
set(SOURCES
    NetworkingStack.cpp
    hal/pc_pcap_hal.cpp     # Will be added soon
    hal/pc_timer_hal.cpp    # Will be added soon
    hal/pc_logging_hal.cpp
    net_stack/network_stack.cpp
    net_stack/arp_cache.cpp
)

add_executable(Networking ${SOURCES})


# --- Configure Target Properties (Includes and Links) ---

# Tell our 'Networking' target where to find header files.
target_include_directories(Networking PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} # Allows #include "protocols/ethernet.h"
    ${NPCAP_INCLUDE_DIR}        # Add Npcap's include directory
)

# Tell our 'Networking' target which libraries to link against.
target_link_libraries(Networking PRIVATE
    ${NPCAP_LIBRARIES}
    fmt::fmt
)

