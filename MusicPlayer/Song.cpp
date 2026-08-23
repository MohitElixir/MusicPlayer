#include "Song.h"
#include <sstream>
#include <iomanip>

Song::Song()
    : title("Unknown Title"), artist("Unknown Artist"), durationSeconds(0), favorite(false) {}

Song::Song(const std::string& title, const std::string& artist, int durationSeconds, const std::string& filePath)
    : title(title), artist(artist), durationSeconds(durationSeconds), filePath(filePath), favorite(false) {}

std::string Song::getTitle() const { return title; }
std::string Song::getArtist() const { return artist; }
int Song::getDurationSeconds() const { return durationSeconds; }
std::string Song::getFilePath() const { return filePath; }
bool Song::isFavorite() const { return favorite; }

void Song::setFavorite(bool fav) {
    favorite = fav;
}

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

std::string Song::toString() const {
    std::ostringstream oss;
    if (favorite) oss << "[<3] ";
    oss << title << " - " << artist << " (" << getFormattedDuration() << ")";
    return oss.str();
}
