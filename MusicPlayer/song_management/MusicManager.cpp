/*
 * MusicManager.cpp
 * ================
 * Core engine of the Music Player application.
 *
 * Contains three main components:
 *   1. Song class       - Represents a single music track (Encapsulation, Constructors)
 *   2. Playlist struct  - Groups song indices under a name
 *   3. MusicManager class - Manages the library, playlists, playback,
 *                           search, favorites, and file I/O
 *
 * OOP Concepts demonstrated:
 *   - Encapsulation (private data members, public getters/setters)
 *   - Constructors (default + parameterized)
 *   - Operator Overloading (==, <<)
 *   - Function Overloading (addSong has 2 versions, renameSong has 2 versions)
 *
 * Course: Object-Oriented Programming in C++
 * Semester: 2nd Semester BSc.IT
 */

#pragma once

// ===================== Standard Library Includes =====================
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

// ===================== Platform-Specific Includes ====================
#ifdef _WIN32
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #include <windows.h>
    // Windows uses _popen/_pclose instead of popen/pclose
    #define popen  _popen
    #define pclose _pclose
#endif

using namespace std;
namespace fs = filesystem;


// =====================================================================
//  Song Class
//  Represents a single music track. Demonstrates Encapsulation
//  (private data, public interface) and Operator Overloading.
// =====================================================================

class Song {
private:
    // Private data members — only accessible through getters/setters
    string title;
    string artist;
    int durationSeconds;
    string filePath;
    bool favorite;

public:
    // Default constructor
    Song()
        : title("Unknown Title"), artist("Unknown Artist"),
          durationSeconds(0), filePath(""), favorite(false) {}

    // Parameterized constructor
    Song(const string& title, const string& artist,
         int durationSeconds, const string& filePath)
        : title(title), artist(artist),
          durationSeconds(durationSeconds), filePath(filePath), favorite(false) {}

    // --- Getters ---
    string getTitle() const { return title; }
    string getArtist() const { return artist; }
    int getDurationSeconds() const { return durationSeconds; }
    string getFilePath() const { return filePath; }
    bool isFavorite() const { return favorite; }

    // --- Setters ---
    void setTitle(const string& t) { title = t; }
    void setArtist(const string& a) { artist = a; }
    void setFilePath(const string& path) { filePath = path; }
    void setFavorite(bool fav) { favorite = fav; }

    // Returns duration formatted as "M:SS" (e.g. "3:45")
    string getFormattedDuration() const {
        int minutes = durationSeconds / 60;
        int seconds = durationSeconds % 60;
        ostringstream oss;
        oss << minutes << ":" << setfill('0') << setw(2) << seconds;
        return oss.str();
    }

    // Operator overloading: compare two songs by title, artist, duration
    bool operator==(const Song& other) const {
        return (title == other.title &&
                artist == other.artist &&
                durationSeconds == other.durationSeconds);
    }

    // Operator overloading: print song info using cout << song
    friend ostream& operator<<(ostream& os, const Song& song) {
        if (song.isFavorite()) os << "[<3] ";
        os << song.getTitle() << " - " << song.getArtist()
           << " (" << song.getFormattedDuration() << ")";
        return os;
    }
};


// =====================================================================
//  Playlist Struct — A named collection of song indices
// =====================================================================

struct Playlist {
    string name;
    vector<int> songIndices;   // stores indices into MusicManager's library
};


// =====================================================================
//  MusicManager Class
//  Core engine that manages the song library, playlists, playback,
//  search, favorites, audio control, and data persistence.
//  Demonstrates Encapsulation and Function Overloading.
// =====================================================================

class MusicManager {
private:
    // Private data (Encapsulation — hidden from outside code)
    vector<Song> library;           // all songs loaded from the Music folder
    vector<Playlist> playlists;     // user-created playlists
    int nowPlayingIndex;            // index of currently playing song (-1 if none)
    string dataFilePath;            // path to saved user data file
    chrono::steady_clock::time_point playbackStartTime;  // for progress tracking

    // --- Private helper functions ---

    // Convert string to lowercase for case-insensitive search
    static string toLower(const string& s) {
        string result = s;
        transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return tolower(c); });
        return result;
    }

    // Use ffprobe to get the duration of an audio file (in seconds)
    static int getMediaDuration(const string& filepath) {
        string cmd = "ffprobe -v error -show_entries format=duration "
                     "-of default=noprint_wrappers=1:nokey=1 \""
                     + filepath + "\"";
#ifdef _WIN32
        cmd += " 2>NUL";
#else
        cmd += " 2>/dev/null";
#endif
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return 0;

        char buffer[128];
        string result = "";
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            result += buffer;
        pclose(pipe);

        if (!result.empty()) {
            try {
                return static_cast<int>(stod(result));
            } catch (...) {}
        }
        return 0;
    }

public:
    // Constructor — initializes default values
    MusicManager()
        : nowPlayingIndex(-1), dataFilePath("userdata.txt") {}


    // === Library Management ===

    // Function Overloading: addSong (version 1 — takes a Song object)
    void addSong(const Song& song) {
        library.push_back(song);
    }

    // Function Overloading: addSong (version 2 — takes individual fields)
    void addSong(const string& title, const string& artist,
                 int durationSeconds, const string& filePath) {
        library.emplace_back(title, artist, durationSeconds, filePath);
    }

    int getSongCount() const {
        return static_cast<int>(library.size());
    }

    bool hasSong(const string& title) const {
        string needle = toLower(title);
        for (const auto& s : library)
            if (toLower(s.getTitle()) == needle)
                return true;
        return false;
    }

    // Scans a folder for .mp3/.wav files and builds the library.
    // Preserves metadata (favorites, custom names) for known files.
    void refreshFromFolder(const string& folderPath) {
        if (!fs::exists(folderPath) || !fs::is_directory(folderPath))
            return;

        vector<Song> newLibrary;

        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (!entry.is_regular_file()) continue;

            string ext = toLower(entry.path().extension().string());
            if (ext != ".mp3" && ext != ".wav") continue;

            string filepath = entry.path().string();

            // Check if this file already exists in our library
            bool found = false;
            for (const auto& existing : library) {
                if (existing.getFilePath() == filepath) {
                    newLibrary.push_back(existing);  // keep existing metadata
                    found = true;
                    break;
                }
            }

            // New file — parse artist/title from "Artist - Title" filename format
            if (!found) {
                string stem = entry.path().stem().string();
                string artist = "Unknown";
                string title = stem;

                size_t dashPos = stem.find('-');
                if (dashPos != string::npos) {
                    artist = stem.substr(0, dashPos);
                    title = stem.substr(dashPos + 1);
                    // Trim whitespace
                    artist.erase(0, artist.find_first_not_of(" \t"));
                    artist.erase(artist.find_last_not_of(" \t") + 1);
                    title.erase(0, title.find_first_not_of(" \t"));
                    title.erase(title.find_last_not_of(" \t") + 1);
                }

                int duration = getMediaDuration(filepath);
                newLibrary.emplace_back(title, artist, duration, filepath);
            }
        }
        library = newLibrary;
    }

    const vector<Song>& getLibrary() const { return library; }

    const Song* getSongAt(int index) const {
        if (index < 0 || index >= static_cast<int>(library.size()))
            return nullptr;
        return &library[index];
    }


    // === Song Renaming (Function Overloading — 2 versions) ===

    // Version 1: Renames the actual file on disk
    bool renameSong(int index, const string& newFileName) {
        if (index < 0 || index >= static_cast<int>(library.size())) return false;

        fs::path oldPath = library[index].getFilePath();
        fs::path newPath = oldPath.parent_path() / newFileName;
        if (newPath.extension() != ".mp3") newPath += ".mp3";

        try { fs::rename(oldPath, newPath); }
        catch (...) { return false; }

        library[index].setFilePath(newPath.string());

        // Re-parse artist/title from new filename
        string stem = newPath.stem().string();
        size_t dashPos = stem.find('-');
        if (dashPos != string::npos) {
            string a = stem.substr(0, dashPos);
            string t = stem.substr(dashPos + 1);
            a.erase(0, a.find_first_not_of(" \t"));
            a.erase(a.find_last_not_of(" \t") + 1);
            t.erase(0, t.find_first_not_of(" \t"));
            t.erase(t.find_last_not_of(" \t") + 1);
            library[index].setArtist(a);
            library[index].setTitle(t);
        } else {
            library[index].setArtist("Unknown");
            library[index].setTitle(stem);
        }
        return true;
    }

    // Version 2: Renames just the metadata (title & artist)
    void renameSong(int index, const string& newTitle, const string& newArtist) {
        if (index >= 0 && index < static_cast<int>(library.size())) {
            library[index].setTitle(newTitle);
            library[index].setArtist(newArtist);
            saveData();
        }
    }


    // === Search ===

    vector<int> searchByTitle(const string& query) const {
        vector<int> results;
        string needle = toLower(query);
        for (size_t i = 0; i < library.size(); ++i)
            if (toLower(library[i].getTitle()).find(needle) != string::npos)
                results.push_back(static_cast<int>(i));
        return results;
    }

    vector<int> searchByArtist(const string& query) const {
        vector<int> results;
        string needle = toLower(query);
        for (size_t i = 0; i < library.size(); ++i)
            if (toLower(library[i].getArtist()).find(needle) != string::npos)
                results.push_back(static_cast<int>(i));
        return results;
    }

    vector<int> searchAll(const string& query) const {
        vector<int> results;
        string needle = toLower(query);
        for (size_t i = 0; i < library.size(); ++i)
            if (toLower(library[i].getTitle()).find(needle) != string::npos ||
                toLower(library[i].getArtist()).find(needle) != string::npos)
                results.push_back(static_cast<int>(i));
        return results;
    }


    // === Favorites ===

    void toggleFavorite(int index) {
        if (index >= 0 && index < static_cast<int>(library.size()))
            library[index].setFavorite(!library[index].isFavorite());
    }

    vector<int> getFavorites() const {
        vector<int> results;
        for (size_t i = 0; i < library.size(); ++i)
            if (library[i].isFavorite())
                results.push_back(static_cast<int>(i));
        return results;
    }


    // === Playlists ===

    void addPlaylist(const string& name) {
        playlists.push_back({name, {}});
    }

    void addToPlaylist(int playlistIndex, int songIndex) {
        if (playlistIndex >= 0 && playlistIndex < static_cast<int>(playlists.size()) &&
            songIndex >= 0 && songIndex < static_cast<int>(library.size()))
            playlists[playlistIndex].songIndices.push_back(songIndex);
    }

    const vector<Playlist>& getPlaylists() const { return playlists; }


    // === Playback Control ===

    bool play(int index) {
        if (index < 0 || index >= static_cast<int>(library.size())) return false;
        if (!fs::exists(library[index].getFilePath())) return false;
        nowPlayingIndex = index;
        playbackStartTime = chrono::steady_clock::now();
        return true;
    }

    void stop() { nowPlayingIndex = -1; }

    bool isPlaying() const { return nowPlayingIndex != -1; }

    // Returns milliseconds elapsed since current song started
    int getPlaybackElapsedMs() const {
        if (nowPlayingIndex == -1) return 0;
        auto now = chrono::steady_clock::now();
        return static_cast<int>(
            chrono::duration_cast<chrono::milliseconds>(now - playbackStartTime).count());
    }

    const Song* getNowPlaying() const {
        if (!isPlaying()) return nullptr;
        return &library[nowPlayingIndex];
    }

    int getNowPlayingIndex() const { return nowPlayingIndex; }

    void playNext() {
        if (library.empty()) return;
        nowPlayingIndex = (nowPlayingIndex == -1) ? 0
                          : (nowPlayingIndex + 1) % library.size();
        playbackStartTime = chrono::steady_clock::now();
    }

    void playPrevious() {
        if (library.empty()) return;
        nowPlayingIndex = (nowPlayingIndex == -1) ? 0
                          : (nowPlayingIndex - 1 + library.size()) % library.size();
        playbackStartTime = chrono::steady_clock::now();
    }


    // === Audio System Control ===
    // Encapsulates OS-specific commands to start/stop ffplay.
    // This keeps platform-dependent code inside the class.

    void startAudio(const string& filePath) {
        stopAudio();  // stop any currently playing audio
#ifdef _WIN32
        string command = "start /B ffplay -nodisp -autoexit \""
                         + filePath + "\" > NUL 2>&1";
#else
        string command = "ffplay -nodisp -autoexit \""
                         + filePath + "\" > /dev/null 2>&1 &";
#endif
        system(command.c_str());
    }

    void stopAudio() {
#ifdef _WIN32
        system("taskkill /f /im ffplay.exe > NUL 2>&1");
#else
        system("killall -9 ffplay > /dev/null 2>&1 || "
               "pkill -9 -f ffplay > /dev/null 2>&1");
#endif
    }


    // === Utility ===

    string getTotalRuntimeFormatted() const {
        int totalSeconds = 0;
        for (const auto& s : library)
            totalSeconds += s.getDurationSeconds();

        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int seconds = totalSeconds % 60;

        ostringstream oss;
        if (hours > 0)
            oss << hours << "h " << setfill('0') << setw(2) << minutes << "m "
                << setfill('0') << setw(2) << seconds << "s";
        else
            oss << minutes << "m " << setfill('0') << setw(2) << seconds << "s";
        return oss.str();
    }


    // === Data Persistence ===
    // Saves/loads favorites and playlists to "userdata.txt"

    void saveData() const {
        ofstream outFile(dataFilePath);
        if (!outFile) return;

        vector<int> favs = getFavorites();
        outFile << favs.size() << "\n";
        for (int idx : favs)
            outFile << library[idx].getFilePath() << "\n";

        outFile << playlists.size() << "\n";
        for (const auto& pl : playlists) {
            outFile << pl.name << "\n";
            outFile << pl.songIndices.size() << "\n";
            for (int idx : pl.songIndices)
                outFile << library[idx].getFilePath() << "\n";
        }
    }

    void loadData() {
        ifstream inFile(dataFilePath);
        if (!inFile) return;

        size_t favCount;
        if (!(inFile >> favCount)) return;
        inFile.ignore();

        for (size_t i = 0; i < favCount; ++i) {
            string path;
            getline(inFile, path);
            for (auto& s : library)
                if (s.getFilePath() == path) {
                    s.setFavorite(true);
                    break;
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
                for (size_t k = 0; k < library.size(); ++k)
                    if (library[k].getFilePath() == path) {
                        pl.songIndices.push_back(k);
                        break;
                    }
            }
            playlists.push_back(pl);
        }
    }
};
