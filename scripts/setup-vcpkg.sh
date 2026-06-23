#!/usr/bin/env bash
# Bootstrap vcpkg and install all C++ dependencies for the debate server.
# Run from repo root: bash scripts/setup-vcpkg.sh

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VCPKG_DIR="$REPO_ROOT/third_party/vcpkg"

if [ ! -d "$VCPKG_DIR" ]; then
  echo ">>> Cloning vcpkg into third_party/vcpkg ..."
  git clone https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
fi

cd "$VCPKG_DIR"

if [ ! -f "vcpkg.exe" ] && [ ! -f "vcpkg" ]; then
  echo ">>> Bootstrapping vcpkg ..."
  if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
    ./bootstrap-vcpkg.bat
  else
    ./bootstrap-vcpkg.sh
  fi
fi

echo ">>> Installing dependencies ..."
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
  ./vcpkg install --triplet=x64-mingw-static
else
  ./vcpkg install
fi

echo ">>> Done. Build with:"
echo "    cd backend && cmake -S . -B build && cmake --build build -j"
