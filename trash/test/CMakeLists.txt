cmake_minimum_required(VERSION 3.24)
project(debate_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to your generated protobuf C++ files
set(GEN_CPP_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../src/gen/cpp")

add_executable(server_min main_min.cpp)
target_include_directories(server_min PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
target_compile_definitions(server_min PRIVATE _WIN32_WINNT=0x0A00 WINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP CPPHTTPLIB_NO_FILESYSTEM)
if (WIN32)
  target_link_libraries(server_min PRIVATE ws2_32)
endif()


add_executable(server
  main.cpp
  "${GEN_CPP_DIR}/page.pb.cc"
  "${GEN_CPP_DIR}/event.pb.cc"
)


# Headers: generated folder + local (for httplib.h)
target_include_directories(server PRIVATE
  "${GEN_CPP_DIR}"
  "${CMAKE_CURRENT_LIST_DIR}"
)

# Windows defines to avoid CreateFile2 path, DLL proto, etc.
target_compile_definitions(server PRIVATE
  PROTOBUF_USE_DLLS
  _WIN32_WINNT=0x0A00
  WINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP
  # CPPHTTPLIB_NO_FILESYSTEM  # uncomment if you don't serve files
)

# Protobuf (from vcpkg). This should pull Abseil transitively.
find_package(Protobuf CONFIG REQUIRED)
target_link_libraries(server PRIVATE protobuf::libprotobuf)

# If your vcpkg version needs explicit Abseil, keep this block:
find_package(absl CONFIG QUIET)
if(absl_FOUND)
  target_link_libraries(server PRIVATE
    absl::log absl::strings absl::time absl::base absl::synchronization
  )
endif()

# Windows sockets
if(WIN32)
  target_link_libraries(server PRIVATE ws2_32)
endif()
