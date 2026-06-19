# Test CMakeLists.txt for Debate Tool — tests branch
# Build: cd test && cmake -S . -B build -G Ninja && cmake --build build -j
# Test:  cd build && ctest --output-on-failure -j4

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

# --- Test sources ---
set(TEST_SOURCES
    ${CMAKE_SOURCE_DIR}/tests/test_debate_create.cc
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
    ${VCPKG_INSTALLED}/lib/libzlib.a
    ws2_32
)

# --- Test executable ---
add_executable(debate_tests
    ${TEST_SOURCES}
    ${DB_SOURCES}
    ${PB_SRC}
)
target_link_libraries(debate_tests ${COMMON_LIBS})

# --- Enable ctest ---
enable_testing()
add_test(NAME DebateTests COMMAND debate_tests)
