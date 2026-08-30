#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>

// ============================================================
// Song: Concrete Class
// ============================================================
// Represents a single track: its metadata (title/artist/duration),
// where it lives on disk, and whether it's been favorited.
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
// A named, ordered set of indices into MusicManager's library.
// Kept as a plain struct (not a class) since it has no invariants
// to protect -- it's just a name plus a list of song positions.
struct Playlist {
    std::string name;
    std::vector<int> songIndices;
};

// ============================================================
// Song: Implementation
// ============================================================
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
