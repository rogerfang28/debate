@echo off
REM Bootstrap vcpkg and install all C++ dependencies for the debate server.
REM Run from repo root: scripts\setup-vcpkg.bat

set REPO_ROOT=%~dp0..
set VCPKG_DIR=%REPO_ROOT%\third_party\vcpkg

if not exist "%VCPKG_DIR%" (
    echo ^>^>^> Cloning vcpkg into third_party\vcpkg ...
    git clone https://github.com/microsoft/vcpkg.git "%VCPKG_DIR%"
)

cd /d "%VCPKG_DIR%"

if not exist "vcpkg.exe" (
    echo ^>^>^> Bootstrapping vcpkg ...
    call bootstrap-vcpkg.bat
)

echo ^>^>^> Installing dependencies ...
vcpkg install --triplet=x64-mingw-static

echo ^>^>^> Done. Build with:
echo     cd backend ^&^& cmake -S . -B build ^&^& cmake --build build -j
