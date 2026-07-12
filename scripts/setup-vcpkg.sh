#!/bin/bash
# Setup vcpkg and install C++ dependencies for the debate server.
# Run from repo root (C:\Users\yunch\Important\Project\debate) in git-bash.
#
# Usage: bash scripts/setup-vcpkg.sh

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VCPKG_DIR="$REPO_ROOT/third_party/vcpkg"

# Step 1: Clone vcpkg if missing
if [ ! -d "$VCPKG_DIR" ]; then
  echo ">>> Cloning vcpkg..."
  git clone https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
fi

cd "$VCPKG_DIR"

# Step 2: Checkout version with protobuf 28.x (known good with MinGW)
# Use latest master which has the gcc15 musttail fix merged
echo ">>> Updating vcpkg..."
git fetch origin
git checkout origin/master

# Step 3: Bootstrap
if [ ! -f "vcpkg.exe" ]; then
  echo ">>> Bootstrapping vcpkg..."
  ./bootstrap-vcpkg.bat
fi

# Step 4: Copy GCC 15 musttail patch into vcpkg ports
echo ">>> Installing GCC 15 compatibility patch..."
cp "$REPO_ROOT/scripts/protobuf-fix-gcc15-musttail.patch" \
   "$VCPKG_DIR/ports/protobuf/fix-gcc15-musttail.patch"

# Step 5: Patch portfile to include our patch
PORTFILE="$VCPKG_DIR/ports/protobuf/portfile.cmake"
if ! grep -q "fix-gcc15-musttail.patch" "$PORTFILE"; then
  sed -i 's|fix-utf8-range.patch|fix-utf8-range.patch\n  fix-gcc15-musttail.patch|' "$PORTFILE"
  echo ">>> Patched portfile.cmake"
else
  echo ">>> Portfile already patched"
fi

# Step 6: Install packages
echo ">>> Installing protobuf:x64-mingw-static..."
./vcpkg install protobuf:x64-mingw-static

echo ">>> Installing remaining packages..."
./vcpkg install sqlite3:x64-mingw-static
./vcpkg install zlib:x64-mingw-static
./vcpkg install abseil:x64-mingw-static
./vcpkg install utf8proc:x64-mingw-static
./vcpkg install jsoncons:x64-mingw-static

# Step 7: Verify
echo ""
echo "=== Verification ==="
for lib in libprotobuf.a libsqlite3.a libzs.a; do
  if [ -f "$VCPKG_DIR/installed/x64-mingw-static/lib/$lib" ]; then
    echo "  OK: $lib"
  else
    echo "  MISSING: $lib"
  fi
done

if [ -f "$VCPKG_DIR/installed/x64-mingw-static/tools/protobuf/protoc.exe" ]; then
  echo "  OK: protoc.exe"
else
  echo "  MISSING: protoc.exe"
fi

echo ""
echo "=== Done ==="
echo "Build with: cd backend && cmake -S . -B build && cmake --build build -j"
