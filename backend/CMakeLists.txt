# build commands:
# cd C:\Users\yunch\Important\Project\debate\backend
# cmake -S . -B build
# cmake --build build -j


cmake_minimum_required(VERSION 3.20)
project(DebateServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF)
set(Protobuf_USE_STATIC_LIBS ON)
set(ZLIB_USE_STATIC_LIBS ON)

add_compile_options(-Wno-deprecated -Wno-unused-variable -Wno-sign-compare)

# -D flags
add_definitions(-D_WIN32_WINNT=0x0A00)
add_definitions(-DWIN32_LEAN_AND_MEAN)
add_definitions(-DNOMINMAX)


# -I flags
include_directories(
    ${CMAKE_SOURCE_DIR}/src/server
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer
    ${CMAKE_SOURCE_DIR}/src/main
    ${CMAKE_SOURCE_DIR}/../src/gen/cpp
    ${CMAKE_SOURCE_DIR}/../third_party/vcpkg/installed/x64-mingw-static/include
)


set(SRC_FILES

    ${CMAKE_SOURCE_DIR}/src/utils/Log.cc
    # ${CMAKE_SOURCE_DIR}/src/debate/main/EventHandler.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/DebateModerator.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/virtualRenderer.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/MiddleendRequestHandler.cc
    
    # event handlers
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/AddDebate/AddDebateHandler.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/ClearDebates/ClearDebatesHandler.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/DeleteDebate/DeleteDebateHandler.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/EnterDebate/EnterDebateHandler.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/GoHome/GoHomeHandler.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/AddClaimUnderClaim/AddClaimUnderClaim.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/GoToClaim/GoToClaim.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/GoToParentClaim/GoToParentClaim.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/DeleteCurrentStatement/DeleteCurrentStatement.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/OpenAddChildClaim/OpenAddChildClaim.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/CloseAddChildClaim/CloseAddChildClaim.cc
    

    # build response
    ${CMAKE_SOURCE_DIR}/src/debateModerator/buildResponse/debatePageResponse/DebatePageResponseGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/buildResponse/homePageResponse/HomePageResponseGenerator.cc

    # database communicators and utils
    ${CMAKE_SOURCE_DIR}/src/database/sqlite/databaseCommunicator.cc
    ${CMAKE_SOURCE_DIR}/src/utils/pathUtils.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/DebateWrapper.cc
    ${CMAKE_SOURCE_DIR}/src/database/sqlite/Database.cc
    ${CMAKE_SOURCE_DIR}/src/database/handlers/UserDatabase.cc
    ${CMAKE_SOURCE_DIR}/src/database/handlers/DebateDatabase.cc
    ${CMAKE_SOURCE_DIR}/src/database/handlers/StatementDatabase.cc
    ${CMAKE_SOURCE_DIR}/src/database/handlers/DebateMembersDatabase.cc

    # layout generators
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/LayoutGenerator/LayoutGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/LayoutGenerator/ComponentGenerator.cc
    # page generators
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/LayoutGenerator/pages/homePage/HomePageGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/LayoutGenerator/pages/loginPage/LoginPageGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/LayoutGenerator/pages/debatePage/DebatePageGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/LayoutGenerator/pages/errorPage/ErrorPageGenerator.cc

    # event parsers
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/ClientMessageHandler/ClientMessageParser.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/ClientMessageHandler/HomePageEventParser/HomePageEventParser.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/ClientMessageHandler/ErrorPageEventParser/ErrorPageEventParser.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/ClientMessageHandler/DebatePageEventParser/DebatePageEventParser.cc
)


# generated protobuf files for c++
file(GLOB PB_SRC "${CMAKE_SOURCE_DIR}/../src/gen/cpp/*.pb.cc")
list(APPEND SRC_FILES ${PB_SRC})


# -L flags
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/../third_party/vcpkg/installed/x64-mingw-static")
link_directories(
    # ${CMAKE_SOURCE_DIR}/../third_party/vcpkg/installed/x64-mingw-dynamic/lib
    ${CMAKE_SOURCE_DIR}/../third_party/vcpkg/installed/x64-mingw-static/lib
)

# ABSL libraries
file(GLOB ABSL_LIBS
    "${CMAKE_SOURCE_DIR}/../third_party/vcpkg/installed/x64-mingw-static/lib/libabsl_*.a"
)

if(ABSL_LIBS)
    list(LENGTH ABSL_LIBS ABSL_COUNT)
    message(STATUS "✅ Found ${ABSL_COUNT} Abseil libs")
else()
    message(FATAL_ERROR "⚠️  No Abseil libs found")
endif()


# UTF8 library
file(GLOB UTF8_LIB
    "${CMAKE_SOURCE_DIR}/../third_party/vcpkg/installed/x64-mingw-static/lib/libutf8*.a"
)

if(NOT UTF8_LIB)
    message(FATAL_ERROR "No utf8_* static library found (libutf8*.a) in x64-mingw-static/lib")
endif()


add_executable(debate_server
    ${CMAKE_SOURCE_DIR}/src/server/main.cc
    ${SRC_FILES}
)

target_link_libraries(debate_server
    zlib
    sqlite3
    ws2_32
    # abseil_dll
    -Wl,--start-group
    dbghelp
    protobuf
    ${UTF8_LIB}
    ${ABSL_LIBS}
    -Wl,--end-group
)

set_target_properties(debate_server PROPERTIES
    LINK_FLAGS "-mconsole"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)