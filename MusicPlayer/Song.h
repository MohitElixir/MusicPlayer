#ifndef SONG_H
#define SONG_H

#include <string>


// ============================================================
// Member 1: Data Model
// Represents a single track's metadata.
// ============================================================
using namespace std;

class Song {
private:
    string title;
    string artist;
    int durationSeconds; // stored in raw seconds, formatted on output
    string filePath;
    bool favorite;

public:
    // Constructors
    Song();
    Song(const string& title, const string& artist, int durationSeconds, const string& filePath);

    // Getters
    string getTitle() const;
    string getArtist() const;
    int getDurationSeconds() const;
    string getFilePath() const;
    bool isFavorite() const;

    // Setters
    void setFavorite(bool fav);
    void setTitle(const string& t);
    void setArtist(const string& a);
    void setFilePath(const string& path);

    // Utility
    string getFormattedDuration() const; // "mm:ss"
    string toString() const;             // "[<3] Title - Artist (mm:ss)"
};

#endif // SONG_H
