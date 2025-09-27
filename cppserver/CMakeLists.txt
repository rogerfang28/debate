cmake_minimum_required(VERSION 3.20)
project(debate LANGUAGES CXX)

# ---- C++ standard ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Paths that mirror your tasks.json layout ----
# CMakeLists.txt is in debate/cppserver/
set(PROTO_DIR "${CMAKE_SOURCE_DIR}/../src/proto")
set(GEN_CPP_DIR "${CMAKE_SOURCE_DIR}/../src/gen/cpp")

# ---- Find Protobuf (and protoc) via vcpkg toolchain ----
find_package(Protobuf REQUIRED) # provides protobuf::libprotobuf and Protobuf_PROTOC_EXECUTABLE

# ---- Generate *.pb.cc / *.pb.h like your 'Generate Protobuf C++' task ----
file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")

# Make sure the output directory exists (matches your tasks.json)
file(MAKE_DIRECTORY "${GEN_CPP_DIR}")

set(PB_SRCS "")
set(PB_HDRS "")
foreach(proto ${PROTO_FILES})
  get_filename_component(fname "${proto}" NAME_WE)
  set(out_cc "${GEN_CPP_DIR}/${fname}.pb.cc")
  set(out_h  "${GEN_CPP_DIR}/${fname}.pb.h")
  add_custom_command(
    OUTPUT "${out_cc}" "${out_h}"
    COMMAND "${Protobuf_PROTOC_EXECUTABLE}"
            -I "${PROTO_DIR}"
            --cpp_out="${GEN_CPP_DIR}"
            "${proto}"
    DEPENDS "${proto}"
    COMMENT "protoc: ${fname}.proto -> ${fname}.pb.cc/.pb.h"
    VERBATIM
  )
  list(APPEND PB_SRCS "${out_cc}")
  list(APPEND PB_HDRS "${out_h}")
endforeach()

# ---- Your sources ----
add_executable(debate
  src/server/test.cpp
  ${PB_SRCS}
)

# ---- Includes (so #include "page.pb.h" works cleanly) ----
target_include_directories(debate PRIVATE
  "${GEN_CPP_DIR}"
  "${CMAKE_SOURCE_DIR}/src/server"
  "${CMAKE_SOURCE_DIR}/src/server/main"
)

# ---- Compile-time defines (match your tasks.json) ----
target_compile_definitions(debate PRIVATE
  WIN32_LEAN_AND_MEAN
  NOMINMAX
  _WIN32_WINNT=0x0A00
  PROTOBUF_USE_DLLS        # because you're using the *dynamic* vcpkg triplet
)

# ---- Link libraries (protobuf + ws2_32; zlib optional) ----
target_link_libraries(debate PRIVATE protobuf::libprotobuf ws2_32)

# Your tasks.json passed "@absl.rsp" (all absl_*.dll.a). Do the equivalent:
# Prefer proper CMake package; fall back to globbing like your RSP if needed.
find_package(absl CONFIG QUIET)
if (absl_FOUND)
  # Link a broad set that covers typical protobuf deps.
  target_link_libraries(debate PRIVATE
    absl::strings
    absl::time
    absl::base
    absl::hash
    absl::cord
  )
else()
  # Fallback: replicate your "absl.rsp" behavior by globbing every Abseil lib
  # under vcpkg's installed lib folder.
  if (DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_LIB_DIR "${CMAKE_SOURCE_DIR}/../third_party/vcpkg/installed/${VCPKG_TARGET_TRIPLET}/lib")
    file(GLOB ABSL_LIBS "${VCPKG_LIB_DIR}/libabsl_*.dll.a")
    if (ABSL_LIBS)
      target_link_libraries(debate PRIVATE ${ABSL_LIBS})
    endif()
  endif()
endif()

# Optional: zlib (your tasks.json used -lzlib; the portable way is via find_package)
find_package(ZLIB QUIET)
if (ZLIB_FOUND)
  target_link_libraries(debate PRIVATE ZLIB::ZLIB)
endif()

# ---- Put the exe in a tidy place ----
set_target_properties(debate PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ---- Post-build: copy DLLs next to the exe so it runs without PATH tweaks ----
if (WIN32)
  # 1) protobuf DLL from vcpkg
  add_custom_command(TARGET debate POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:protobuf::libprotobuf>
            $<TARGET_FILE_DIR:debate>
  )

  # 2) MinGW runtime DLLs (adjust if your MinGW path differs)
  set(MINGW_BIN "C:/mingw-w64/mingw64/bin")
  foreach(dll IN ITEMS libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll)
    if (EXISTS "${MINGW_BIN}/${dll}")
      add_custom_command(TARGET debate POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MINGW_BIN}/${dll}"
                $<TARGET_FILE_DIR:debate>
      )
    endif()
  endforeach()
endif()

# ---- Make the generated files part of the build graph ----
# (So building 'debate' first triggers protoc if needed)
add_custom_target(generate_protos DEPENDS ${PB_SRCS} ${PB_HDRS})
add_dependencies(debate generate_protos)
