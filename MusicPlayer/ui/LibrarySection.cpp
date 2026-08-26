#pragma once
#include "../song_management/MusicManager.cpp"
#include "Screen.cpp"
#include "UIUtils.cpp"
#include <iostream>
#include <string>

using namespace std;

Screen showLibrary(MusicManager& manager) {
    manager.refreshFromFolder("Music");
    UIUtils::printBoxTop("♫  SONG LIBRARY");

    const auto& songs = manager.getLibrary();
    if (songs.empty()) {
        UIUtils::printBoxLine("  (Library is empty - add .mp3 files)");
    } else {
        for (size_t i = 0; i < songs.size(); ++i) {
            string line = "  [" + to_string(i + 1) + "]    " + songs[i].getTitle() + " - " + songs[i].getArtist();
            UIUtils::printBoxLine(line);
        }
    }

    UIUtils::printBoxBottom();

    cout << "\n  ▸ Song # to play, or 0 to go back: ";
    int choice = UIUtils::readIntChoice();

    if (choice == 0 || choice == -1) {
        return Screen::MENU;
    }

    if (choice >= 1 && choice <= static_cast<int>(songs.size())) {
        if (!manager.play(choice - 1)) {
            cout << "  Error: File missing or invalid.\n";
            UIUtils::pause();
        } else {
            UIUtils::playSystemAudio(manager.getNowPlaying());
            return Screen::NOW_PLAYING;
        }
    } else {
        cout << "  Invalid choice!\n";
        UIUtils::pause();
    }
    return Screen::LIBRARY;
}
