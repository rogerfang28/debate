# Debate Tool — Tests

## Prerequisites

- **MinGW-w64** (GCC 15+) on `PATH`
- **Ninja** build system
- **vcpkg** at `../third_party/vcpkg` with the following packages installed:
  - `gtest:x64-mingw-static`
  - `protobuf:x64-mingw-static`
  - `sqlite3:x64-mingw-static`
  - `abseil:x64-mingw-static`
  - `utf8-range:x64-mingw-static`

## Quick Start

```bash
# 1. Configure (only needed once, or after CMakeLists.txt changes)
cmake -S . -B build -G Ninja

# 2. Build
cmake --build build -j4

# 3. Run all tests
cd build
./debate_tests.exe
```

Or run via CTest:

```bash
cd build
ctest --output-on-failure -j4
```

## Incremental Builds

If CMake re-runs configure every time (very slow), use Ninja directly:

```bash
cd build
ninja -j4
```

## Tests

| Test | What it checks |
|------|---------------|
| `CreateDebate_ProducesRootClaim` | `initNewDebate` creates root claim with topic text |
| `AddChildClaim_UnderRoot` | Child claim linked to root via parent, `findChildrenIds` |
| `FullFlow_CreateDebateAddChild_VerifyInDB` | End-to-end: debate → child → verify in statements & links tables |
| `AddMultipleChildren_VerifyCount` | 3 children, all linked correctly |
| `ChildClaim_HasCorrectConnectionType` | `findChildrenInfo` returns correct (id, sentence) pairs |

## Notes

- Each test uses a **fresh temp SQLite database** — no side effects on real data
- Temp files (`*.sqlite3`, `*-wal`, `*-shm`) are cleaned up automatically on teardown
- Build output: `build/debate_tests.exe`
