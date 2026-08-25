#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#undef max
#undef min
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#include "UIManager.h"
#include <iostream>
#include <limits>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <cstdio>
#ifndef _WIN32
#include <chrono>
#endif

using namespace std;

namespace {
#ifndef _WIN32
    chrono::steady_clock::time_point linuxPlaybackStartTime;
    int linuxCurrentDurationMs = 0;

    int getDurationMs(const string& filePath) {
        string cmd = "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"" + filePath + "\" 2>/dev/null";
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
                return static_cast<int>(seconds * 1000);
            } catch (...) {}
        }
        return 0;
    }
#endif

    const int UI_WIDTH = 64;

    // Helper to get visual width of UTF-8 strings (assuming 1 char = 1 width)
    int visualLength(const string& s) {
        int len = 0;
        for (size_t i = 0; i < s.length(); ) {
            unsigned char c = s[i];
            if ((c & 0x80) == 0) i += 1;
            else if ((c & 0xE0) == 0xC0) i += 2;
            else if ((c & 0xF0) == 0xE0) i += 3;
            else if ((c & 0xF8) == 0xF0) i += 4;
            else i++;
            len++;
        }
        return len;
    }

    // ── ANSI Escape Code Color Helpers ──
    const string ANSI_RESET  = "\x1B[0m";
    const string ANSI_GREEN  = "\x1B[92m"; // Bright Green
    const string ANSI_CYAN   = "\x1B[96m"; // Bright Cyan
    const string ANSI_RED    = "\x1B[91m"; // Bright Red
    const string ANSI_YELLOW = "\x1B[93m"; // Bright Yellow
    const string ANSI_WHITE  = "\x1B[97m"; // Bright White
    const string ANSI_GRAY   = "\x1B[90m"; // Bright Black / Gray

    void setColor(const string& ansiCode) {
        cout << ansiCode;
    }

    void resetColor() {
        cout << ANSI_RESET;
    }

    // Map old color constants to ANSI strings
    #define CLR_GREEN  ANSI_GREEN
    #define CLR_CYAN   ANSI_CYAN
    #define CLR_RED    ANSI_RED
    #define CLR_YELLOW ANSI_YELLOW
    #define CLR_WHITE  ANSI_WHITE
    #define CLR_GRAY   ANSI_GRAY
    #define CLR_DGREEN ANSI_GREEN // Mapping dark green to bright green for simplicity



    // Pad a string to a fixed width (truncate if too long)
    string pad(const string& s, int width) {
        if (static_cast<int>(s.size()) >= width) return s.substr(0, width);
        return s + string(width - s.size(), ' ');
    }

    // Format milliseconds as m:ss
    string formatTime(int ms) {
        int totalSec = ms / 1000;
        int m = totalSec / 60;
        int s = totalSec % 60;
        ostringstream oss;
        oss << m << ":" << setfill('0') << setw(2) << s;
        return oss.str();
    }

    // UTF-8 icon strings
    const char* ICON_NOTE    = "\xE2\x99\xAB";  // ♫
    const char* ICON_PLAY    = "\xE2\x96\xB6";  // ▶
    const char* ICON_STOP    = "\xE2\x96\xA0";  // ■
    const char* ICON_HEART   = "\xE2\x99\xA5";  // ♥
    const char* ICON_STAR    = "\xE2\x98\x85";  // ★
    const char* ICON_ARROW   = "\xE2\x96\xB8";  // ▸
    const char* ICON_SEARCH  = "\xE2\x97\x89";  // ◉

    // Box-drawing characters
    const char* BOX_TL = "\xE2\x95\x94"; // ╔
    const char* BOX_TR = "\xE2\x95\x97"; // ╗
    const char* BOX_BL = "\xE2\x95\x9A"; // ╚
    const char* BOX_BR = "\xE2\x95\x9D"; // ╝
    const char* BOX_H  = "\xE2\x95\x90"; // ═
    const char* BOX_V  = "\xE2\x95\x91"; // ║
    const char* BOX_LH = "\xE2\x94\x80"; // ─

    void hBar(int n, const char* ch = nullptr) {
        if (!ch) ch = BOX_H;
        for (int i = 0; i < n; ++i) cout << ch;
    }

    void closeRow(int used) {
        int remaining = UI_WIDTH - used;
        if (remaining > 0) cout << string(remaining, ' ');
        setColor(CLR_GREEN);
        cout << BOX_V << "\n";
    }

    void emptyRow() {
        setColor(CLR_GREEN);
        cout << "  " << BOX_V << string(UI_WIDTH, ' ') << BOX_V << "\n";
    }
}

void UIManager::clearScreen() {
    cout << "\x1B[H";
}

UIManager::UIManager(MusicManager& manager)
    : manager(manager), currentScreen(Screen::MENU), forceAudioRestart(false) {
#ifdef _WIN32
    SetConsoleOutputCP(65001); // Enable UTF-8 output

    // Enable Virtual Terminal Processing for ANSI escape code support
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        SetConsoleMode(hOut, dwMode | 0x0004); // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    }
#endif
}

void UIManager::drawDivider() const {
    setColor(CLR_GRAY);
    cout << "  ";
    hBar(UI_WIDTH, BOX_LH);
    cout << "\n";
    resetColor();
}

void UIManager::playSystemAudio(const Song* current) {
    if (!current) return;
#ifdef _WIN32
    mciSendStringA("close all", NULL, 0, NULL);
    string command = "open \"" + current->getFilePath() + "\" type mpegvideo alias mymusic";
    mciSendStringA(command.c_str(), NULL, 0, NULL);
    mciSendStringA("play mymusic", NULL, 0, NULL);
#else
    system("killall -9 ffplay > /dev/null 2>&1 || pkill -9 -f ffplay > /dev/null 2>&1");
    string command = "ffplay -nodisp -autoexit \"" + current->getFilePath() + "\" > /dev/null 2>&1 &";
    system(command.c_str());
    linuxPlaybackStartTime = chrono::steady_clock::now();
    linuxCurrentDurationMs = getDurationMs(current->getFilePath());
#endif
}

void UIManager::drawHeader(const string& title) const {
    clearScreen();
    cout << "\n";

    // Top border ╔════╗
    setColor(CLR_GREEN);
    cout << "  " << BOX_TL;
    hBar(UI_WIDTH);
    cout << BOX_TR << "\n";

    // Title row  ║  TITLE  ║
    int titleVisual = visualLength(title);
    int pad1 = (UI_WIDTH - titleVisual) / 2;
    int pad2 = UI_WIDTH - titleVisual - pad1;
    cout << "  " << BOX_V;
    setColor(CLR_WHITE);
    cout << string(pad1, ' ') << title
              << string(pad2, ' ');
    setColor(CLR_GREEN);
    cout << BOX_V << "\n";

    // Bottom border ╚════╝
    cout << "  " << BOX_BL;
    hBar(UI_WIDTH);
    cout << BOX_BR << "\n";
    resetColor();
}

void UIManager::drawFooter() const {
    setColor(CLR_GREEN);
    cout << "  " << BOX_BL;
    hBar(UI_WIDTH);
    cout << BOX_BR << "\n";
    resetColor();
}

void UIManager::pause() {
    setColor(CLR_GRAY);
    cout << "\n  Press ENTER to continue...\x1B[0J" << flush;
    resetColor();
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

int UIManager::readIntChoice() {
    cout << "\x1B[0J" << flush;
    int choice;
    cin >> choice;
    if (cin.fail()) {
        cin.clear();
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    return choice;
}

namespace {
    int readSingleKeyWithTimeout(int timeoutMs) {
        cout << "\x1B[0J" << flush;
        int elapsed = 0;
        while (elapsed < timeoutMs) {
#ifdef _WIN32
            if (_kbhit()) {
                char c = _getch();
                if (c >= '0' && c <= '9') return c - '0';
            }
            Sleep(50);
#else
            struct termios oldt, newt;
            tcgetattr(STDIN_FILENO, &oldt);
            newt = oldt;
            newt.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
            int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
            fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
            
            int ch = getchar();
            
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            fcntl(STDIN_FILENO, F_SETFL, oldf);
            
            if (ch != EOF && ch >= '0' && ch <= '9') {
                return ch - '0';
            }
            usleep(50000); // 50ms
#endif
            elapsed += 50;
        }
        return -1; // timeout
    }
}

void UIManager::showMenu() {
    manager.refreshFromFolder("Music");

    string title = string(ICON_NOTE) + "  MUSIC PLAYER  " + ICON_NOTE;
    drawHeader(title);
    emptyRow();

    if (manager.isPlaying()) {
        const Song* current = manager.getNowPlaying();
        
        // Top banner: Title with notes
        string notes1 = string(ICON_NOTE) + " ";
        string npText = "NOW PLAYING: ";
        string songTitle = current->getTitle();
        string notes2 = " " + string(ICON_NOTE);
        
        int textLen = visualLength(notes1) + visualLength(npText) + visualLength(songTitle) + visualLength(notes2);
        if (textLen > UI_WIDTH) {
            int availableForTitle = UI_WIDTH - visualLength(notes1) - visualLength(npText) - visualLength(notes2);
            songTitle = songTitle.substr(0, availableForTitle - 3) + "...";
            textLen = visualLength(notes1) + visualLength(npText) + visualLength(songTitle) + visualLength(notes2);
        }
        
        int padding = (UI_WIDTH - textLen) / 2;
        if (padding < 0) padding = 0;

        setColor(CLR_GREEN);
        cout << "  " << BOX_V;
        cout << string(padding, ' ');
        
        setColor(CLR_YELLOW);
        cout << notes1;

        setColor(CLR_CYAN);
        cout << npText;
        
        setColor(CLR_WHITE);
        cout << songTitle;

        setColor(CLR_YELLOW);
        cout << notes2;
        
        closeRow(padding + textLen);
        
        // Secondary line: Artist
        string artistText = "by " + current->getArtist();
        int artistLen = visualLength(artistText);
        if (artistLen > UI_WIDTH) {
            artistText = artistText.substr(0, UI_WIDTH - 3) + "...";
            artistLen = visualLength(artistText);
        }
        
        int artistPad = (UI_WIDTH - artistLen) / 2;
        if (artistPad < 0) artistPad = 0;

        setColor(CLR_GREEN);
        cout << "  " << BOX_V;
        cout << string(artistPad, ' ');
        
        setColor(CLR_GRAY);
        cout << artistText;
        closeRow(artistPad + artistLen);
        
        emptyRow();
    }

    auto menuItem = [&](const char* num, const char* icon, const string& iconColor, const string& text, const string& textColor) {
        setColor(CLR_GREEN);
        cout << "  " << BOX_V;
        int chars = 0;

        setColor(CLR_YELLOW);
        string numStr = string("   [") + num + "]  ";
        cout << numStr;
        chars += static_cast<int>(numStr.size());

        setColor(iconColor);
        cout << icon << " ";
        chars += 2;

        setColor(textColor);
        cout << text;
        chars += static_cast<int>(text.size());

        closeRow(chars);
    };

    menuItem("1", ICON_NOTE,   CLR_CYAN,   "View Library",  CLR_WHITE);
    if (manager.isPlaying()) {
        menuItem("2", ICON_PLAY,   CLR_GREEN,  "Now Playing",   CLR_WHITE);
        menuItem("3", ICON_SEARCH, CLR_CYAN,   "Search Songs",  CLR_WHITE);
        menuItem("4", ICON_HEART,  CLR_RED,    "Favorites",     CLR_RED);
        menuItem("5", ICON_STAR,   CLR_YELLOW, "Playlists",     CLR_WHITE);
        menuItem("6", ICON_STOP,   CLR_GRAY,   "Exit",          CLR_GRAY);
    } else {
        menuItem("2", ICON_SEARCH, CLR_CYAN,   "Search Songs",  CLR_WHITE);
        menuItem("3", ICON_HEART,  CLR_RED,    "Favorites",     CLR_RED);
        menuItem("4", ICON_STAR,   CLR_YELLOW, "Playlists",     CLR_WHITE);
        menuItem("5", ICON_STOP,   CLR_GRAY,   "Exit",          CLR_GRAY);
    }

    emptyRow();
    drawFooter();

    // Status bar
    setColor(CLR_GRAY);
    cout << "  | ";
    setColor(CLR_DGREEN);
    cout << manager.getSongCount() << " songs";
    setColor(CLR_GRAY);
    cout << " | Runtime: ";
    setColor(CLR_DGREEN);
    cout << manager.getTotalRuntimeFormatted() << "\n";

    cout << "\n\n\n";

    cout << "\n";
    setColor(CLR_CYAN);
    cout << "  " << ICON_ARROW << " ";
    resetColor();
    cout << "Select option: ";

    int choice = readIntChoice();
    if (manager.isPlaying()) {
        if (choice == 1)      currentScreen = Screen::LIBRARY;
        else if (choice == 2) currentScreen = Screen::NOW_PLAYING;
        else if (choice == 3) currentScreen = Screen::SEARCH;
        else if (choice == 4) currentScreen = Screen::FAVORITES;
        else if (choice == 5) currentScreen = Screen::PLAYLISTS;
        else if (choice == 6) {
            manager.stop();
#ifdef _WIN32
            mciSendStringA("close all", NULL, 0, NULL);
#else
            system("killall -9 ffplay > /dev/null 2>&1 || pkill -9 -f ffplay > /dev/null 2>&1");
#endif
            currentScreen = Screen::EXIT;
        }
    } else {
        if (choice == 1)      currentScreen = Screen::LIBRARY;
        else if (choice == 2) currentScreen = Screen::SEARCH;
        else if (choice == 3) currentScreen = Screen::FAVORITES;
        else if (choice == 4) currentScreen = Screen::PLAYLISTS;
        else if (choice == 5) {
            manager.stop();
#ifdef _WIN32
            mciSendStringA("close all", NULL, 0, NULL);
#else
            system("killall -9 ffplay > /dev/null 2>&1 || pkill -9 -f ffplay > /dev/null 2>&1");
#endif
            currentScreen = Screen::EXIT;
        }
    }
}

void UIManager::showLibrary() {
    manager.refreshFromFolder("Music");

    string title = string(ICON_NOTE) + "  SONG LIBRARY";
    drawHeader(title);

    const auto& songs = manager.getLibrary();
    if (songs.empty()) {
        setColor(CLR_GREEN);
        cout << "  " << BOX_V;
        setColor(CLR_GRAY);
        string msg = "   (Library is empty - add .mp3 files to Music folder)";
        cout << msg;
        closeRow(static_cast<int>(msg.size()));
    } else {
        for (size_t i = 0; i < songs.size(); ++i) {
            setColor(CLR_GREEN);
            cout << "  " << BOX_V;
            int chars = 0;

            if (manager.getNowPlayingIndex() == static_cast<int>(i)) {
                setColor(CLR_GREEN);
                cout << " " << ICON_PLAY << " ";
                chars += 3;
            } else {
                cout << "   ";
                chars += 3;
            }

            setColor(CLR_YELLOW);
            ostringstream num;
            num << "[" << i + 1 << "]";
            string numStr = num.str();
            cout << left << setw(5) << numStr;
            chars += 5;

            if (songs[i].isFavorite()) {
                setColor(CLR_RED);
                cout << ICON_HEART << " ";
                chars += 2;
            } else {
                cout << "  ";
                chars += 2;
            }

            setColor(CLR_WHITE);
            string entry = songs[i].getTitle() + " - " + songs[i].getArtist();
            int maxLen = UI_WIDTH - chars - 1;
            if (static_cast<int>(entry.size()) > maxLen) entry = entry.substr(0, maxLen);
            cout << entry;
            chars += static_cast<int>(entry.size());

            closeRow(chars);
        }
    }

    emptyRow();
    drawFooter();

    cout << "\n";
    setColor(CLR_CYAN);
    cout << "  " << ICON_ARROW << " ";
    resetColor();
    cout << "Song # to play, or 0 to go back: ";
    int choice = readIntChoice();

    if (choice == 0) {
        currentScreen = Screen::MENU;
        return;
    }
    if (choice >= 1 && choice <= static_cast<int>(songs.size())) {
        if (manager.getNowPlayingIndex() != choice - 1) forceAudioRestart = true;
        manager.play(choice - 1);
        currentScreen = Screen::NOW_PLAYING;
    } else {
        setColor(CLR_RED);
        cout << "  Invalid selection.\n";
        resetColor();
        pause();
    }
}

void UIManager::showSearch() {
    string title = string(ICON_SEARCH) + "  SEARCH SONGS";
    drawHeader(title);

    setColor(CLR_GREEN);
    cout << "  " << BOX_V;
    setColor(CLR_WHITE);
    string msg = "   Type a title or artist name:";
    cout << msg;
    closeRow(static_cast<int>(msg.size()));
    emptyRow();
    drawFooter();

    cout << "\n";
    setColor(CLR_CYAN);
    cout << "  " << ICON_ARROW << " ";
    resetColor();

    string query;
    cout << "\x1B[0J" << flush;
    getline(cin, query);

    auto results = manager.searchAll(query);

    string rTitle = string(ICON_SEARCH) + "  RESULTS";
    drawHeader(rTitle);

    if (results.empty()) {
        setColor(CLR_GREEN);
        cout << "  " << BOX_V;
        setColor(CLR_GRAY);
        string noMatch = "   No matches for \"" + query + "\"";
        cout << noMatch;
        closeRow(static_cast<int>(noMatch.size()));
        emptyRow();
        drawFooter();
        pause();
        currentScreen = Screen::MENU;
        return;
    }

    for (size_t i = 0; i < results.size(); ++i) {
        const Song* s = manager.getSongAt(results[i]);
        setColor(CLR_GREEN);
        cout << "  " << BOX_V;
        int chars = 0;

        setColor(CLR_YELLOW);
        ostringstream num;
        num << "   [" << i + 1 << "]  ";
        string numStr = num.str();
        cout << numStr;
        chars += static_cast<int>(numStr.size());

        setColor(CLR_WHITE);
        string entry = s->getTitle() + " - " + s->getArtist();
        cout << entry;
        chars += static_cast<int>(entry.size());

        closeRow(chars);
    }

    emptyRow();
    drawFooter();

    cout << "\n";
    setColor(CLR_CYAN);
    cout << "  " << ICON_ARROW << " ";
    resetColor();
    cout << "Song # to play, or 0 to go back: ";
    int choice = readIntChoice();

    if (choice > 0 && choice <= static_cast<int>(results.size())) {
        if (manager.getNowPlayingIndex() != results[choice - 1]) forceAudioRestart = true;
        manager.play(results[choice - 1]);
        currentScreen = Screen::NOW_PLAYING;
        return;
    } else {
        currentScreen = Screen::MENU;
    }
}

void UIManager::showNowPlaying() {
    string title = string(ICON_PLAY) + "  NOW PLAYING";
    drawHeader(title);

    const Song* current = manager.getNowPlaying();
    if (current == nullptr) {
        setColor(CLR_GREEN);
        cout << "  " << BOX_V;
        setColor(CLR_GRAY);
        string msg = "   Nothing is playing. Pick a song first!";
        cout << msg;
        closeRow(static_cast<int>(msg.size()));
        emptyRow();
        drawFooter();
        pause();
        currentScreen = Screen::MENU;
        return;
    }

    emptyRow();

    // Title
    cout << "  " << BOX_V;
    setColor(CLR_CYAN);
    cout << "   " << ICON_NOTE << " ";
    setColor(CLR_WHITE);
    cout << current->getTitle();
    closeRow(5 + visualLength(current->getTitle()));

    // Artist
    cout << "  " << BOX_V;
    setColor(CLR_GRAY);
    cout << "     " << current->getArtist();
    closeRow(5 + visualLength(current->getArtist()));

    emptyRow();

    // Progress details
    int lengthMs = 0;
    int positionMs = 0;
#ifdef _WIN32
    char lenBuf[128] = {0}, posBuf[128] = {0};
    mciSendStringA("status mymusic length", lenBuf, sizeof(lenBuf), NULL);
    mciSendStringA("status mymusic position", posBuf, sizeof(posBuf), NULL);
    lengthMs  = atoi(lenBuf);
    positionMs = atoi(posBuf);
#else
    lengthMs = linuxCurrentDurationMs;
    auto now = chrono::steady_clock::now();
    positionMs = static_cast<int>(chrono::duration_cast<chrono::milliseconds>(now - linuxPlaybackStartTime).count());
    if (positionMs > lengthMs && lengthMs > 0) positionMs = lengthMs;
#endif

    string timeL = formatTime(positionMs);
    string timeR = formatTime(lengthMs);

    cout << "  " << BOX_V;
    int chars = 0;
    setColor(CLR_GRAY);
    cout << "   " << timeL << " ";
    chars += 3 + static_cast<int>(timeL.size()) + 1;

    setColor(CLR_GREEN);
    cout << "[";
    chars += 1;

    const int BAR_WIDTH = 30;
    int filled = (lengthMs > 0) ? (positionMs * BAR_WIDTH / lengthMs) : 0;
    if (filled > BAR_WIDTH) filled = BAR_WIDTH;
    int empty = BAR_WIDTH - filled;

    for (int i = 0; i < filled; ++i) cout << "=";
    chars += filled;

    if (empty > 0) {
        setColor(CLR_WHITE);
        cout << ">";
        chars += 1;
        empty--;
    }

    setColor(CLR_GRAY);
    for (int i = 0; i < empty; ++i) cout << "-";
    chars += empty;

    setColor(CLR_GREEN);
    cout << "]";
    chars += 1;

    setColor(CLR_GRAY);
    cout << " " << timeR;
    chars += 1 + static_cast<int>(timeR.size());
    closeRow(chars);

    emptyRow();

    // Liked status
    cout << "  " << BOX_V;
    int c2 = 0;
    if (current->isFavorite()) {
        setColor(CLR_RED);
        cout << "   " << ICON_HEART << " Liked";
        c2 += 3 + 2 + 5;
    } else {
        setColor(CLR_GRAY);
        cout << "   Not Liked";
        c2 += 12;
    }
    closeRow(c2);

    emptyRow();
    drawFooter();

    // Trigger audio only if newly selected
    if (forceAudioRestart) {
        playSystemAudio(current);
        forceAudioRestart = false;
    }

    // Controls display
    drawDivider();
    setColor(CLR_GREEN);
    cout << "  " << BOX_V;
    int visualCc = 0;

    setColor(CLR_WHITE);
    cout << " ";
    visualCc += 1;

    setColor(CLR_YELLOW); cout << "[1]"; visualCc += 3;
    setColor(CLR_WHITE);  cout << ICON_STOP << " Stop "; visualCc += 7;

    setColor(CLR_YELLOW); cout << "[2]"; visualCc += 3;
    setColor(CLR_WHITE);  cout << ">> Next "; visualCc += 8;

    setColor(CLR_YELLOW); cout << "[3]"; visualCc += 3;
    setColor(CLR_WHITE);  cout << "<< Prev "; visualCc += 8;

    setColor(CLR_YELLOW); cout << "[4]"; visualCc += 3;
    setColor(CLR_RED);    cout << ICON_HEART << " Like "; visualCc += 7;

    setColor(CLR_YELLOW); cout << "[5]"; visualCc += 3;
    setColor(CLR_WHITE);  cout << "+List "; visualCc += 6;

    closeRow(visualCc);
    
    // Second row
    setColor(CLR_GREEN);
    cout << "  " << BOX_V;
    int visualCc2 = 0;
    setColor(CLR_WHITE);
    cout << " ";
    visualCc2 += 1;

    setColor(CLR_YELLOW); cout << "[6]"; visualCc2 += 3;
    setColor(CLR_WHITE);  cout << "Rename "; visualCc2 += 7;

    setColor(CLR_YELLOW); cout << "[0]"; visualCc2 += 3;
    setColor(CLR_GRAY);   cout << "Back"; visualCc2 += 4;

    closeRow(visualCc2);
    drawFooter();

    cout << "\n";
    setColor(CLR_CYAN);
    cout << "  " << ICON_ARROW << " ";
    resetColor();
    cout << "Choice: ";
    int choice = readSingleKeyWithTimeout(1000);
    if (choice == -1) {
        return; // timeout: loop around to redraw screen and update progress bar
    }

    if (choice == 1) {
        manager.stop();
#ifdef _WIN32
        mciSendStringA("close all", NULL, 0, NULL);
#else
        system("killall -9 ffplay > /dev/null 2>&1 || pkill -9 -f ffplay > /dev/null 2>&1");
#endif
        setColor(CLR_GRAY);
        cout << "  Playback stopped.\n";
        resetColor();
        pause();
        currentScreen = Screen::MENU;
    } else if (choice == 2) {
        forceAudioRestart = true;
        manager.playNext();
    } else if (choice == 3) {
        forceAudioRestart = true;
        manager.playPrevious();
    } else if (choice == 4) {
        manager.toggleFavorite(manager.getNowPlayingIndex());
    } else if (choice == 5) {
        cout << "\n";
        setColor(CLR_WHITE);
        cout << "  Available Playlists:\n";
        auto playlists = manager.getPlaylists();
        if (playlists.empty()) {
            setColor(CLR_GRAY);
            cout << "  (No playlists yet)\n";
            resetColor();
            pause();
        } else {
            for (size_t i = 0; i < playlists.size(); ++i) {
                setColor(CLR_YELLOW);
                cout << "  [" << i + 1 << "] ";
                setColor(CLR_WHITE);
                cout << playlists[i].name << "\n";
            }
            resetColor();
            cout << "  Playlist #: ";
            int plChoice = readIntChoice();
            if (plChoice > 0 && plChoice <= static_cast<int>(playlists.size())) {
                manager.addToPlaylist(plChoice - 1, manager.getNowPlayingIndex());
                setColor(CLR_GREEN);
                cout << "  Added to playlist!\n";
                resetColor();
                pause();
            }
        }
    } else if (choice == 6) {
        cout << "\n";
        setColor(CLR_CYAN);
        cout << "  " << ICON_ARROW << " ";
        resetColor();
        cout << "New File Name (no .mp3): \x1B[0J" << flush;
        
        string newName;
        getline(cin, newName);
        if (!newName.empty()) {
#ifdef _WIN32
            mciSendStringA("close all", NULL, 0, NULL);
#else
            system("killall -9 ffplay > /dev/null 2>&1 || pkill -9 -f ffplay > /dev/null 2>&1");
#endif
            if (manager.renameSong(manager.getNowPlayingIndex(), newName)) {
                setColor(CLR_GREEN);
                cout << "  Song renamed successfully!\n";
            } else {
                setColor(CLR_RED);
                cout << "  Failed to rename file.\n";
            }
            forceAudioRestart = true;
        }
        resetColor();
        pause();
    } else if (choice == 0) {
        currentScreen = Screen::MENU;
    }
}

void UIManager::showFavorites() {
    string title = string(ICON_HEART) + "  FAVORITES";
    drawHeader(title);

    auto favs = manager.getFavorites();
    if (favs.empty()) {
        setColor(CLR_GREEN);
        cout << "  " << BOX_V;
        setColor(CLR_GRAY);
        string msg = "   No favorites yet. Like songs with [4] in Now Playing!";
        cout << msg;
        closeRow(static_cast<int>(msg.size()));
    } else {
        for (size_t i = 0; i < favs.size(); ++i) {
            setColor(CLR_GREEN);
            cout << "  " << BOX_V;
            int chars = 0;

            setColor(CLR_RED);
            cout << " " << ICON_HEART << " ";
            chars += 3;

            setColor(CLR_YELLOW);
            ostringstream num;
            num << "[" << i + 1 << "]";
            cout << left << setw(5) << num.str();
            chars += 5;

            setColor(CLR_WHITE);
            string entry = manager.getSongAt(favs[i])->getTitle()
                + " - " + manager.getSongAt(favs[i])->getArtist();
            cout << entry;
            chars += static_cast<int>(entry.size());

            closeRow(chars);
        }
    }

    emptyRow();
    drawFooter();

    cout << "\n";
    setColor(CLR_CYAN);
    cout << "  " << ICON_ARROW << " ";
    resetColor();
    cout << "Song # to play, or 0 to go back: ";
    int choice = readIntChoice();
    if (choice > 0 && choice <= static_cast<int>(favs.size())) {
        if (manager.getNowPlayingIndex() != favs[choice - 1]) forceAudioRestart = true;
        manager.play(favs[choice - 1]);
        currentScreen = Screen::NOW_PLAYING;
        return;
    } else {
        currentScreen = Screen::MENU;
    }
}

void UIManager::showPlaylists() {
    string title = string(ICON_STAR) + "  PLAYLISTS";
    drawHeader(title);

    auto playlists = manager.getPlaylists();

    setColor(CLR_GREEN);
    cout << "  " << BOX_V;
    setColor(CLR_YELLOW);
    cout << "   [1]  ";
    setColor(CLR_CYAN);
    string createText = "+ Create New Playlist";
    cout << createText;
    closeRow(8 + static_cast<int>(createText.size()));

    for (size_t i = 0; i < playlists.size(); ++i) {
        setColor(CLR_GREEN);
        cout << "  " << BOX_V;
        int chars = 0;

        setColor(CLR_YELLOW);
        ostringstream num;
        num << "   [" << i + 2 << "]  ";
        string numStr = num.str();
        cout << numStr;
        chars += static_cast<int>(numStr.size());

        setColor(CLR_WHITE);
        ostringstream entry;
        entry << playlists[i].name << " (" << playlists[i].songIndices.size() << " songs)";
        string entryStr = entry.str();
        cout << entryStr;
        chars += static_cast<int>(entryStr.size());

        closeRow(chars);
    }

    emptyRow();
    drawFooter();

    cout << "\n";
    setColor(CLR_CYAN);
    cout << "  " << ICON_ARROW << " ";
    resetColor();
    cout << "Select, or 0 to go back: ";
    int choice = readIntChoice();

    if (choice == 1) {
        cout << "\n";
        setColor(CLR_CYAN);
        cout << "  " << ICON_ARROW << " ";
        resetColor();
        cout << "  Playlist name: \x1B[0J" << flush;
        string name;
        getline(cin, name);
        manager.addPlaylist(name);
        setColor(CLR_GREEN);
        cout << "  Created \"" << name << "\"\n";
        resetColor();
        pause();
    } else if (choice > 1 && choice <= static_cast<int>(playlists.size()) + 1) {
        int plIndex = choice - 2;
        const auto& pl = playlists[plIndex];

        string plTitle = string(ICON_STAR) + "  " + pl.name;
        drawHeader(plTitle);

        if (pl.songIndices.empty()) {
            setColor(CLR_GREEN);
            cout << "  " << BOX_V;
            setColor(CLR_GRAY);
            string msg = "   (Empty playlist)";
            cout << msg;
            closeRow(static_cast<int>(msg.size()));
        } else {
            for (size_t i = 0; i < pl.songIndices.size(); ++i) {
                setColor(CLR_GREEN);
                cout << "  " << BOX_V;
                int chars = 0;

                setColor(CLR_YELLOW);
                ostringstream num;
                num << "   [" << i + 1 << "]  ";
                string numStr = num.str();
                cout << numStr;
                chars += static_cast<int>(numStr.size());

                setColor(CLR_WHITE);
                string entry = manager.getSongAt(pl.songIndices[i])->getTitle();
                cout << entry;
                chars += static_cast<int>(entry.size());

                closeRow(chars);
            }
        }

        emptyRow();
        drawFooter();

        cout << "\n";
        setColor(CLR_CYAN);
        cout << "  " << ICON_ARROW << " ";
        resetColor();
        cout << "Song # to play, or 0 to go back: ";
        int sChoice = readIntChoice();
        if (sChoice > 0 && sChoice <= static_cast<int>(pl.songIndices.size())) {
            if (manager.getNowPlayingIndex() != pl.songIndices[sChoice - 1]) forceAudioRestart = true;
            manager.play(pl.songIndices[sChoice - 1]);
            currentScreen = Screen::NOW_PLAYING;
            return;
        }
    } else if (choice == 0) {
        currentScreen = Screen::MENU;
    }
}

void UIManager::run() {
    Screen lastScreen = Screen::EXIT;
    while (currentScreen != Screen::EXIT) {
        if (currentScreen != lastScreen) {
            cout << "\x1B[2J\x1B[H";
            lastScreen = currentScreen;
        }
        switch (currentScreen) {
            case Screen::MENU:        showMenu();        break;
            case Screen::LIBRARY:     showLibrary();     break;
            case Screen::SEARCH:      showSearch();      break;
            case Screen::NOW_PLAYING: showNowPlaying();  break;
            case Screen::FAVORITES:   showFavorites();   break;
            case Screen::PLAYLISTS:   showPlaylists();   break;
            default: currentScreen = Screen::EXIT;        break;
        }
    }
    cout << "\x1B[2J\x1B[3J\x1B[H" << flush;
    cout << "\n\n";
    setColor(CLR_GREEN);
    cout << "  " << ICON_NOTE << "  Thanks for using Music Player. Goodbye!  " << ICON_NOTE << "\n\n";
    resetColor();
}
