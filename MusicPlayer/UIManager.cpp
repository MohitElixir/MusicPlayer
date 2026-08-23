#include "UIManager.h"
#include <iostream>
#include <limits>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <chrono>
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

namespace {
#ifndef _WIN32
    std::chrono::steady_clock::time_point linuxPlaybackStartTime;
    int linuxCurrentDurationMs = 0;

    int getDurationMs(const std::string& filePath) {
        std::string cmd = "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"" + filePath + "\" 2>/dev/null";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return 0;
        char buffer[128];
        std::string result = "";
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        if (!result.empty()) {
            try {
                double seconds = std::stod(result);
                return static_cast<int>(seconds * 1000);
            } catch (...) {}
        }
        return 0;
    }
#endif

    const int UI_WIDTH = 64;

    // Helper to get visual width of UTF-8 strings (assuming 1 char = 1 width)
    int visualLength(const std::string& s) {
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
    const std::string ANSI_RESET  = "\x1B[0m";
    const std::string ANSI_GREEN  = "\x1B[92m"; // Bright Green
    const std::string ANSI_CYAN   = "\x1B[96m"; // Bright Cyan
    const std::string ANSI_RED    = "\x1B[91m"; // Bright Red
    const std::string ANSI_YELLOW = "\x1B[93m"; // Bright Yellow
    const std::string ANSI_WHITE  = "\x1B[97m"; // Bright White
    const std::string ANSI_GRAY   = "\x1B[90m"; // Bright Black / Gray

    void setColor(const std::string& ansiCode) {
        std::cout << ansiCode;
    }

    void resetColor() {
        std::cout << ANSI_RESET;
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
    std::string pad(const std::string& s, int width) {
        if (static_cast<int>(s.size()) >= width) return s.substr(0, width);
        return s + std::string(width - s.size(), ' ');
    }

    // Format milliseconds as m:ss
    std::string formatTime(int ms) {
        int totalSec = ms / 1000;
        int m = totalSec / 60;
        int s = totalSec % 60;
        std::ostringstream oss;
        oss << m << ":" << std::setfill('0') << std::setw(2) << s;
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
        for (int i = 0; i < n; ++i) std::cout << ch;
    }

    void closeRow(int used) {
        int remaining = UI_WIDTH - used;
        if (remaining > 0) std::cout << std::string(remaining, ' ');
        setColor(CLR_GREEN);
        std::cout << BOX_V << "\n";
    }

    void emptyRow() {
        setColor(CLR_GREEN);
        std::cout << "  " << BOX_V << std::string(UI_WIDTH, ' ') << BOX_V << "\n";
    }
}

void UIManager::clearScreen() {
    std::cout << "\x1B[2J\x1B[3J\x1B[H" << std::flush;
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
    std::cout << "  ";
    hBar(UI_WIDTH, BOX_LH);
    std::cout << "\n";
    resetColor();
}

void UIManager::playSystemAudio(const Song* current) {
    if (!current) return;
#ifdef _WIN32
    mciSendStringA("close all", NULL, 0, NULL);
    std::string command = "open \"" + current->getFilePath() + "\" type mpegvideo alias mymusic";
    mciSendStringA(command.c_str(), NULL, 0, NULL);
    mciSendStringA("play mymusic", NULL, 0, NULL);
#else
    system("pkill -f ffplay > /dev/null 2>&1");
    std::string command = "ffplay -nodisp -autoexit \"" + current->getFilePath() + "\" > /dev/null 2>&1 &";
    system(command.c_str());
    linuxPlaybackStartTime = std::chrono::steady_clock::now();
    linuxCurrentDurationMs = getDurationMs(current->getFilePath());
#endif
}

void UIManager::drawHeader(const std::string& title) const {
    clearScreen();
    std::cout << "\n";

    // Top border ╔════╗
    setColor(CLR_GREEN);
    std::cout << "  " << BOX_TL;
    hBar(UI_WIDTH);
    std::cout << BOX_TR << "\n";

    // Title row  ║  TITLE  ║
    int titleVisual = visualLength(title);
    int pad1 = (UI_WIDTH - titleVisual) / 2;
    int pad2 = UI_WIDTH - titleVisual - pad1;
    std::cout << "  " << BOX_V;
    setColor(CLR_WHITE);
    std::cout << std::string(pad1, ' ') << title
              << std::string(pad2, ' ');
    setColor(CLR_GREEN);
    std::cout << BOX_V << "\n";

    // Bottom border ╚════╝
    std::cout << "  " << BOX_BL;
    hBar(UI_WIDTH);
    std::cout << BOX_BR << "\n";
    resetColor();
}

void UIManager::drawFooter() const {
    setColor(CLR_GREEN);
    std::cout << "  " << BOX_BL;
    hBar(UI_WIDTH);
    std::cout << BOX_BR << "\n";
    resetColor();
}

void UIManager::pause() {
    setColor(CLR_GRAY);
    std::cout << "\n  Press ENTER to continue...";
    resetColor();
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

int UIManager::readIntChoice() {
    int choice;
    std::cin >> choice;
    if (std::cin.fail()) {
        std::cin.clear();
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return choice;
}

namespace {
    int readSingleKeyWithTimeout(int timeoutMs) {
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

    std::string title = std::string(ICON_NOTE) + "  MUSIC PLAYER  " + ICON_NOTE;
    drawHeader(title);
    emptyRow();

    auto menuItem = [&](const char* num, const char* icon, const std::string& iconColor, const std::string& text, const std::string& textColor) {
        setColor(CLR_GREEN);
        std::cout << "  " << BOX_V;
        int chars = 0;

        setColor(CLR_YELLOW);
        std::string numStr = std::string("   [") + num + "]  ";
        std::cout << numStr;
        chars += static_cast<int>(numStr.size());

        setColor(iconColor);
        std::cout << icon << " ";
        chars += 2;

        setColor(textColor);
        std::cout << text;
        chars += static_cast<int>(text.size());

        closeRow(chars);
    };

    menuItem("1", ICON_NOTE,   CLR_CYAN,   "View Library",  CLR_WHITE);
    menuItem("2", ICON_SEARCH, CLR_CYAN,   "Search Songs",  CLR_WHITE);
    menuItem("3", ICON_PLAY,   CLR_GREEN,  "Now Playing",   CLR_WHITE);
    menuItem("4", ICON_HEART,  CLR_RED,    "Favorites",     CLR_RED);
    menuItem("5", ICON_STAR,   CLR_YELLOW, "Playlists",     CLR_WHITE);
    menuItem("6", ICON_STOP,   CLR_GRAY,   "Exit",          CLR_GRAY);

    emptyRow();
    drawFooter();

    // Status bar
    setColor(CLR_GRAY);
    std::cout << "  | ";
    setColor(CLR_DGREEN);
    std::cout << manager.getSongCount() << " songs";
    setColor(CLR_GRAY);
    std::cout << " | Runtime: ";
    setColor(CLR_DGREEN);
    std::cout << manager.getTotalRuntimeFormatted() << "\n";

    std::cout << "\n\n\n";

    std::cout << "\n";
    setColor(CLR_CYAN);
    std::cout << "  " << ICON_ARROW << " ";
    resetColor();
    std::cout << "Select option: ";

    int choice = readIntChoice();
    if (choice == 1)      currentScreen = Screen::LIBRARY;
    else if (choice == 2) currentScreen = Screen::SEARCH;
    else if (choice == 3) currentScreen = Screen::NOW_PLAYING;
    else if (choice == 4) currentScreen = Screen::FAVORITES;
    else if (choice == 5) currentScreen = Screen::PLAYLISTS;
    else if (choice == 6) currentScreen = Screen::EXIT;
}

void UIManager::showLibrary() {
    manager.refreshFromFolder("E:\\MusicPlayer\\MusicPlayer\\Music");

    std::string title = std::string(ICON_NOTE) + "  SONG LIBRARY";
    drawHeader(title);

    const auto& songs = manager.getLibrary();
    if (songs.empty()) {
        setColor(CLR_GREEN);
        std::cout << "  " << BOX_V;
        setColor(CLR_GRAY);
        std::string msg = "   (Library is empty - add .mp3 files to Music folder)";
        std::cout << msg;
        closeRow(static_cast<int>(msg.size()));
    } else {
        for (size_t i = 0; i < songs.size(); ++i) {
            setColor(CLR_GREEN);
            std::cout << "  " << BOX_V;
            int chars = 0;

            if (manager.getNowPlayingIndex() == static_cast<int>(i)) {
                setColor(CLR_GREEN);
                std::cout << " " << ICON_PLAY << " ";
                chars += 3;
            } else {
                std::cout << "   ";
                chars += 3;
            }

            setColor(CLR_YELLOW);
            std::ostringstream num;
            num << "[" << i + 1 << "]";
            std::string numStr = num.str();
            std::cout << std::left << std::setw(5) << numStr;
            chars += 5;

            if (songs[i].isFavorite()) {
                setColor(CLR_RED);
                std::cout << ICON_HEART << " ";
                chars += 2;
            } else {
                std::cout << "  ";
                chars += 2;
            }

            setColor(CLR_WHITE);
            std::string entry = songs[i].getTitle() + " - " + songs[i].getArtist();
            int maxLen = UI_WIDTH - chars - 1;
            if (static_cast<int>(entry.size()) > maxLen) entry = entry.substr(0, maxLen);
            std::cout << entry;
            chars += static_cast<int>(entry.size());

            closeRow(chars);
        }
    }

    emptyRow();
    drawFooter();

    std::cout << "\n";
    setColor(CLR_CYAN);
    std::cout << "  " << ICON_ARROW << " ";
    resetColor();
    std::cout << "Song # to play, or 0 to go back: ";
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
        std::cout << "  Invalid selection.\n";
        resetColor();
        pause();
    }
}

void UIManager::showSearch() {
    std::string title = std::string(ICON_SEARCH) + "  SEARCH SONGS";
    drawHeader(title);

    setColor(CLR_GREEN);
    std::cout << "  " << BOX_V;
    setColor(CLR_WHITE);
    std::string msg = "   Type a title or artist name:";
    std::cout << msg;
    closeRow(static_cast<int>(msg.size()));
    emptyRow();
    drawFooter();

    std::cout << "\n";
    setColor(CLR_CYAN);
    std::cout << "  " << ICON_ARROW << " ";
    resetColor();

    std::string query;
    std::getline(std::cin, query);

    auto results = manager.searchAll(query);

    std::string rTitle = std::string(ICON_SEARCH) + "  RESULTS";
    drawHeader(rTitle);

    if (results.empty()) {
        setColor(CLR_GREEN);
        std::cout << "  " << BOX_V;
        setColor(CLR_GRAY);
        std::string noMatch = "   No matches for \"" + query + "\"";
        std::cout << noMatch;
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
        std::cout << "  " << BOX_V;
        int chars = 0;

        setColor(CLR_YELLOW);
        std::ostringstream num;
        num << "   [" << i + 1 << "]  ";
        std::string numStr = num.str();
        std::cout << numStr;
        chars += static_cast<int>(numStr.size());

        setColor(CLR_WHITE);
        std::string entry = s->getTitle() + " - " + s->getArtist();
        std::cout << entry;
        chars += static_cast<int>(entry.size());

        closeRow(chars);
    }

    emptyRow();
    drawFooter();

    std::cout << "\n";
    setColor(CLR_CYAN);
    std::cout << "  " << ICON_ARROW << " ";
    resetColor();
    std::cout << "Song # to play, or 0 to go back: ";
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
    std::string title = std::string(ICON_PLAY) + "  NOW PLAYING";
    drawHeader(title);

    const Song* current = manager.getNowPlaying();
    if (current == nullptr) {
        setColor(CLR_GREEN);
        std::cout << "  " << BOX_V;
        setColor(CLR_GRAY);
        std::string msg = "   Nothing is playing. Pick a song first!";
        std::cout << msg;
        closeRow(static_cast<int>(msg.size()));
        emptyRow();
        drawFooter();
        pause();
        currentScreen = Screen::MENU;
        return;
    }

    emptyRow();

    // Title
    std::cout << "  " << BOX_V;
    setColor(CLR_CYAN);
    std::cout << "   " << ICON_NOTE << " ";
    setColor(CLR_WHITE);
    std::cout << current->getTitle();
    closeRow(5 + visualLength(current->getTitle()));

    // Artist
    std::cout << "  " << BOX_V;
    setColor(CLR_GRAY);
    std::cout << "     " << current->getArtist();
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
    auto now = std::chrono::steady_clock::now();
    positionMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now - linuxPlaybackStartTime).count());
    if (positionMs > lengthMs && lengthMs > 0) positionMs = lengthMs;
#endif

    std::string timeL = formatTime(positionMs);
    std::string timeR = formatTime(lengthMs);

    std::cout << "  " << BOX_V;
    int chars = 0;
    setColor(CLR_GRAY);
    std::cout << "   " << timeL << " ";
    chars += 3 + static_cast<int>(timeL.size()) + 1;

    setColor(CLR_GREEN);
    std::cout << "[";
    chars += 1;

    const int BAR_WIDTH = 30;
    int filled = (lengthMs > 0) ? (positionMs * BAR_WIDTH / lengthMs) : 0;
    if (filled > BAR_WIDTH) filled = BAR_WIDTH;
    int empty = BAR_WIDTH - filled;

    for (int i = 0; i < filled; ++i) std::cout << "=";
    chars += filled;

    if (empty > 0) {
        setColor(CLR_WHITE);
        std::cout << ">";
        chars += 1;
        empty--;
    }

    setColor(CLR_GRAY);
    for (int i = 0; i < empty; ++i) std::cout << "-";
    chars += empty;

    setColor(CLR_GREEN);
    std::cout << "]";
    chars += 1;

    setColor(CLR_GRAY);
    std::cout << " " << timeR;
    chars += 1 + static_cast<int>(timeR.size());
    closeRow(chars);

    emptyRow();

    // Liked status
    std::cout << "  " << BOX_V;
    int c2 = 0;
    if (current->isFavorite()) {
        setColor(CLR_RED);
        std::cout << "   " << ICON_HEART << " Liked";
        c2 += 3 + 2 + 5;
    } else {
        setColor(CLR_GRAY);
        std::cout << "   Not Liked";
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
    std::cout << "  " << BOX_V;
    int visualCc = 0;

    setColor(CLR_WHITE);
    std::cout << " ";
    visualCc += 1;

    setColor(CLR_YELLOW); std::cout << "[1]"; visualCc += 3;
    setColor(CLR_WHITE);  std::cout << ICON_STOP << " Stop "; visualCc += 7;

    setColor(CLR_YELLOW); std::cout << "[2]"; visualCc += 3;
    setColor(CLR_WHITE);  std::cout << ">> Next "; visualCc += 8;

    setColor(CLR_YELLOW); std::cout << "[3]"; visualCc += 3;
    setColor(CLR_WHITE);  std::cout << "<< Prev "; visualCc += 8;

    setColor(CLR_YELLOW); std::cout << "[4]"; visualCc += 3;
    setColor(CLR_RED);    std::cout << ICON_HEART << " Like "; visualCc += 7;

    setColor(CLR_YELLOW); std::cout << "[5]"; visualCc += 3;
    setColor(CLR_WHITE);  std::cout << "+List "; visualCc += 6;

    setColor(CLR_YELLOW); std::cout << "[0]"; visualCc += 3;
    setColor(CLR_GRAY);   std::cout << "Back"; visualCc += 4;

    closeRow(visualCc);
    drawFooter();

    std::cout << "\n";
    setColor(CLR_CYAN);
    std::cout << "  " << ICON_ARROW << " ";
    resetColor();
    std::cout << "Choice: ";
    int choice = readSingleKeyWithTimeout(1000);
    if (choice == -1) {
        return; // timeout: loop around to redraw screen and update progress bar
    }

    if (choice == 1) {
        manager.stop();
#ifdef _WIN32
        mciSendStringA("close all", NULL, 0, NULL);
#else
        system("pkill -f ffplay > /dev/null 2>&1");
#endif
        setColor(CLR_GRAY);
        std::cout << "  Playback stopped.\n";
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
        std::cout << "\n";
        setColor(CLR_WHITE);
        std::cout << "  Available Playlists:\n";
        auto playlists = manager.getPlaylists();
        if (playlists.empty()) {
            setColor(CLR_GRAY);
            std::cout << "  (No playlists yet)\n";
            resetColor();
            pause();
        } else {
            for (size_t i = 0; i < playlists.size(); ++i) {
                setColor(CLR_YELLOW);
                std::cout << "  [" << i + 1 << "] ";
                setColor(CLR_WHITE);
                std::cout << playlists[i].name << "\n";
            }
            resetColor();
            std::cout << "  Playlist #: ";
            int plChoice = readIntChoice();
            if (plChoice > 0 && plChoice <= static_cast<int>(playlists.size())) {
                manager.addToPlaylist(plChoice - 1, manager.getNowPlayingIndex());
                setColor(CLR_GREEN);
                std::cout << "  Added to playlist!\n";
                resetColor();
                pause();
            }
        }
    } else if (choice == 0) {
        currentScreen = Screen::MENU;
    }
}

void UIManager::showFavorites() {
    std::string title = std::string(ICON_HEART) + "  FAVORITES";
    drawHeader(title);

    auto favs = manager.getFavorites();
    if (favs.empty()) {
        setColor(CLR_GREEN);
        std::cout << "  " << BOX_V;
        setColor(CLR_GRAY);
        std::string msg = "   No favorites yet. Like songs with [4] in Now Playing!";
        std::cout << msg;
        closeRow(static_cast<int>(msg.size()));
    } else {
        for (size_t i = 0; i < favs.size(); ++i) {
            setColor(CLR_GREEN);
            std::cout << "  " << BOX_V;
            int chars = 0;

            setColor(CLR_RED);
            std::cout << " " << ICON_HEART << " ";
            chars += 3;

            setColor(CLR_YELLOW);
            std::ostringstream num;
            num << "[" << i + 1 << "]";
            std::cout << std::left << std::setw(5) << num.str();
            chars += 5;

            setColor(CLR_WHITE);
            std::string entry = manager.getSongAt(favs[i])->getTitle()
                + " - " + manager.getSongAt(favs[i])->getArtist();
            std::cout << entry;
            chars += static_cast<int>(entry.size());

            closeRow(chars);
        }
    }

    emptyRow();
    drawFooter();

    std::cout << "\n";
    setColor(CLR_CYAN);
    std::cout << "  " << ICON_ARROW << " ";
    resetColor();
    std::cout << "Song # to play, or 0 to go back: ";
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
    std::string title = std::string(ICON_STAR) + "  PLAYLISTS";
    drawHeader(title);

    auto playlists = manager.getPlaylists();

    setColor(CLR_GREEN);
    std::cout << "  " << BOX_V;
    setColor(CLR_YELLOW);
    std::cout << "   [1]  ";
    setColor(CLR_CYAN);
    std::string createText = "+ Create New Playlist";
    std::cout << createText;
    closeRow(8 + static_cast<int>(createText.size()));

    for (size_t i = 0; i < playlists.size(); ++i) {
        setColor(CLR_GREEN);
        std::cout << "  " << BOX_V;
        int chars = 0;

        setColor(CLR_YELLOW);
        std::ostringstream num;
        num << "   [" << i + 2 << "]  ";
        std::string numStr = num.str();
        std::cout << numStr;
        chars += static_cast<int>(numStr.size());

        setColor(CLR_WHITE);
        std::ostringstream entry;
        entry << playlists[i].name << " (" << playlists[i].songIndices.size() << " songs)";
        std::string entryStr = entry.str();
        std::cout << entryStr;
        chars += static_cast<int>(entryStr.size());

        closeRow(chars);
    }

    emptyRow();
    drawFooter();

    std::cout << "\n";
    setColor(CLR_CYAN);
    std::cout << "  " << ICON_ARROW << " ";
    resetColor();
    std::cout << "Select, or 0 to go back: ";
    int choice = readIntChoice();

    if (choice == 1) {
        std::cout << "\n";
        setColor(CLR_CYAN);
        std::cout << "  " << ICON_ARROW << " ";
        resetColor();
        std::cout << "Playlist name: ";
        std::string name;
        std::getline(std::cin, name);
        manager.addPlaylist(name);
        setColor(CLR_GREEN);
        std::cout << "  Created \"" << name << "\"\n";
        resetColor();
        pause();
    } else if (choice > 1 && choice <= static_cast<int>(playlists.size()) + 1) {
        int plIndex = choice - 2;
        const auto& pl = playlists[plIndex];

        std::string plTitle = std::string(ICON_STAR) + "  " + pl.name;
        drawHeader(plTitle);

        if (pl.songIndices.empty()) {
            setColor(CLR_GREEN);
            std::cout << "  " << BOX_V;
            setColor(CLR_GRAY);
            std::string msg = "   (Empty playlist)";
            std::cout << msg;
            closeRow(static_cast<int>(msg.size()));
        } else {
            for (size_t i = 0; i < pl.songIndices.size(); ++i) {
                setColor(CLR_GREEN);
                std::cout << "  " << BOX_V;
                int chars = 0;

                setColor(CLR_YELLOW);
                std::ostringstream num;
                num << "   [" << i + 1 << "]  ";
                std::string numStr = num.str();
                std::cout << numStr;
                chars += static_cast<int>(numStr.size());

                setColor(CLR_WHITE);
                std::string entry = manager.getSongAt(pl.songIndices[i])->getTitle();
                std::cout << entry;
                chars += static_cast<int>(entry.size());

                closeRow(chars);
            }
        }

        emptyRow();
        drawFooter();

        std::cout << "\n";
        setColor(CLR_CYAN);
        std::cout << "  " << ICON_ARROW << " ";
        resetColor();
        std::cout << "Song # to play, or 0 to go back: ";
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
    while (currentScreen != Screen::EXIT) {
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
    clearScreen();
    std::cout << "\n\n";
    setColor(CLR_GREEN);
    std::cout << "  " << ICON_NOTE << "  Thanks for using Music Player. Goodbye!  " << ICON_NOTE << "\n\n";
    resetColor();
}
