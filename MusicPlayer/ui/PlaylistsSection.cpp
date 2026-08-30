/*
 * PlaylistsSection.cpp
 * ====================
 * Manages user playlists — create new ones, view existing ones,
 * and play songs from within a playlist.
 */

#pragma once
#include <iostream>
#include <string>
#include "Screen.cpp"
#include "UIUtils.cpp"
#include "../song_management/MusicManager.cpp"

using namespace std;

Screen showPlaylists(MusicManager& manager) {
    UIUtils::printBoxTop("★  PLAYLISTS");

    if (manager.getPlaylists().empty()) {
        UIUtils::printBoxLine("  No playlists found.");
    } else {
        for (size_t i = 0; i < manager.getPlaylists().size(); ++i) {
            string line = "  [" + to_string(i + 1) + "]    "
                          + manager.getPlaylists()[i].name
                          + " (" + to_string(manager.getPlaylists()[i].songIndices.size())
                          + " songs)";
            UIUtils::printBoxLine(line);
        }
    }

    UIUtils::printBoxLine("");
    UIUtils::printBoxLine("  Options:");
    UIUtils::printBoxLine("  [1] Open Playlist");
    UIUtils::printBoxLine("  [2] Create New Playlist");
    UIUtils::printBoxLine("  [0] Back");

    UIUtils::printBoxBottom();

    cout << "\n  ▸ Select option: ";
    int choice = UIUtils::readIntChoice();

    if (choice == 1) {
        if (manager.getPlaylists().empty()) {
            cout << "  No playlists to open.\n";
            UIUtils::pause();
            return Screen::PLAYLISTS;
        }
        cout << "  ▸ Enter playlist #: ";
        int plChoice = UIUtils::readIntChoice();
        if (plChoice > 0 && plChoice <= static_cast<int>(manager.getPlaylists().size())) {
            const Playlist& pl = manager.getPlaylists()[plChoice - 1];

            UIUtils::printBoxTop("★  " + pl.name);
            if (pl.songIndices.empty()) {
                UIUtils::printBoxLine("  Playlist is empty.");
            } else {
                for (size_t i = 0; i < pl.songIndices.size(); ++i) {
                    const Song* s = manager.getSongAt(pl.songIndices[i]);
                    string line = "  [" + to_string(i + 1) + "]    "
                                  + s->getTitle() + " - " + s->getArtist();
                    UIUtils::printBoxLine(line);
                }
            }
            UIUtils::printBoxBottom();

            cout << "\n  ▸ Song # to play, or 0 to go back: ";
            int songChoice = UIUtils::readIntChoice();
            if (songChoice > 0 && songChoice <= static_cast<int>(pl.songIndices.size())) {
                if (!manager.play(pl.songIndices[songChoice - 1])) {
                    cout << "  Error: File missing or invalid.\n";
                    UIUtils::pause();
                } else {
                    manager.startAudio(manager.getNowPlaying()->getFilePath());
                    return Screen::NOW_PLAYING;
                }
            }
        }
    } else if (choice == 2) {
        cout << "  ▸ Enter new playlist name: ";
        string name;
        cin.ignore();
        getline(cin, name);
        if (!name.empty()) {
            manager.addPlaylist(name);
            cout << "  Playlist created!\n";
            UIUtils::pause();
        }
    } else if (choice == 0) {
        return Screen::MENU;
    }

    return Screen::PLAYLISTS;
}
