/*
 * NowPlayingSection.cpp
 * =====================
 * Shows the currently playing song with a live progress bar
 * and provides playback controls (stop, next, prev, like, etc.).
 *
 * The progress bar auto-updates every ~1 second using a
 * non-blocking keypress reader with timeout.
 */

#pragma once
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include "Screen.cpp"
#include "UIUtils.cpp"
#include "../song_management/MusicManager.cpp"

using namespace std;

// Helper: format milliseconds as "M:SS" (e.g. "3:45")
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

    UIUtils::printBoxTop("▶  NOW PLAYING");
    UIUtils::printBoxLine("");
    UIUtils::printBoxLine("  ♫ " + current->getTitle());
    UIUtils::printBoxLine("    " + current->getArtist());
    UIUtils::printBoxLine("");

    // Calculate progress bar position
    int lengthMs = current->getDurationSeconds() * 1000;
    int positionMs = manager.getPlaybackElapsedMs();

    // Auto-advance to next song when the current one ends
    if (lengthMs > 0 && positionMs >= lengthMs) {
        positionMs = lengthMs;
        manager.playNext();
        manager.startAudio(manager.getNowPlaying()->getFilePath());
        return Screen::NOW_PLAYING;
    }

    string timeL = formatTime(positionMs);
    string timeR = formatTime(lengthMs > 0 ? lengthMs : current->getDurationSeconds() * 1000);

    // Build the visual progress bar: [=====>-----------]
    const int BAR_WIDTH = 30;
    int filled = (lengthMs > 0) ? (positionMs * BAR_WIDTH / lengthMs) : 0;
    if (filled > BAR_WIDTH) filled = BAR_WIDTH;
    int empty = BAR_WIDTH - filled;

    string bar = "";
    for (int i = 0; i < filled; ++i) bar += "=";
    if (empty > 0) { bar += ">"; empty--; }
    for (int i = 0; i < empty; ++i) bar += "-";

    string progress = timeL + " [" + bar + "] " + timeR;
    UIUtils::printBoxLine("  " + progress);

    UIUtils::printBoxLine("");
    UIUtils::printBoxLine(current->isFavorite() ? "  ♥ Liked" : "  Not Liked");
    UIUtils::printBoxLine("");
    UIUtils::printBoxBottom();

    // Playback control bar
    cout << "  ────────────────────────────────────────────────────────────────\n";
    cout << "  ║ [1]■ Stop [2]>> Next [3]<< Prev [4]♥ Like [5]+List             ║\n";
    cout << "  ║ [6]Rename [0]Back                                              ║\n";
    cout << "  ╚════════════════════════════════════════════════════════════════╝\n\n";
    cout << "  ▸ Choice: ";

    // Non-blocking keypress with 1-second timeout (redraws progress)
    int choice = UIUtils::readSingleKeyWithTimeout(1000);

    if (choice == -1) return Screen::NOW_PLAYING;  // timeout — redraw

    switch (choice) {
        case 1:
            // Stop playback using the encapsulated audio method
            manager.stop();
            manager.stopAudio();
            return Screen::MENU;

        case 2:
            manager.playNext();
            manager.startAudio(manager.getNowPlaying()->getFilePath());
            break;

        case 3:
            manager.playPrevious();
            manager.startAudio(manager.getNowPlaying()->getFilePath());
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
            cout << "  ▸ New Title (or press enter to keep '"
                 << current->getTitle() << "'): ";
            string newTitle;
            getline(cin, newTitle);
            if (newTitle.empty()) newTitle = current->getTitle();

            cout << "  ▸ New Artist (or press enter to keep '"
                 << current->getArtist() << "'): ";
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
