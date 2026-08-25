#include "Song.h"
#include <sstream>
#include <iomanip>


using namespace std;

Song::Song()
    : title("Unknown Title"), artist("Unknown Artist"), durationSeconds(0), favorite(false) {}

Song::Song(const string& title, const string& artist, int durationSeconds, const string& filePath)
    : title(title), artist(artist), durationSeconds(durationSeconds), filePath(filePath), favorite(false) {}

string Song::getTitle() const { return title; }
string Song::getArtist() const { return artist; }
int Song::getDurationSeconds() const { return durationSeconds; }
string Song::getFilePath() const { return filePath; }
bool Song::isFavorite() const { return favorite; }

void Song::setFavorite(bool fav) {
    favorite = fav;
}

void Song::setTitle(const string& t) { title = t; }
void Song::setArtist(const string& a) { artist = a; }
void Song::setFilePath(const string& path) { filePath = path; }

string Song::getFormattedDuration() const {
    int minutes = durationSeconds / 60;
    int seconds = durationSeconds % 60;

    ostringstream oss;
    oss << minutes << ":" << setfill('0') << setw(2) << seconds;
    return oss.str();
}

string Song::toString() const {
    ostringstream oss;
    if (favorite) oss << "[<3] ";
    oss << title << " - " << artist << " (" << getFormattedDuration() << ")";
    return oss.str();
}
