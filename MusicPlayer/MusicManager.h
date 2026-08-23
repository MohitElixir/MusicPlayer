#ifndef MUSIC_MANAGER_H
#define MUSIC_MANAGER_H

#include <vector>
#include <string>
#include "Song.h"

struct Playlist {
    std::string name;
    std::vector<int> songIndices;
};

// ============================================================
// Member 2: Core Engine
// Owns the song library and provides search / playback logic.
// ============================================================
class MusicManager {
private:
    std::vector<Song> library;
    std::vector<Playlist> playlists;
    int nowPlayingIndex; // -1 if nothing is playing

public:
    MusicManager();

    // Track management
    void addSong(const Song& song);
    void addSong(const std::string& title, const std::string& artist, int durationSeconds, const std::string& filePath);
    int  getSongCount() const;
    bool hasSong(const std::string& title) const;
    void refreshFromFolder(const std::string& folderPath);
    bool renameSong(int index, const std::string& newFileName);

    // Access
    const std::vector<Song>& getLibrary() const;
    const Song* getSongAt(int index) const;

    // Search algorithms (case-insensitive substring match)
    std::vector<int> searchByTitle(const std::string& query) const;
    std::vector<int> searchByArtist(const std::string& query) const;
    std::vector<int> searchAll(const std::string& query) const;

    // Playlists & Favorites
    void toggleFavorite(int index);
    std::vector<int> getFavorites() const;

    void addPlaylist(const std::string& name);
    void addToPlaylist(int playlistIndex, int songIndex);
    const std::vector<Playlist>& getPlaylists() const;

    // Playback control
    bool play(int index);          // returns false if index invalid
    void stop();
    bool isPlaying() const;
    const Song* getNowPlaying() const;
    int getNowPlayingIndex() const;
    void playNext();
    void playPrevious();

    // Total library runtime, formatted "mm:ss"
    std::string getTotalRuntimeFormatted() const;
};

#endif // MUSIC_MANAGER_H
