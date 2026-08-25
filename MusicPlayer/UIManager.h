#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "MusicManager.h"


using namespace std;

enum class Screen {
    MENU,
    LIBRARY,
    SEARCH,
    NOW_PLAYING,
    FAVORITES,
    PLAYLISTS,
    EXIT
};

class UIManager {
private:
    MusicManager& manager;
    Screen currentScreen;
    bool forceAudioRestart;

    void playSystemAudio(const Song* current);
    void drawHeader(const string& title) const;
    void drawDivider() const;
    void drawFooter() const;
    void showMenu();
    void showLibrary();
    void showSearch();
    void showNowPlaying();
    void showFavorites();
    void showPlaylists();

    static void clearScreen();
    static void pause();
    static int readIntChoice();

public:
    explicit UIManager(MusicManager& manager);
    void run();
};

#endif // UI_MANAGER_H
