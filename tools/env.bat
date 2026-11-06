@echo off
rem Toolchain environment for this port: LLVM/Clang + MSVC + Ninja.
rem Prepends LLVM to PATH and imports the MSVC amd64 environment into the
rem caller's shell (no setlocal, so PATH changes persist).

rem vswhere supports Community, Professional and Build Tools in custom install paths.
set "SIMS1_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "SIMS1_VSDIR="
if not exist "%SIMS1_VSWHERE%" goto :novs
for /f "usebackq tokens=*" %%i in (`"%SIMS1_VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "SIMS1_VSDIR=%%i"
if not defined SIMS1_VSDIR goto :novs
rem Some VS command scripts call vswhere by name.
set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
call "%SIMS1_VSDIR%\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 >nul
if errorlevel 1 exit /b 1
set "SIMS1_CMAKE=%SIMS1_VSDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake"
set "PATH=%SIMS1_CMAKE%\CMake\bin;%SIMS1_CMAKE%\Ninja;%SIMS1_VSDIR%\VC\Tools\Llvm\x64\bin;%PATH%"
if exist "%ProgramFiles%\LLVM\bin\clang++.exe" set "PATH=%ProgramFiles%\LLVM\bin;%PATH%"
where clang++ >nul 2>nul || exit /b 1
where cmake >nul 2>nul || exit /b 1
where ninja >nul 2>nul || exit /b 1

echo.
echo === Toolchain versions ===
clang++ --version
ninja --version
cmake --version
exit /b 0

:novs
echo ERROR: Visual Studio with the C++ workload was not found. 1>&2
exit /b 1
