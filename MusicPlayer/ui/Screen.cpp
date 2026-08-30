/*
 * Screen.cpp
 * ==========
 * Defines the Screen enum used for navigating between
 * different sections of the music player UI.
 *
 * Each value corresponds to a different view that the
 * user can interact with in the application.
 */

#pragma once

enum class Screen {
    MENU,
    LIBRARY,
    SEARCH,
    NOW_PLAYING,
    FAVORITES,
    PLAYLISTS,
    EXIT
};
