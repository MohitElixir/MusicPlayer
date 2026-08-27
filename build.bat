@echo off
REM Rebuild MusicPlayer as a fully self-contained exe.
REM Requires g++ (MinGW-w64 / MSYS2) on PATH.

g++ -std=c++17 -O2 -static -static-libgcc -static-libstdc++ -o AudioPlayer.exe MusicPlayer\main.cpp -lwinmm

if %ERRORLEVEL% EQU 0 (
    echo Build succeeded: AudioPlayer.exe
) else (
    echo Build failed.
)
