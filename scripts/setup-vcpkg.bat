@echo off
REM Bootstrap vcpkg and install all C++ dependencies for the debate server.
REM Run from repo root: scripts\setup-vcpkg.bat

set REPO_ROOT=%~dp0..
set VCPKG_DIR=%REPO_ROOT%\third_party\vcpkg

if not exist "%VCPKG_DIR%" (
    echo ^>^>^> Cloning vcpkg into third_party\vcpkg ...
    git clone https://github.com/microsoft/vcpkg.git "%VCPKG_DIR%"
)

if not exist "%VCPKG_DIR%\vcpkg.exe" (
    echo ^>^>^> Bootstrapping vcpkg ...
    pushd "%VCPKG_DIR%"
    call bootstrap-vcpkg.bat
    popd
)

echo ^>^>^> Installing dependencies from vcpkg.json ...
set VCPKG_DEFAULT_TRIPLET=x64-mingw-static
"%VCPKG_DIR%\vcpkg" install --x-manifest-root="%REPO_ROOT%"

echo ^>^>^> Done. Build with:
echo     cd backend ^&^& cmake -S . -B build ^&^& cmake --build build -j
