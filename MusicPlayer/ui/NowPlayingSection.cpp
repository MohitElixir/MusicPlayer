#pragma once
#include "../song_management/MusicManager.cpp"
#include "Screen.cpp"
#include "UIUtils.cpp"
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

namespace {
    string formatTime(int ms) {
        int totalSec = ms / 1000;
        int m = totalSec / 60;
        int s = totalSec % 60;
        ostringstream oss;
        oss << m << ":" << setfill('0') << setw(2) << s;
        return oss.str();
    }
}

Screen showNowPlaying(MusicManager& manager) {
    const Song* current = manager.getNowPlaying();
    if (!current) return Screen::MENU;

    UIUtils::updateLinuxPlaybackState(manager);

    UIUtils::printBoxTop("▶  NOW PLAYING");
    UIUtils::printBoxLine("");
    UIUtils::printBoxLine("  ♫ " + current->getTitle());
    UIUtils::printBoxLine("    " + current->getArtist());
    UIUtils::printBoxLine("");
    
    // Fetch dynamic position and length from MCI
    int lengthMs = 0;
    int positionMs = 0;
#ifdef _WIN32
    char lenBuf[128] = {0}, posBuf[128] = {0};
    mciSendStringA("status mymusic length", lenBuf, sizeof(lenBuf), NULL);
    mciSendStringA("status mymusic position", posBuf, sizeof(posBuf), NULL);
    lengthMs  = atoi(lenBuf);
    positionMs = atoi(posBuf);
#else
    lengthMs = current->getDurationSeconds() * 1000;
    positionMs = 0; 
#endif

    string timeL = formatTime(positionMs);
    string timeR = formatTime(lengthMs > 0 ? lengthMs : current->getDurationSeconds() * 1000);

    const int BAR_WIDTH = 30;
    int filled = (lengthMs > 0) ? (positionMs * BAR_WIDTH / lengthMs) : 0;
    if (filled > BAR_WIDTH) filled = BAR_WIDTH;
    int empty = BAR_WIDTH - filled;

    string bar = "";
    for (int i = 0; i < filled; ++i) bar += "=";
    if (empty > 0) {
        bar += ">";
        empty--;
    }
    for (int i = 0; i < empty; ++i) bar += "-";
    
    string progress = timeL + " [" + bar + "] " + timeR;
    UIUtils::printBoxLine("  " + progress);
    
    UIUtils::printBoxLine("");
    if (current->isFavorite()) {
        UIUtils::printBoxLine("  ♥ Liked");
    } else {
        UIUtils::printBoxLine("  Not Liked");
    }
    UIUtils::printBoxLine("");
    UIUtils::printBoxBottom();

    // The exact control bar requested by the user
    cout << "  ────────────────────────────────────────────────────────────────\n";
    cout << "  ║ [1]■ Stop [2]>> Next [3]<< Prev [4]♥ Like [5]+List             ║\n";
    cout << "  ║ [6]Rename [0]Back                                              ║\n";
    cout << "  ╚════════════════════════════════════════════════════════════════╝\n\n";
    cout << "  ▸ Choice: ";

    // Non-blocking single keypress read with 1000ms timeout
    int choice = UIUtils::readSingleKeyWithTimeout(1000);
    
    if (choice == -1) {
        // Timeout: loop back to redraw and update the progress bar
        return Screen::NOW_PLAYING;
    }

    switch (choice) {
        case 1:
            manager.stop();
#ifdef _WIN32
            mciSendStringA("close all", NULL, 0, NULL);
#endif
            return Screen::MENU;
        case 2:
            manager.playNext();
            UIUtils::playSystemAudio(manager.getNowPlaying());
            break;
        case 3:
            manager.playPrevious();
            UIUtils::playSystemAudio(manager.getNowPlaying());
            break;
        case 4:
            manager.toggleFavorite(manager.getNowPlayingIndex());
            break;
        case 5: {
            if (manager.getPlaylists().empty()) {
                cout << "  No playlists available. Create one first.\n";
                UIUtils::pause();
                break;
            }
            cout << "  ▸ Select Playlist # to add to: ";
            int pl = UIUtils::readIntChoice();
            if (pl > 0 && pl <= static_cast<int>(manager.getPlaylists().size())) {
                manager.addToPlaylist(pl - 1, manager.getNowPlayingIndex());
                cout << "  Added!\n";
                UIUtils::pause();
            }
            break;
        }
        case 6: {
            cout << "  ▸ New Title (or press enter to keep '" << current->getTitle() << "'): ";
            string newTitle;
            getline(cin, newTitle);
            if (newTitle.empty()) newTitle = current->getTitle();

            cout << "  ▸ New Artist (or press enter to keep '" << current->getArtist() << "'): ";
            string newArtist;
            getline(cin, newArtist);
            if (newArtist.empty()) newArtist = current->getArtist();

            manager.renameSong(manager.getNowPlayingIndex(), newTitle, newArtist);
            cout << "  Song renamed successfully!\n";
            UIUtils::pause();
            break;
        }
        case 0:
            return Screen::MENU;
    }
    
    return Screen::NOW_PLAYING;
}
