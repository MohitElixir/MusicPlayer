/*
 * MenuSection.cpp
 * ===============
 * Displays the main menu of the music player.
 * Shows different options depending on whether a song is playing.
 *
 * This file is #included by main.cpp (unity build), so it has
 * access to MusicManager, Screen, and UIUtils automatically.
 */

#pragma once
#include <iostream>
#include <string>
#include "Screen.cpp"
#include "UIUtils.cpp"
#include "../song_management/MusicManager.cpp"

using namespace std;

// Displays the main menu and returns the next screen to navigate to
Screen showMenu(MusicManager& manager) {
    manager.refreshFromFolder("Music");

    UIUtils::clearScreen();
    bool playing = (manager.getNowPlayingIndex() != -1);

    if (playing) {
        string title = manager.getNowPlaying()->getTitle();
        string artist = manager.getNowPlaying()->getArtist();

        string center_title = "♫ NOW PLAYING: " + title + " ♫";
        string center_artist = "by " + artist;

        int pad1 = (62 - UIUtils::visualLength(center_title)) / 2;
        int pad2 = (62 - UIUtils::visualLength(center_artist)) / 2;
        if (pad1 < 0) pad1 = 0;
        if (pad2 < 0) pad2 = 0;

        string spaces1(pad1, ' ');
        string spaces1_r(62 - UIUtils::visualLength(center_title) - pad1, ' ');
        string spaces2(pad2, ' ');
        string spaces2_r(62 - UIUtils::visualLength(center_artist) - pad2, ' ');

        cout << "\n"
             << "  ╔════════════════════════════════════════════════════════════════╗\n"
             << "  ║                     ♫  MUSIC PLAYER  ♫                     ║\n"
             << "  ╚════════════════════════════════════════════════════════════════╝\n"
             << "  ║                                                                ║\n"
             << "  ║" << spaces1 << center_title << spaces1_r << "║\n"
             << "  ║" << spaces2 << center_artist << spaces2_r << "║\n"
             << "  ║                                                                ║\n"
             << "  ║   [1]  ♫ View Library                                          ║\n"
             << "  ║   [2]  ▶ Now Playing                                           ║\n"
             << "  ║   [3]  ◉ Search Songs                                          ║\n"
             << "  ║   [4]  ♥ Favorites                                             ║\n"
             << "  ║   [5]  ★ Playlists                                             ║\n"
             << "  ║   [6]  ■ Exit                                                  ║\n"
             << "  ║                                                                ║\n"
             << "  ╚════════════════════════════════════════════════════════════════╝\n";
    } else {
        cout << "\n"
             << "  ╔════════════════════════════════════════════════════════════════╗\n"
             << "  ║                     ♫  MUSIC PLAYER  ♫                     ║\n"
             << "  ╚════════════════════════════════════════════════════════════════╝\n"
             << "  ║                                                                ║\n"
             << "  ║   [1]  ♫ View Library                                          ║\n"
             << "  ║   [2]  ◉ Search Songs                                          ║\n"
             << "  ║   [3]  ♥ Favorites                                             ║\n"
             << "  ║   [4]  ★ Playlists                                             ║\n"
             << "  ║   [5]  ■ Exit                                                  ║\n"
             << "  ║                                                                ║\n"
             << "  ║                                                                ║\n"
             << "  ║                                                                ║\n"
             << "  ╚════════════════════════════════════════════════════════════════╝\n";
    }

    if (manager.getSongCount() > 0)
        cout << "  | " << manager.getSongCount() << " songs |\n\n\n";
    else
        cout << "\n\n\n";

    cout << "  ▸ Select option: ";
    int choice = UIUtils::readIntChoice();

    if (playing) {
        switch (choice) {
            case 1: return Screen::LIBRARY;
            case 2: return Screen::NOW_PLAYING;
            case 3: return Screen::SEARCH;
            case 4: return Screen::FAVORITES;
            case 5: return Screen::PLAYLISTS;
            case 6: return Screen::EXIT;
            default: return Screen::MENU;
        }
    } else {
        switch (choice) {
            case 1: return Screen::LIBRARY;
            case 2: return Screen::SEARCH;
            case 3: return Screen::FAVORITES;
            case 4: return Screen::PLAYLISTS;
            case 5: return Screen::EXIT;
            default: return Screen::MENU;
        }
    }
}
