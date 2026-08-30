@echo off
REM ============================================================
REM  Build script for Music Player
REM  Compiles main.cpp which #includes all other .cpp files
REM  (unity build — no .h header files in this project).
REM  Requires: g++ (MinGW-w64 / MSYS2) on PATH
REM ============================================================

g++ -std=c++17 -O2 -static -static-libgcc -static-libstdc++ ^
    -o AudioPlayer.exe MusicPlayer\main.cpp -lwinmm

if %ERRORLEVEL% EQU 0 (
    echo Build succeeded: AudioPlayer.exe
) else (
    echo Build failed.
)