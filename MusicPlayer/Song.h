#ifndef SONG_H
#define SONG_H

#include <string>

// ============================================================
// Member 1: Data Model
// Represents a single track's metadata.
// ============================================================
class Song {
private:
    std::string title;
    std::string artist;
    int durationSeconds; // stored in raw seconds, formatted on output
    std::string filePath;
    bool favorite;

public:
    // Constructors
    Song();
    Song(const std::string& title, const std::string& artist, int durationSeconds, const std::string& filePath);

    // Getters
    std::string getTitle() const;
    std::string getArtist() const;
    int getDurationSeconds() const;
    std::string getFilePath() const;
    bool isFavorite() const;

    // Setters
    void setFavorite(bool fav);

    // Utility
    std::string getFormattedDuration() const; // "mm:ss"
    std::string toString() const;             // "[<3] Title - Artist (mm:ss)"
};

#endif // SONG_H
