/*
 * FavoritesSection.cpp
 * ====================
 * Shows all songs marked as favorites and allows
 * playing them directly from this list.
 */

#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "Screen.cpp"
#include "UIUtils.cpp"
#include "../song_management/MusicManager.cpp"

using namespace std;

Screen showFavorites(MusicManager& manager) {
    UIUtils::printBoxTop("♥  FAVORITES");

    vector<int> favs = manager.getFavorites();
    if (favs.empty()) {
        UIUtils::printBoxLine("  You have no favorite songs yet.");
    } else {
        for (size_t i = 0; i < favs.size(); ++i) {
            string line = "  [" + to_string(i + 1) + "]    "
                          + manager.getSongAt(favs[i])->getTitle() + " - "
                          + manager.getSongAt(favs[i])->getArtist();
            UIUtils::printBoxLine(line);
        }
    }

    UIUtils::printBoxBottom();

    cout << "\n  ▸ Song # to play, or 0 to go back: ";
    int choice = UIUtils::readIntChoice();

    if (choice > 0 && choice <= static_cast<int>(favs.size())) {
        if (!manager.play(favs[choice - 1])) {
            cout << "  Error: File missing or invalid.\n";
            UIUtils::pause();
        } else {
            manager.startAudio(manager.getNowPlaying()->getFilePath());
            return Screen::NOW_PLAYING;
        }
    }
    return Screen::MENU;
}
