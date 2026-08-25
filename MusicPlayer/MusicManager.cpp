#ifdef _WIN32
#include <windows.h>
#else
#include <cstdio>
#endif
#include "MusicManager.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <filesystem>

using namespace std;

namespace fs = filesystem;

namespace {
    // Helper: lowercase a string for case-insensitive comparisons
    string toLower(const string& s) {
        string result = s;
        transform(result.begin(), result.end(), result.begin(),
                        [](unsigned char c) { return tolower(c); });
        return result;
    }

    int getMediaDuration(const string& filepath) {
#ifdef _WIN32
        char lenBuf[128] = {0};
        string alias = "len_tmp";
        string cmdOpen = "open \"" + filepath + "\" alias " + alias;
        mciSendStringA(cmdOpen.c_str(), NULL, 0, NULL);
        mciSendStringA(("status " + alias + " length").c_str(), lenBuf, sizeof(lenBuf), NULL);
        mciSendStringA(("close " + alias).c_str(), NULL, 0, NULL);
        return atoi(lenBuf) / 1000;
#else
        string cmd = "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"" + filepath + "\" 2>/dev/null";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return 0;
        char buffer[128];
        string result = "";
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        if (!result.empty()) {
            try {
                double seconds = stod(result);
                return static_cast<int>(seconds);
            } catch (...) {}
        }
        return 0;
#endif
    }
}

MusicManager::MusicManager() : nowPlayingIndex(-1) {}

void MusicManager::addSong(const Song& song) {
    library.push_back(song);
}

void MusicManager::addSong(const string& title, const string& artist, int durationSeconds, const string& filePath) {
    library.emplace_back(title, artist, durationSeconds, filePath);
}

bool MusicManager::hasSong(const string& title) const {
    string needle = toLower(title);
    for (const auto& s : library) {
        if (toLower(s.getTitle()) == needle) {
            return true;
        }
    }
    return false;
}

void MusicManager::refreshFromFolder(const string& folderPath) {
    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        return;
    }

    vector<Song> newLibrary;

    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            string ext = entry.path().extension().string();
            ext = toLower(ext);

            if (ext == ".mp3" || ext == ".wav") {
                string filepath = entry.path().string();
                
                bool found = false;
                for (const auto& existing : library) {
                    if (existing.getFilePath() == filepath) {
                        newLibrary.push_back(existing);
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    string stem = entry.path().stem().string();
                    string artist = "Unknown";
                    string title = stem;
                    size_t dashPos = stem.find('-');
                    if (dashPos != string::npos) {
                        artist = stem.substr(0, dashPos);
                        title = stem.substr(dashPos + 1);
                        artist.erase(0, artist.find_first_not_of(" \t"));
                        artist.erase(artist.find_last_not_of(" \t") + 1);
                        title.erase(0, title.find_first_not_of(" \t"));
                        title.erase(title.find_last_not_of(" \t") + 1);
                    }
                    
                    int duration = getMediaDuration(filepath);
                    newLibrary.emplace_back(title, artist, duration, filepath);
                }
            }
        }
    }
    
    library = newLibrary;
}

int MusicManager::getSongCount() const {
    return static_cast<int>(library.size());
}

bool MusicManager::renameSong(int index, const string& newFileName) {
    if (index < 0 || index >= static_cast<int>(library.size())) return false;
    
    fs::path oldPath = library[index].getFilePath();
    fs::path newPath = oldPath.parent_path() / newFileName;

    if (newPath.extension() != ".mp3") {
        newPath += ".mp3";
    }

    try {
        fs::rename(oldPath, newPath);
    } catch (...) {
        return false;
    }

    library[index].setFilePath(newPath.string());
    
    string stem = newPath.stem().string();
    size_t dashPos = stem.find('-');
    if (dashPos != string::npos) {
        string artist = stem.substr(0, dashPos);
        string title = stem.substr(dashPos + 1);
        
        artist.erase(0, artist.find_first_not_of(" \t"));
        artist.erase(artist.find_last_not_of(" \t") + 1);
        title.erase(0, title.find_first_not_of(" \t"));
        title.erase(title.find_last_not_of(" \t") + 1);
        
        library[index].setArtist(artist);
        library[index].setTitle(title);
    } else {
        library[index].setArtist("Unknown");
        library[index].setTitle(stem);
    }
    return true;
}

const vector<Song>& MusicManager::getLibrary() const {
    return library;
}

const Song* MusicManager::getSongAt(int index) const {
    if (index < 0 || index >= static_cast<int>(library.size())) {
        return nullptr;
    }
    return &library[index];
}

vector<int> MusicManager::searchByTitle(const string& query) const {
    vector<int> results;
    string needle = toLower(query);

    for (size_t i = 0; i < library.size(); ++i) {
        if (toLower(library[i].getTitle()).find(needle) != string::npos) {
            results.push_back(static_cast<int>(i));
        }
    }
    return results;
}

vector<int> MusicManager::searchByArtist(const string& query) const {
    vector<int> results;
    string needle = toLower(query);

    for (size_t i = 0; i < library.size(); ++i) {
        if (toLower(library[i].getArtist()).find(needle) != string::npos) {
            results.push_back(static_cast<int>(i));
        }
    }
    return results;
}

vector<int> MusicManager::searchAll(const string& query) const {
    vector<int> results;
    string needle = toLower(query);

    for (size_t i = 0; i < library.size(); ++i) {
        const Song& s = library[i];
        if (toLower(s.getTitle()).find(needle) != string::npos ||
            toLower(s.getArtist()).find(needle) != string::npos) {
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

vector<int> MusicManager::getFavorites() const {
    vector<int> results;
    for (size_t i = 0; i < library.size(); ++i) {
        if (library[i].isFavorite()) {
            results.push_back(static_cast<int>(i));
        }
    }
    return results;
}

void MusicManager::addPlaylist(const string& name) {
    playlists.push_back({name, {}});
}

void MusicManager::addToPlaylist(int playlistIndex, int songIndex) {
    if (playlistIndex >= 0 && playlistIndex < static_cast<int>(playlists.size()) &&
        songIndex >= 0 && songIndex < static_cast<int>(library.size())) {
        playlists[playlistIndex].songIndices.push_back(songIndex);
    }
}

const vector<Playlist>& MusicManager::getPlaylists() const {
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

string MusicManager::getTotalRuntimeFormatted() const {
    int totalSeconds = 0;
    for (const auto& s : library) {
        totalSeconds += s.getDurationSeconds();
    }

    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    ostringstream oss;
    if (hours > 0) {
        oss << hours << "h " << setfill('0') << setw(2) << minutes << "m "
            << setfill('0') << setw(2) << seconds << "s";
    } else {
        oss << minutes << "m " << setfill('0') << setw(2) << seconds << "s";
    }
    return oss.str();
}
