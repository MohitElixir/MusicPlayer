#include "MusicManager.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <windows.h>
#include <iostream>

namespace {
    // Helper: lowercase a string for case-insensitive comparisons
    std::string toLower(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(),
                        [](unsigned char c) { return std::tolower(c); });
        return result;
    }
}

MusicManager::MusicManager() : nowPlayingIndex(-1) {}

void MusicManager::addSong(const Song& song) {
    library.push_back(song);
}

void MusicManager::addSong(const std::string& title, const std::string& artist, int durationSeconds, const std::string& filePath) {
    library.emplace_back(title, artist, durationSeconds, filePath);
}

bool MusicManager::hasSong(const std::string& title) const {
    std::string needle = toLower(title);
    for (const auto& s : library) {
        if (toLower(s.getTitle()) == needle) {
            return true;
        }
    }
    return false;
}

void MusicManager::refreshFromFolder(const std::string& folderPath) {
    WIN32_FIND_DATAA findFileData;
    std::string searchPath = folderPath + "\\*.*";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findFileData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        // optionally log a warning, but we'll just return silently to not disrupt the UI
        return;
    }
    
    do {
        std::string filename = findFileData.cFileName;
        if (filename.length() > 4) {
            std::string ext = filename.substr(filename.length() - 4);
            for (char& c : ext) c = std::tolower(c);
            
            if (ext == ".mp3" || ext == ".wav") {
                std::string title = filename.substr(0, filename.length() - 4);
                if (!hasSong(title)) {
                    std::string filepath = folderPath + "\\" + filename;
                    addSong(title, "Unknown", 0, filepath);
                }
            }
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);
    
    FindClose(hFind);
}

int MusicManager::getSongCount() const {
    return static_cast<int>(library.size());
}

const std::vector<Song>& MusicManager::getLibrary() const {
    return library;
}

const Song* MusicManager::getSongAt(int index) const {
    if (index < 0 || index >= static_cast<int>(library.size())) {
        return nullptr;
    }
    return &library[index];
}

std::vector<int> MusicManager::searchByTitle(const std::string& query) const {
    std::vector<int> results;
    std::string needle = toLower(query);

    for (size_t i = 0; i < library.size(); ++i) {
        if (toLower(library[i].getTitle()).find(needle) != std::string::npos) {
            results.push_back(static_cast<int>(i));
        }
    }
    return results;
}

std::vector<int> MusicManager::searchByArtist(const std::string& query) const {
    std::vector<int> results;
    std::string needle = toLower(query);

    for (size_t i = 0; i < library.size(); ++i) {
        if (toLower(library[i].getArtist()).find(needle) != std::string::npos) {
            results.push_back(static_cast<int>(i));
        }
    }
    return results;
}

std::vector<int> MusicManager::searchAll(const std::string& query) const {
    std::vector<int> results;
    std::string needle = toLower(query);

    for (size_t i = 0; i < library.size(); ++i) {
        const Song& s = library[i];
        if (toLower(s.getTitle()).find(needle) != std::string::npos ||
            toLower(s.getArtist()).find(needle) != std::string::npos) {
            results.push_back(static_cast<int>(i));
        }
    }
    return results;
}

void MusicManager::toggleFavorite(int index) {
    if (index >= 0 && index < static_cast<int>(library.size())) {
        library[index].setFavorite(!library[index].isFavorite());
    }
}

std::vector<int> MusicManager::getFavorites() const {
    std::vector<int> results;
    for (size_t i = 0; i < library.size(); ++i) {
        if (library[i].isFavorite()) {
            results.push_back(static_cast<int>(i));
        }
    }
    return results;
}

void MusicManager::addPlaylist(const std::string& name) {
    playlists.push_back({name, {}});
}

void MusicManager::addToPlaylist(int playlistIndex, int songIndex) {
    if (playlistIndex >= 0 && playlistIndex < static_cast<int>(playlists.size()) &&
        songIndex >= 0 && songIndex < static_cast<int>(library.size())) {
        playlists[playlistIndex].songIndices.push_back(songIndex);
    }
}

const std::vector<Playlist>& MusicManager::getPlaylists() const {
    return playlists;
}

bool MusicManager::play(int index) {
    if (index < 0 || index >= static_cast<int>(library.size())) {
        return false;
    }
    nowPlayingIndex = index;
    return true;
}

void MusicManager::stop() {
    nowPlayingIndex = -1;
}

bool MusicManager::isPlaying() const {
    return nowPlayingIndex != -1;
}

const Song* MusicManager::getNowPlaying() const {
    if (!isPlaying()) return nullptr;
    return &library[nowPlayingIndex];
}

int MusicManager::getNowPlayingIndex() const {
    return nowPlayingIndex;
}

void MusicManager::playNext() {
    if (library.empty()) return;
    if (nowPlayingIndex == -1) {
        nowPlayingIndex = 0;
    } else {
        nowPlayingIndex = (nowPlayingIndex + 1) % library.size();
    }
}

void MusicManager::playPrevious() {
    if (library.empty()) return;
    if (nowPlayingIndex == -1) {
        nowPlayingIndex = 0;
    } else {
        nowPlayingIndex = (nowPlayingIndex - 1 + library.size()) % library.size();
    }
}

std::string MusicManager::getTotalRuntimeFormatted() const {
    int totalSeconds = 0;
    for (const auto& s : library) {
        totalSeconds += s.getDurationSeconds();
    }

    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    std::ostringstream oss;
    if (hours > 0) {
        oss << hours << "h " << std::setfill('0') << std::setw(2) << minutes << "m "
            << std::setfill('0') << std::setw(2) << seconds << "s";
    } else {
        oss << minutes << "m " << std::setfill('0') << std::setw(2) << seconds << "s";
    }
    return oss.str();
}
