# build commands:
# cd C:\Users\yunch\Important\Project\debate\cppserver
# cmake -S . -B build
# cmake --build build -j


cmake_minimum_required(VERSION 3.20)
project(DebateServer LANGUAGES CXX)


# ---------------------------------
# Compiler and definitions
# ---------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# These replicate your -D flags
add_definitions(-D_WIN32_WINNT=0x0A00)
add_definitions(-DPROTOBUF_USE_DLLS)
add_definitions(-DWIN32_LEAN_AND_MEAN)
add_definitions(-DNOMINMAX)


# ---------------------------------
# Include directories (same as -I flags)
# ---------------------------------
include_directories(
    ${CMAKE_SOURCE_DIR}/src/server
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer
    ${CMAKE_SOURCE_DIR}/src/main
    ${CMAKE_SOURCE_DIR}/../src/gen/cpp
    ${CMAKE_SOURCE_DIR}/../third_party/vcpkg/installed/x64-mingw-dynamic/include
)


# ---------------------------------
# Source files (same as task args)
# ---------------------------------
set(SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/debate/main/EventHandler.cc
    ${CMAKE_SOURCE_DIR}/src/debate/DebateModerator.cpp
    ${CMAKE_SOURCE_DIR}/src/debate/DebateDatabaseHandler.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/pageGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/virtualRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/database/databaseCommunicator.cc
    ${CMAKE_SOURCE_DIR}/src/database/sqlite3.o
    ${CMAKE_SOURCE_DIR}/src/utils/pathUtils.cpp
    ${CMAKE_SOURCE_DIR}/src/debate/UserDatabaseHandler.cc
    ${CMAKE_SOURCE_DIR}/src/debate/main/events/enterDebate.cc
    ${CMAKE_SOURCE_DIR}/src/debate/main/events/goHome.cc
)


# Add all generated protobuf files (*.pb.cc)
file(GLOB PB_SRC "${CMAKE_SOURCE_DIR}/../src/gen/cpp/*.pb.cc")


# Combine sources
list(APPEND SRC_FILES ${PB_SRC})


# ---------------------------------
# Library search paths (-L flags)
# ---------------------------------
link_directories(
    ${CMAKE_SOURCE_DIR}/../third_party/vcpkg/installed/x64-mingw-dynamic/lib
    ${CMAKE_SOURCE_DIR}/../third_party/vcpkg/installed/x64-mingw-static/lib
)


# ---------------------------------
# Abseil RSP equivalent (load all Abseil libs dynamically)
# ---------------------------------
file(GLOB ABSL_LIBS
    "${CMAKE_SOURCE_DIR}/../third_party/vcpkg/installed/x64-mingw-dynamic/lib/libabsl_*.dll.a"
)


if(ABSL_LIBS)
    message(STATUS "✅ Found Abseil libs:")
    foreach(LIB ${ABSL_LIBS})
        message(STATUS "   ${LIB}")
    endforeach()
else()
    message(WARNING "⚠️  No Abseil libs found under x64-mingw-dynamic/lib")
endif()


# ---------------------------------
# Create executable
# ---------------------------------
add_executable(debate_server
    ${CMAKE_SOURCE_DIR}/src/server/main.cc
    ${SRC_FILES}   # all the other code files you already listed
)



# ---------------------------------
# Link libraries (in same order as your .rsp build)
# ---------------------------------
target_link_libraries(debate_server
    protobuf
    ${ABSL_LIBS}
    zlib
    ws2_32
    sqlite3
)


# ---------------------------------
# Output directory
# ---------------------------------
set_target_properties(debate_server PROPERTIES
    LINK_FLAGS "-mconsole"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)


# ---------------------------------
# Ensure MinGW runtime and vcpkg DLLs are found
# ---------------------------------
set(ENV{PATH} "C:\\mingw-w64\\mingw64\\bin;${CMAKE_SOURCE_DIR}/../third_party/vcpkg/installed/x64-mingw-dynamic/bin;$ENV{PATH}")
message(STATUS "PATH = $ENV{PATH}")



