#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// Unity Build - Include all CPP files in dependency order
#include "song_management/MusicManager.cpp"
#include "ui/Screen.cpp"
#include "ui/UIUtils.cpp"
#include "ui/MenuSection.cpp"
#include "ui/LibrarySection.cpp"
#include "ui/SearchSection.cpp"
#include "ui/FavoritesSection.cpp"
#include "ui/PlaylistsSection.cpp"
#include "ui/NowPlayingSection.cpp"

#include <iostream>

using namespace std;

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001); // Enable UTF-8 output
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        SetConsoleMode(hOut, dwMode | 0x0004); // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    }
#endif

    MusicManager manager;
    manager.refreshFromFolder("Music");
    manager.loadData();

    Screen currentScreen = Screen::MENU;
    Screen lastScreen = Screen::EXIT;

    while (currentScreen != Screen::EXIT) {
        if (currentScreen != lastScreen) {
            std::cout << "\x1B[2J\x1B[H" << std::flush;
            lastScreen = currentScreen;
        }
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

    manager.saveData();

    cout << "\nThanks for using Music Player. Goodbye!\n";
    return 0;
}
