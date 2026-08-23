#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <string>
#include "MusicManager.h"

// ============================================================
// Member 3: UI & Navigation
// Owns screen state and drives the main application loop.
// ============================================================
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
    MusicManager& manager;   // reference to the core engine (Member 2's code)
    Screen currentScreen;
    int audioPlayingIndex;   // tracks which song MCI is currently playing (-1 = none)

    // Drawing helpers
    void drawHeader(const std::string& title) const;
    void drawFooter() const;
    void drawDivider() const;

    // Screen handlers
    void showMenu();
    void showLibrary();
    void showSearch();
    void showNowPlaying();
    void showFavorites();
    void showPlaylists();

    // Input helpers
    static void pause();
    static int readIntChoice();

public:
    explicit UIManager(MusicManager& manager);

    // Runs the main application loop until the user exits
    void run();
};

#endif // UI_MANAGER_H
