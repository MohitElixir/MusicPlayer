#pragma once
#include "../song_management/MusicManager.cpp"
#include "Screen.cpp"
#include "UIUtils.cpp"
#include <iostream>
#include <string>

using namespace std;

Screen showMenu(MusicManager& manager) {
    manager.refreshFromFolder("Music");
    
    UIUtils::printBoxTop("♫  MUSIC PLAYER  ♫");
    
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
        
        cout << "  ║" << string(pad1, ' ') << center_title << string(62 - UIUtils::visualLength(center_title) - pad1, ' ') << "║\n";
        cout << "  ║" << string(pad2, ' ') << center_artist << string(62 - UIUtils::visualLength(center_artist) - pad2, ' ') << "║\n";
        cout << "  ║                                                                ║\n";
    }

    UIUtils::printBoxLine("  [1]  ♫ View Library");
    if (playing) {
        UIUtils::printBoxLine("  [2]  ▶ Now Playing");
        UIUtils::printBoxLine("  [3]  ◉ Search Songs");
        UIUtils::printBoxLine("  [4]  ♥ Favorites");
        UIUtils::printBoxLine("  [5]  ★ Playlists");
        UIUtils::printBoxLine("  [6]  ■ Exit");
    } else {
        UIUtils::printBoxLine("  [2]  ◉ Search Songs");
        UIUtils::printBoxLine("  [3]  ♥ Favorites");
        UIUtils::printBoxLine("  [4]  ★ Playlists");
        UIUtils::printBoxLine("  [5]  ■ Exit");
        // Print extra lines to keep the box size consistent when not playing
        cout << "  ║                                                                ║\n";
        cout << "  ║                                                                ║\n";
    }
    
    UIUtils::printBoxBottom();
    
    if (manager.getSongCount() > 0) {
        cout << "  | " << manager.getSongCount() << " songs |\n\n\n";
    } else {
        cout << "\n\n\n";
    }

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
