#pragma once
#include "../song_management/MusicManager.cpp"
#include "Screen.cpp"
#include "UIUtils.cpp"
#include <iostream>
#include <string>

using namespace std;

Screen showSearch(MusicManager& manager) {
    UIUtils::printBoxTop("◉  SEARCH SONGS");
    UIUtils::printBoxBottom();
    
    cout << "\n  ▸ Enter search query (or '0' to go back): ";
    string query;
    cin.ignore();
    getline(cin, query);

    if (query == "0" || query.empty()) return Screen::MENU;

    vector<int> results = manager.searchAll(query);
    
    UIUtils::printBoxTop("SEARCH RESULTS");
    if (results.empty()) {
        UIUtils::printBoxLine("  No matches found.");
        UIUtils::printBoxBottom();
        UIUtils::pause();
        return Screen::MENU;
    }

    for (size_t i = 0; i < results.size(); ++i) {
        string line = "  [" + to_string(i + 1) + "]    " + manager.getSongAt(results[i])->getTitle() + " - " + manager.getSongAt(results[i])->getArtist();
        UIUtils::printBoxLine(line);
    }
    UIUtils::printBoxBottom();

    cout << "\n  ▸ Song # to play, or 0 to go back: ";
    int choice = UIUtils::readIntChoice();

    if (choice > 0 && choice <= static_cast<int>(results.size())) {
        if (!manager.play(results[choice - 1])) {
            cout << "  Error: File missing or invalid.\n";
            UIUtils::pause();
        } else {
            UIUtils::playSystemAudio(manager.getNowPlaying());
            return Screen::NOW_PLAYING;
        }
    }
    return Screen::MENU;
}
