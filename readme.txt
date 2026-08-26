Music Player

## Prerequisites
- Windows OS (as it links against the `winmm` library for audio playback).
- MSYS2 / MinGW-w64 with `g++.exe` installed (C++17 supported), or a compatible compiler.
- Visual Studio Code (optional, but recommended as build tasks are provided).

## How to Compile and Run

### Method 1: Using Visual Studio Code (Recommended)
1. Open the project folder in Visual Studio Code.
2. Press `Ctrl + Shift + B` to run the default build task. This uses the configuration in `.vscode/tasks.json` to compile the project automatically.
3. Once the build succeeds, an executable named `AudioPlayer.exe` will be generated in the root directory.
4. Open the VS Code integrated terminal (`Ctrl + \``) and run the program:
   .\AudioPlayer.exe

### Method 2: Using the Command Line
If you prefer compiling manually via the command prompt or terminal (ensure your compiler is in your system PATH), navigate to the root directory of this project and run the following command:

g++ -g -std=c++17 MusicPlayer\*.cpp -o AudioPlayer.exe -lwinmm

Then, run the generated executable:
.\AudioPlayer.exe

If g++ is installed but not correctly configured, open MSYS2 UCRT64 from the Start Menu and run:
cd /h/MusicPlayer/MusicPlayer
g++ -g -std=c++17 *.cpp -o ../AudioPlayer.exe -lwinmm
../AudioPlayer.exe