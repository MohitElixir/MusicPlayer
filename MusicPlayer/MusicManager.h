#ifndef MUSIC_MANAGER_H
#define MUSIC_MANAGER_H

#include <vector>
#include <string>
#include "Song.h"


using namespace std;

struct Playlist {
    string name;
    vector<int> songIndices;
};

// ============================================================
// Member 2: Core Engine
// Owns the song library and provides search / playback logic.
// ============================================================
class MusicManager {
private:
    vector<Song> library;
    vector<Playlist> playlists;
    int nowPlayingIndex; // -1 if nothing is playing

public:
    MusicManager();

    // Track management
    void addSong(const Song& song);
    void addSong(const string& title, const string& artist, int durationSeconds, const string& filePath);
    int  getSongCount() const;
    bool hasSong(const string& title) const;
    void refreshFromFolder(const string& folderPath);
    bool renameSong(int index, const string& newFileName);

    // Access
    const vector<Song>& getLibrary() const;
    const Song* getSongAt(int index) const;

    // Search algorithms (case-insensitive substring match)
    vector<int> searchByTitle(const string& query) const;
    vector<int> searchByArtist(const string& query) const;
    vector<int> searchAll(const string& query) const;

    // Playlists & Favorites
    void toggleFavorite(int index);
    vector<int> getFavorites() const;

    void addPlaylist(const string& name);
    void addToPlaylist(int playlistIndex, int songIndex);
    const vector<Playlist>& getPlaylists() const;

    // Playback control
    bool play(int index);          // returns false if index invalid
    void stop();
    bool isPlaying() const;
    const Song* getNowPlaying() const;
    int getNowPlayingIndex() const;
    void playNext();
    void playPrevious();

    // Total library runtime, formatted "mm:ss"
    string getTotalRuntimeFormatted() const;
};

#endif // MUSIC_MANAGER_H
