# Test CMakeLists.txt for Debate Tool — tests branch
# Build: cd test && cmake -S . -B build -G Ninja && cmake --build build -j
# Test:  cd build && ctest --output-on-failure -j4
#
# Targets:
#   debate_tests       — unit tests (12 tests: DebateEventTest + PropagationTest)
#   debate_simulation  — comprehensive simulation test (1 test: DebateSimulationTest)
#
# Run separately:
#   ./build/debate_tests.exe           — all unit tests
#   ./build/debate_simulation.exe      — simulation test only
#   ctest -R UnitTests                 — unit tests via ctest
#   ctest -R Simulation                — simulation via ctest

cmake_minimum_required(VERSION 3.20)
project(DebateTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- vcpkg paths (repo root is parent of test/) ---
set(REPO_ROOT "${CMAKE_SOURCE_DIR}/..")
set(VCPKG_ROOT "${REPO_ROOT}/third_party/vcpkg" CACHE PATH "vcpkg root")
set(VCPKG_INSTALLED "${VCPKG_ROOT}/installed/x64-mingw-static")

# --- Compiler flags ---
add_compile_options(-Wno-deprecated -Wno-unused-variable -Wno-sign-compare)
add_definitions(-D_WIN32_WINNT=0x0A00 -DWIN32_LEAN_AND_MEAN -DNOMINMAX)

# --- Git commit hash (used by StepView/FullDebatePageGenerator for page title) ---
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${REPO_ROOT}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions(-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\")
# Static linking to avoid DLL dependencies
add_link_options(-static-libgcc -static-libstdc++)

# --- Include directories ---
include_directories(
    ${REPO_ROOT}/src/gen/cpp
    ${REPO_ROOT}/backend/src
    ${REPO_ROOT}/backend/src/database/sqlite
    ${REPO_ROOT}/backend/src/database/debate
    ${REPO_ROOT}/backend/src/utils
    ${VCPKG_INSTALLED}/include
)

# --- Generated protobuf sources ---
file(GLOB PB_SRC "${REPO_ROOT}/src/gen/cpp/*.pb.cc")

# --- Abseil libraries (glob all, link with --start-group for circular deps) ---
file(GLOB ABSL_LIBS "${VCPKG_INSTALLED}/lib/libabsl_*.a")

# --- Database layer source files (the code under test) ---
set(DB_SOURCES
    ${REPO_ROOT}/backend/src/database/sqlite/Database.cc
    ${REPO_ROOT}/backend/src/database/sqlite/databaseCommunicator.cc
    ${REPO_ROOT}/backend/src/database/debate/UserDatabase.cc
    ${REPO_ROOT}/backend/src/database/debate/DebateDatabase.cc
    ${REPO_ROOT}/backend/src/database/debate/StatementDatabase.cc
    ${REPO_ROOT}/backend/src/database/debate/DebateMembersDatabase.cc
    ${REPO_ROOT}/backend/src/database/debate/LinkDatabase.cc
    ${REPO_ROOT}/backend/src/database/debate/DatabaseWrapper.cc
    ${REPO_ROOT}/backend/src/utils/DebateWrapper.cc
    ${REPO_ROOT}/backend/src/utils/Log.cc
    ${REPO_ROOT}/backend/src/utils/pathUtils.cpp
)

# --- Event handler source files (the code under test) ---
set(HANDLER_SOURCES
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/DebateHandler/DebateHandler.cc
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/MoveUserHandler/MoveUserHandler.cc
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/AddClaimHandler/AddClaimHandler.cc
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/ChallengeHandler/ChallengeHandler.cc
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/DeleteClaimHandler/DeleteClaimHandler.cc
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/EditClaimHandler/EditClaimHandler.cc
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/ConnectClaimsHandler/ConnectClaimsHandler.cc
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/ModifyClaimHandler/ModifyClaimHandler.cc
)

# --- Moderator + response generators (needed by scenario runner) ---
set(MODERATOR_SOURCES
    ${REPO_ROOT}/backend/src/debateModerator/DebateModerator.cc
    ${REPO_ROOT}/backend/src/debateModerator/buildResponse/homePageResponse/HomePageResponseGenerator.cc
    ${REPO_ROOT}/backend/src/debateModerator/buildResponse/debatePageResponse/DebatePageResponseGenerator.cc
    ${REPO_ROOT}/backend/src/debateModerator/buildResponse/loginPageResponse/LoginPageResponseGenerator.cc
)

# --- Virtual renderer source files (step view / pbtxt generation) ---
set(RENDERER_SOURCES
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/ComponentGenerator.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/debatePage/StepView/StepView.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/debatePage/FullDebateView/FullDebatePageInfoParser.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/debatePage/FullDebateView/FullDebatePageGenerator.cc
    ${REPO_ROOT}/backend/src/database/virtualrenderer/VRUserDatabase.cc
    ${REPO_ROOT}/backend/src/debateModerator/buildResponse/debatePageResponse/BuildCollection.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/utils/UserNameResolver.cc
)

# --- Add handler include paths ---
include_directories(
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/DebateHandler
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/MoveUserHandler
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/AddClaimHandler
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/ChallengeHandler
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/DeleteClaimHandler
    ${REPO_ROOT}/backend/src/debateModerator
)

# --- Virtual renderer/include paths ---
include_directories(
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/debatePage/StepView
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/debatePage/FullDebateView
    ${REPO_ROOT}/backend/src/database/virtualrenderer
    ${REPO_ROOT}/backend/src/debateModerator/buildResponse/debatePageResponse
    ${REPO_ROOT}/backend/src/debateModerator/buildResponse/homePageResponse
    ${REPO_ROOT}/backend/src/debateModerator/buildResponse/loginPageResponse
    ${REPO_ROOT}/backend/src/virtualRenderer/utils
)

# --- Shared link libraries ---
set(COMMON_LIBS
    ${VCPKG_INSTALLED}/lib/libgtest.a
    ${VCPKG_INSTALLED}/lib/manual-link/libgtest_main.a
    ${VCPKG_INSTALLED}/lib/libgmock.a
    ${VCPKG_INSTALLED}/lib/manual-link/libgmock_main.a
    -Wl,--start-group
    ${VCPKG_INSTALLED}/lib/libprotobuf.a
    ${VCPKG_INSTALLED}/lib/libutf8_range.a
    ${VCPKG_INSTALLED}/lib/libutf8_validity.a
    ${ABSL_LIBS}
    dbghelp
    -Wl,--end-group
    ${VCPKG_INSTALLED}/lib/libsqlite3.a
    ${VCPKG_INSTALLED}/lib/libzs.a
    ws2_32
)

# ============================================================
# UNIT TESTS — fast, isolated, no persistent state
# ============================================================
set(UNIT_TEST_SOURCES
    ${CMAKE_SOURCE_DIR}/tests/test_debate_create.cc
    ${CMAKE_SOURCE_DIR}/tests/test_challenge_propagation.cc
)

add_executable(debate_tests
    ${UNIT_TEST_SOURCES}
    ${DB_SOURCES}
    ${HANDLER_SOURCES}
    ${PB_SRC}
)
target_link_libraries(debate_tests ${COMMON_LIBS})

# ============================================================
# SIMULATION TEST — comprehensive, persistent DB, verbose output
# ============================================================
set(SIMULATION_TEST_SOURCES
    ${CMAKE_SOURCE_DIR}/tests/test_debate_simulation.cc
    ${CMAKE_SOURCE_DIR}/tests/test_scenario_runner.cc
    ${CMAKE_SOURCE_DIR}/tests/test_step_view_layout.cc
)

add_executable(debate_simulation
    ${SIMULATION_TEST_SOURCES}
    ${DB_SOURCES}
    ${HANDLER_SOURCES}
    ${MODERATOR_SOURCES}
    ${RENDERER_SOURCES}
    ${PB_SRC}
)
target_link_libraries(debate_simulation ${COMMON_LIBS})

# ============================================================
# Copy scenario .pbtxt files to build directory
# ============================================
file(GLOB SCENARIO_FILES "${CMAKE_SOURCE_DIR}/scenarios/*.pbtxt")
file(COPY ${SCENARIO_FILES} DESTINATION "${CMAKE_BINARY_DIR}/scenarios")
message(STATUS "Copied scenario files: ${SCENARIO_FILES}")

# ============================================================
# CTest integration
# ============================================================
enable_testing()
add_test(NAME UnitTests COMMAND debate_tests)
add_test(NAME Simulation COMMAND debate_simulation)
