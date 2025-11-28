# Build commands:
# cd C:\Users\yunch\Important\Project\debate
# Remove-Item -Recurse -Force build
# cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc
# cmake --build build -j

cmake_minimum_required(VERSION 3.20)
project(DebateProject LANGUAGES CXX)

# ---------------------------------
# Compiler and definitions
# ---------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MINGW)
    add_compile_options(-Wno-deprecated -Wno-unused-variable -Wno-sign-compare)
endif()

add_definitions(-D_WIN32_WINNT=0x0A00)
add_definitions(-DPROTOBUF_USE_DLLS)
add_definitions(-DWIN32_LEAN_AND_MEAN)
add_definitions(-DNOMINMAX)

# ---------------------------------
# Include directories
# ---------------------------------
include_directories(
    # Middleend includes
    ${CMAKE_SOURCE_DIR}/middleend/src/BackendCommunicator
    ${CMAKE_SOURCE_DIR}/middleend/src/FrontendCommunicator/server
    ${CMAKE_SOURCE_DIR}/middleend/src/VirtualRenderer
    ${CMAKE_SOURCE_DIR}/middleend/src/VirtualRenderer/layoutGenerator
    ${CMAKE_SOURCE_DIR}/middleend/src/VirtualRenderer/ClientMessageHandler
    
    # Backend includes
    ${CMAKE_SOURCE_DIR}/backend/src/server
    ${CMAKE_SOURCE_DIR}/backend/src/database/handlers
    ${CMAKE_SOURCE_DIR}/backend/src/database/sqlite
    ${CMAKE_SOURCE_DIR}/backend/src/debate
    ${CMAKE_SOURCE_DIR}/backend/src/debate/virtual-renderer-communicator
    ${CMAKE_SOURCE_DIR}/backend/src/debate/event-handlers
    ${CMAKE_SOURCE_DIR}/backend/src/utils
    
    # Generated protobuf files
    ${CMAKE_SOURCE_DIR}/src/gen/cpp
    
    # Third party
    ${CMAKE_SOURCE_DIR}/third_party/vcpkg/installed/x64-mingw-dynamic/include
)

# ---------------------------------
# Source files
# ---------------------------------
set(MIDDLEEND_SRC
    ${CMAKE_SOURCE_DIR}/middleend/src/BackendCommunicator/BackendCommunicator.cc
    ${CMAKE_SOURCE_DIR}/middleend/src/FrontendCommunicator/server/MiddleendRequestHandler.cc
    ${CMAKE_SOURCE_DIR}/middleend/src/VirtualRenderer/virtualRenderer.cpp
    ${CMAKE_SOURCE_DIR}/middleend/src/VirtualRenderer/layoutGenerator/LayoutGenerator.cc
    ${CMAKE_SOURCE_DIR}/middleend/src/VirtualRenderer/layoutGenerator/pageGenerator.cpp
    ${CMAKE_SOURCE_DIR}/middleend/src/VirtualRenderer/ClientMessageHandler/ClientMessageParser.cc
)

set(BACKEND_SRC
    ${CMAKE_SOURCE_DIR}/backend/src/debate/DebateModerator.cpp
    ${CMAKE_SOURCE_DIR}/backend/src/debate/virtual-renderer-communicator/BuildPayload.cc
    ${CMAKE_SOURCE_DIR}/backend/src/database/handlers/DebateDatabaseHandler.cc
    ${CMAKE_SOURCE_DIR}/backend/src/database/handlers/UserDatabaseHandler.cc
    ${CMAKE_SOURCE_DIR}/backend/src/database/sqlite/DatabaseCommunicator.cc
    ${CMAKE_SOURCE_DIR}/backend/src/database/sqlite/sqlite3.o
    ${CMAKE_SOURCE_DIR}/backend/src/utils/pathUtils.cpp
    ${CMAKE_SOURCE_DIR}/backend/src/debate/event-handlers/AddDebate/AddDebateHandler.cc
    ${CMAKE_SOURCE_DIR}/backend/src/debate/event-handlers/ClearDebates/ClearDebatesHandler.cc
    ${CMAKE_SOURCE_DIR}/backend/src/debate/event-handlers/DeleteDebate/DeleteDebateHandler.cc
    ${CMAKE_SOURCE_DIR}/backend/src/debate/event-handlers/EnterDebate/EnterDebateHandler.cc
    ${CMAKE_SOURCE_DIR}/backend/src/debate/event-handlers/GoHome/GoHomeHandler.cc
)

# Add all generated protobuf files
file(GLOB PB_SRC "${CMAKE_SOURCE_DIR}/src/gen/cpp/*.pb.cc")

# ---------------------------------
# Library search paths
# ---------------------------------
link_directories(
    ${CMAKE_SOURCE_DIR}/third_party/vcpkg/installed/x64-mingw-dynamic/lib
    ${CMAKE_SOURCE_DIR}/third_party/vcpkg/installed/x64-mingw-static/lib
)

# ---------------------------------
# Abseil libraries
# ---------------------------------
file(GLOB ABSL_LIBS
    "${CMAKE_SOURCE_DIR}/third_party/vcpkg/installed/x64-mingw-dynamic/lib/libabsl_*.dll.a"
)

# ---------------------------------
# Create VirtualRendererServer executable (combines middleend + backend)
# ---------------------------------
add_executable(virtual_renderer_server
    ${CMAKE_SOURCE_DIR}/middleend/src/FrontendCommunicator/server/VirtualRendererServer.cc
    ${MIDDLEEND_SRC}
    ${BACKEND_SRC}
    ${PB_SRC}
)

# ---------------------------------
# Link libraries
# ---------------------------------
target_link_libraries(virtual_renderer_server
    protobuf
    ${ABSL_LIBS}
    zlib
    ws2_32
    sqlite3
)

# ---------------------------------
# Output directory
# ---------------------------------
set_target_properties(virtual_renderer_server PROPERTIES
    LINK_FLAGS "-mconsole"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
