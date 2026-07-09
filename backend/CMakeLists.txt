cmake_minimum_required(VERSION 3.20)
project(DebateServer LANGUAGES CXX)

# Embed git commit hash (optional but nice)
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wno-deprecated -Wno-unused-variable -Wno-sign-compare)

# Needed for GoogleJWTVerifier's httplib::SSLClient fallback path.
add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)

# ----- Include directories -----
include_directories(
    ${CMAKE_SOURCE_DIR}/src/server
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer
    ${CMAKE_SOURCE_DIR}/src/main
    ${CMAKE_SOURCE_DIR}/../src/gen/cpp
    /usr/include
    /usr/include/absl
)

# ----- Source files -----
set(SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/utils/Log.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/DebateModerator.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/virtualRenderer.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/MiddleendRequestHandler.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/DebateHandler/DebateHandler.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/MoveUserHandler/MoveUserHandler.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/AddClaimHandler/AddClaimHandler.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/DeleteClaimHandler/DeleteClaimHandler.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/EditClaimHandler/EditClaimHandler.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/ModifyClaimHandler/ModifyClaimHandler.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/ConnectClaimsHandler/ConnectClaimsHandler.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/event-handlers/ChallengeHandler/ChallengeHandler.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/ClientMessageHandler/ClientMessageParser.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/ClientMessageHandler/HomePageEventParser/HomePageEventParser.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/ClientMessageHandler/ErrorPageEventParser/ErrorPageEventParser.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/ClientMessageHandler/DebatePageEventParser/DebatePageEventParser.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/ClientMessageHandler/LoginPageEventParser/LoginPageEventParser.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/buildResponse/debatePageResponse/DebatePageResponseGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/buildResponse/debatePageResponse/BuildCollection.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/buildResponse/homePageResponse/HomePageResponseGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/debateModerator/buildResponse/loginPageResponse/LoginPageResponseGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/database/sqlite/databaseCommunicator.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/utils/parseCookie.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/utils/UserNameResolver.cc
    ${CMAKE_SOURCE_DIR}/src/utils/pathUtils.cpp
     ${CMAKE_SOURCE_DIR}/src/utils/DebateWrapper.cc
     ${CMAKE_SOURCE_DIR}/src/utils/GoogleJWTVerifier.cc
     ${CMAKE_SOURCE_DIR}/src/utils/ClaimParserLLM.cc
    ${CMAKE_SOURCE_DIR}/src/database/sqlite/Database.cc
    ${CMAKE_SOURCE_DIR}/src/database/debate/UserDatabase.cc
    ${CMAKE_SOURCE_DIR}/src/database/debate/DebateDatabase.cc
    ${CMAKE_SOURCE_DIR}/src/database/debate/StatementDatabase.cc
    ${CMAKE_SOURCE_DIR}/src/database/debate/DebateMembersDatabase.cc
    ${CMAKE_SOURCE_DIR}/src/database/debate/LinkDatabase.cc
    ${CMAKE_SOURCE_DIR}/src/database/debate/DatabaseWrapper.cc
    ${CMAKE_SOURCE_DIR}/src/database/virtualrenderer/VRUserDatabase.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/LayoutGenerator/LayoutGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/LayoutGenerator/ComponentGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/LayoutGenerator/pages/homePage/HomePageGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/LayoutGenerator/pages/homePage/HomePageInfoParser.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/LayoutGenerator/pages/loginPage/LoginPageGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/LayoutGenerator/pages/debatePage/SingleStatementView/SingleDebatePageGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/LayoutGenerator/pages/debatePage/SingleStatementView/SingleDebatePageInfoParser.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/LayoutGenerator/pages/debatePage/FullDebateView/FullDebatePageGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/LayoutGenerator/pages/debatePage/FullDebateView/FullDebatePageInfoParser.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/LayoutGenerator/pages/debatePage/StepView/StepView.cc
    ${CMAKE_SOURCE_DIR}/src/virtualRenderer/LayoutGenerator/pages/errorPage/ErrorPageGenerator.cc
)

# Generated protobuf sources
file(GLOB PB_SRC "${CMAKE_SOURCE_DIR}/../src/gen/cpp/*.pb.cc")
list(APPEND SRC_FILES ${PB_SRC})

# ----- Find packages (vcpkg toolchain provides these) -----
find_package(Protobuf REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)
find_package(absl REQUIRED)
find_package(utf8proc REQUIRED)

add_executable(debate_server
    ${CMAKE_SOURCE_DIR}/src/server/main.cc
    ${SRC_FILES}
)

# Link libraries
# vcpkg provides: protobuf::libprotobuf, absl::base, absl::strings, absl::synchronization, utf8proc::utf8proc
# System provides: sqlite3, ssl, crypto, z, pthread
target_link_libraries(debate_server
    protobuf::libprotobuf
    ZLIB::ZLIB
    Threads::Threads
    sqlite3
    ssl
    crypto
    absl::base
    absl::strings
    absl::synchronization
    utf8proc::utf8proc
)

set_target_properties(debate_server PROPERTIES
    
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
