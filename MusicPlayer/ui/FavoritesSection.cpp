#pragma once
#include "../song_management/MusicManager.cpp"
#include "Screen.cpp"
#include "UIUtils.cpp"
#include <iostream>
#include <string>

using namespace std;

Screen showFavorites(MusicManager& manager) {
    UIUtils::printBoxTop("♥  FAVORITES");

    vector<int> favs = manager.getFavorites();
    if (favs.empty()) {
        UIUtils::printBoxLine("  You have no favorite songs yet.");
    } else {
        for (size_t i = 0; i < favs.size(); ++i) {
            string line = "  [" + to_string(i + 1) + "]    " + manager.getSongAt(favs[i])->getTitle() + " - " + manager.getSongAt(favs[i])->getArtist();
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
            UIUtils::playSystemAudio(manager.getNowPlaying());
            return Screen::NOW_PLAYING;
        }
    }
    return Screen::MENU;
}
