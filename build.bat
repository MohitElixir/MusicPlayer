@echo off
REM Rebuild MusicPlayer as a fully self-contained exe.
REM Requires g++ (MinGW-w64 / MSYS2) on PATH.

g++ -std=c++17 -O2 -static -static-libgcc -static-libstdc++ -o MusicPlayer.exe main.cpp MusicManager.cpp Song.cpp UIManager.cpp -lwinmm

if %ERRORLEVEL% EQU 0 (
    echo Build succeeded: MusicPlayer.exe
) else (
    echo Build failed.
)
