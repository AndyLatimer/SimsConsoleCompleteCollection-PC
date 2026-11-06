@echo off
rem Runs one command inside the same toolchain build.bat sets up (VS 2022 C++ tools, the clang
rem that ships with Visual Studio, VS CMake and Ninja), without prompts. For scripts and agents:
rem   tools\devshell.bat cmake --preset release
rem   tools\devshell.bat cmake --build out\build\release
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (
    echo devshell: Visual Studio with the C++ workload was not found 1>&2
    exit /b 1
)
call "%VSDIR%\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 >nul
set "VSCMAKE=%VSDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake"
set "PATH=%VSCMAKE%\CMake\bin;%VSCMAKE%\Ninja;%VSDIR%\VC\Tools\Llvm\x64\bin;%PATH%"
if exist "%ProgramFiles%\LLVM\bin\clang++.exe" set "PATH=%ProgramFiles%\LLVM\bin;%PATH%"
cd /d "%~dp0.."
%*
exit /b %ERRORLEVEL%
