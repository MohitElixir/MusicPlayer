#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <chrono>

// ============================================================
// Song: Concrete Class
// ============================================================
class Song {
private:
    std::string title;
    std::string artist;
    int durationSeconds;
    std::string filePath;
    bool favorite;

public:
    Song();
    Song(const std::string& title, const std::string& artist, int durationSeconds, const std::string& filePath);

    std::string getTitle() const;
    std::string getArtist() const;
    int getDurationSeconds() const;
    std::string getFilePath() const;
    bool isFavorite() const;

    void setFavorite(bool fav);
    void setTitle(const std::string& t);
    void setArtist(const std::string& a);
    void setFilePath(const std::string& path);

    std::string getFormattedDuration() const;

    bool operator==(const Song& other) const;
    friend std::ostream& operator<<(std::ostream& os, const Song& song);
};

// ============================================================
// Playlist Struct
// ============================================================
struct Playlist {
    std::string name;
    std::vector<int> songIndices;
};

// ============================================================
// Core Engine
// ============================================================
class MusicManager {
private:
    std::vector<Song> library;
    std::vector<Playlist> playlists;
    int nowPlayingIndex; // -1 if nothing is playing
    std::string dataFilePath;
    std::chrono::steady_clock::time_point playbackStartTime;

public:
    MusicManager();

    void addSong(const Song& song);
    void addSong(const std::string& title, const std::string& artist, int durationSeconds, const std::string& filePath);
    int getSongCount() const;
    bool hasSong(const std::string& title) const;
    void refreshFromFolder(const std::string& folderPath);
    bool renameSong(int index, const std::string& newFileName);

    const std::vector<Song>& getLibrary() const;
    const Song* getSongAt(int index) const;

    std::vector<int> searchByTitle(const std::string& query) const;
    std::vector<int> searchByArtist(const std::string& query) const;
    std::vector<int> searchAll(const std::string& query) const;

    void toggleFavorite(int index);
    std::vector<int> getFavorites() const;

    void addPlaylist(const std::string& name);
    void addToPlaylist(int playlistIndex, int songIndex);
    const std::vector<Playlist>& getPlaylists() const;

    bool play(int index);
    void stop();
    bool isPlaying() const;
    int getPlaybackElapsedMs() const;
    
    const Song* getNowPlaying() const;
    int getNowPlayingIndex() const;
    void playNext();
    void playPrevious();

    std::string getTotalRuntimeFormatted() const;

    void renameSong(int index, const std::string& newTitle, const std::string& newArtist);

    void saveData() const;
    void loadData();
};



#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#define popen _popen
#define pclose _pclose
#endif
#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

using namespace std;

Song::Song()
    : title("Unknown Title"), artist("Unknown Artist"), durationSeconds(0), filePath(""), favorite(false) {
}

Song::Song(const std::string& title, const std::string& artist, int durationSeconds, const std::string& filePath)
    : title(title), artist(artist), durationSeconds(durationSeconds), filePath(filePath), favorite(false) {
}

std::string Song::getTitle() const { return title; }
std::string Song::getArtist() const { return artist; }
int Song::getDurationSeconds() const { return durationSeconds; }
std::string Song::getFilePath() const { return filePath; }
bool Song::isFavorite() const { return favorite; }

void Song::setFavorite(bool fav) { favorite = fav; }
void Song::setTitle(const std::string& t) { title = t; }
void Song::setArtist(const std::string& a) { artist = a; }
void Song::setFilePath(const std::string& path) { filePath = path; }

std::string Song::getFormattedDuration() const {
    int minutes = durationSeconds / 60;
    int seconds = durationSeconds % 60;

    std::ostringstream oss;
    oss << minutes << ":" << std::setfill('0') << std::setw(2) << seconds;
    return oss.str();
}

bool Song::operator==(const Song& other) const {
    return (title == other.title && artist == other.artist && durationSeconds == other.durationSeconds);
}

std::ostream& operator<<(std::ostream& os, const Song& song) {
    if (song.isFavorite()) os << "[<3] ";
    os << song.getTitle() << " - " << song.getArtist() << " (" << song.getFormattedDuration() << ")";
    return os;
}



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
        string cmd = "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"" + filepath + "\"";
#ifdef _WIN32
        cmd += " 2>NUL";
#else
        cmd += " 2>/dev/null";
#endif
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
    }
}

MusicManager::MusicManager() : nowPlayingIndex(-1), dataFilePath("userdata.txt") {}

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

void MusicManager::renameSong(int index, const string& newTitle, const string& newArtist) {
    if (index >= 0 && index < library.size()) {
        library[index].setTitle(newTitle);
        library[index].setArtist(newArtist);
        saveData();
    }
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
        if (toLower(library[i].getTitle()).find(needle) != string::npos ||
            toLower(library[i].getArtist()).find(needle) != string::npos) {
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
    if (!fs::exists(library[index].getFilePath())) {
        return false;
    }
    nowPlayingIndex = index;
    playbackStartTime = std::chrono::steady_clock::now();
    return true;
}

void MusicManager::stop() {
    nowPlayingIndex = -1;
}

bool MusicManager::isPlaying() const {
    return nowPlayingIndex != -1;
}

int MusicManager::getPlaybackElapsedMs() const {
    if (nowPlayingIndex == -1) return 0;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - playbackStartTime).count();
    return static_cast<int>(elapsed);
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
    playbackStartTime = std::chrono::steady_clock::now();
}

void MusicManager::playPrevious() {
    if (library.empty()) return;
    if (nowPlayingIndex == -1) {
        nowPlayingIndex = 0;
    } else {
        nowPlayingIndex = (nowPlayingIndex - 1 + library.size()) % library.size();
    }
    playbackStartTime = std::chrono::steady_clock::now();
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

void MusicManager::saveData() const {
    ofstream outFile(dataFilePath);
    if (!outFile) return;
    
    // Save favorites
    vector<int> favs = getFavorites();
    outFile << favs.size() << "\n";
    for (int idx : favs) {
        outFile << library[idx].getFilePath() << "\n";
    }
    
    // Save playlists
    outFile << playlists.size() << "\n";
    for (const auto& pl : playlists) {
        outFile << pl.name << "\n";
        outFile << pl.songIndices.size() << "\n";
        for (int idx : pl.songIndices) {
            outFile << library[idx].getFilePath() << "\n";
        }
    }
}

void MusicManager::loadData() {
    ifstream inFile(dataFilePath);
    if (!inFile) return;

    size_t favCount;
    if (!(inFile >> favCount)) return;
    inFile.ignore();

    for (size_t i = 0; i < favCount; ++i) {
        string path;
        getline(inFile, path);
        for (auto& s : library) {
            if (s.getFilePath() == path) {
                s.setFavorite(true);
                break;
            }
        }
    }

    size_t plCount;
    if (!(inFile >> plCount)) return;
    inFile.ignore();
    
    playlists.clear();
    for (size_t i = 0; i < plCount; ++i) {
        string name;
        getline(inFile, name);
        size_t sCount;
        inFile >> sCount;
        inFile.ignore();
        
        Playlist pl;
        pl.name = name;
        for (size_t j = 0; j < sCount; ++j) {
            string path;
            getline(inFile, path);
            for (size_t k = 0; k < library.size(); ++k) {
                if (library[k].getFilePath() == path) {
                    pl.songIndices.push_back(k);
                    break;
                }
            }
        }
        playlists.push_back(pl);
    }
}

