/*
 * main.cpp
 * ========
 * Entry point / driver file for the Music Player application.
 *
 * Since this project uses .cpp files only (no .h headers), we use
 * a "unity build" approach: main.cpp #includes all other .cpp files
 * in the correct dependency order, and then the build command only
 * compiles this single file. This avoids duplicate symbol errors
 * that would occur if each file were compiled separately.
 *
 * Build command:
 *   g++ -std=c++17 -O2 main.cpp -o AudioPlayer.exe
 *
 * Course: Object-Oriented Programming in C++
 * Semester: 2nd Semester BSc.IT
 */

// =====================================================================
//  Include all project files in dependency order:
//  1. MusicManager.cpp  - Core classes (Song, Playlist, MusicManager)
//  2. Screen.cpp        - Screen enum for navigation
//  3. UIUtils.cpp       - Terminal UI utility functions
//  4. *Section.cpp      - Individual screen handler functions
// =====================================================================
// Unity Build - Include all CPP files in dependency order
// clang-format off
#include "song_management/MusicManager.cpp"
#include "ui/Screen.cpp"
#include "ui/UIUtils.cpp"
#include "ui/MenuSection.cpp"
#include "ui/LibrarySection.cpp"
#include "ui/SearchSection.cpp"
#include "ui/FavoritesSection.cpp"
#include "ui/PlaylistsSection.cpp"
#include "ui/NowPlayingSection.cpp"
// clang-format on

#include <iostream>

using namespace std;


// =====================================================================
//  Main Function — Application Entry Point
// =====================================================================

int main() {
    // Set up console for UTF-8 and ANSI escape codes
    UIUtils::setupConsole();

    // Create the MusicManager object and initialize it
    MusicManager manager;
    manager.refreshFromFolder("Music");
    manager.loadData();

    // Start on the main menu
    Screen currentScreen = Screen::MENU;
    Screen lastScreen = Screen::EXIT;

    // Main loop — keeps running until the user selects Exit
    while (currentScreen != Screen::EXIT) {
        // Clear screen when transitioning between views
        if (currentScreen != lastScreen) {
            UIUtils::fullClear();
            lastScreen = currentScreen;
        }

        // Route to the correct screen handler function
        switch (currentScreen) {
            case Screen::MENU:
                currentScreen = showMenu(manager);
                break;
            case Screen::LIBRARY:
                currentScreen = showLibrary(manager);
                break;
            case Screen::SEARCH:
                currentScreen = showSearch(manager);
                break;
            case Screen::FAVORITES:
                currentScreen = showFavorites(manager);
                break;
            case Screen::PLAYLISTS:
                currentScreen = showPlaylists(manager);
                break;
            case Screen::NOW_PLAYING:
                currentScreen = showNowPlaying(manager);
                break;
            default:
                currentScreen = Screen::EXIT;
                break;
        }
    }

    // Save user data before exiting
    manager.saveData();

    // Stop any audio that's still playing (uses encapsulated method)
    manager.stopAudio();

    cout << "\nThanks for using Music Player. Goodbye!\n";
    return 0;
}

