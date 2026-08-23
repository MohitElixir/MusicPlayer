#include "UIManager.h"
#include <iostream>
#include <limits>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <conio.h>
#include <io.h>
#include <windows.h>
#undef max
#undef min

// ════════════════════════════════════════════════════════════
// Helper utilities (anonymous namespace)
// ════════════════════════════════════════════════════════════
namespace {
    const int UI_WIDTH = 56;

    // ── Windows Console Handle ──
    HANDLE getConsoleHandle() {
        return GetStdHandle(STD_OUTPUT_HANDLE);
    }

    // ── Color helpers ──
    void setColor(int fg) { SetConsoleTextAttribute(getConsoleHandle(), fg); }
    void resetColor()     { SetConsoleTextAttribute(getConsoleHandle(), 7);  }

    const int CLR_GREEN   = 10;
    const int CLR_CYAN    = 11;
    const int CLR_RED     = 12;
    const int CLR_YELLOW  = 14;
    const int CLR_WHITE   = 15;
    const int CLR_GRAY    = 8;
    const int CLR_DGREEN  = 2;

    // ── Screen control (no system("cls") to avoid Application Control blocks) ──
    void clearScreen() {
        // Output ANSI escape code to clear console screen and move cursor to (0,0)
        std::cout << "\x1B[2J\x1B[H" << std::flush;

        COORD origin = { 0, 0 };
        DWORD written;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        HANDLE hConsole = getConsoleHandle();
        if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
            DWORD size = csbi.dwSize.X * csbi.dwSize.Y;
            FillConsoleOutputCharacterA(hConsole, ' ', size, origin, &written);
            FillConsoleOutputAttribute(hConsole, csbi.wAttributes, size, origin, &written);
            SetConsoleCursorPosition(hConsole, origin);
        }
    }

    // Move cursor to top-left without clearing (for flicker-free redraws)
    void cursorToTop() {
        std::cout << "\x1B[H" << std::flush;
        SetConsoleCursorPosition(getConsoleHandle(), { 0, 0 });
    }

    // Hide/show the blinking cursor
    void hideCursor() {
        CONSOLE_CURSOR_INFO ci;
        HANDLE hConsole = getConsoleHandle();
        GetConsoleCursorInfo(hConsole, &ci);
        ci.bVisible = FALSE;
        SetConsoleCursorInfo(hConsole, &ci);
    }
    void showCursor() {
        CONSOLE_CURSOR_INFO ci;
        HANDLE hConsole = getConsoleHandle();
        GetConsoleCursorInfo(hConsole, &ci);
        ci.bVisible = TRUE;
        SetConsoleCursorInfo(hConsole, &ci);
    }

    // ── UTF-8 icon strings ──
    // These are 3-byte UTF-8 sequences that render as 1 display character
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

    // Print N copies of a box-drawing horizontal bar
    void hBar(int n, const char* ch = nullptr) {
        if (!ch) ch = BOX_H;
        for (int i = 0; i < n; ++i) std::cout << ch;
    }

    // Print a bordered row: ║ <content padded to UI_WIDTH> ║
    // 'used' = how many DISPLAY characters the caller already printed
    // after printing the opening ║. This prints the remaining spaces + closing ║.
    void closeRow(int used) {
        int remaining = UI_WIDTH - used;
        if (remaining > 0) std::cout << std::string(remaining, ' ');
        setColor(CLR_GREEN);
        std::cout << BOX_V << "\n";
    }

    // Print an empty bordered row:  ║                          ║
    void emptyRow() {
        setColor(CLR_GREEN);
        std::cout << "  " << BOX_V << std::string(UI_WIDTH, ' ') << BOX_V << "\n";
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
}

// ════════════════════════════════════════════════════════════
// Constructor
// ════════════════════════════════════════════════════════════
UIManager::UIManager(MusicManager& manager)
    : manager(manager), currentScreen(Screen::MENU), audioPlayingIndex(-1) {
    SetConsoleOutputCP(65001); // Enable UTF-8 output

    // Enable Virtual Terminal Processing for ANSI escape code support
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        SetConsoleMode(hOut, dwMode | 0x0004); // 0x0004 = ENABLE_VIRTUAL_TERMINAL_PROCESSING
    }
}

// ════════════════════════════════════════════════════════════
// Drawing helpers
// ════════════════════════════════════════════════════════════
void UIManager::drawDivider() const {
    setColor(CLR_GRAY);
    std::cout << "  ";
    hBar(UI_WIDTH + 2, BOX_LH);
    std::cout << "\n";
    resetColor();
}

void UIManager::drawHeader(const std::string& title) const {
    clearScreen();
    std::cout << "\n";

    // ╔════════════╗
    setColor(CLR_GREEN);
    std::cout << "  " << BOX_TL;
    hBar(UI_WIDTH);
    std::cout << BOX_TR << "\n";

    // ║   TITLE    ║
    // Count display width of title (each 3-byte UTF-8 char = 1 display char)
    int dispLen = 0;
    for (size_t i = 0; i < title.size(); ) {
        unsigned char c = title[i];
        dispLen++;
        if (c < 0x80) i++; else if (c < 0xE0) i += 2; else if (c < 0xF0) i += 3; else i += 4;
    }
    int pad = (UI_WIDTH - dispLen) / 2;
    if (pad < 0) pad = 0;

    std::cout << "  " << BOX_V;
    setColor(CLR_WHITE);
    std::cout << std::string(pad, ' ') << title << std::string(UI_WIDTH - pad - dispLen, ' ');
    setColor(CLR_GREEN);
    std::cout << BOX_V << "\n";

    // ╚════════════╝
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

// ════════════════════════════════════════════════════════════
// Input helpers
// ════════════════════════════════════════════════════════════
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

// ════════════════════════════════════════════════════════════
// Screen: Main Menu (LIVE with persistent real-time player)
// ════════════════════════════════════════════════════════════
void UIManager::showMenu() {
    manager.refreshFromFolder("E:\\MusicPlayer\\MusicPlayer\\Music");

    const int BAR_WIDTH = 30;

    // Enable live updates only if running inside a real interactive terminal window (not piped stdout) AND a song is playing
    bool liveMode = (_isatty(_fileno(stdout)) != 0) && manager.isPlaying();

    if (!liveMode) {
        // --- Static Menu Mode (Runs once and blocks on input) ---
        showCursor();
        clearScreen();

        std::string title = std::string(ICON_NOTE) + "  MUSIC PLAYER  " + ICON_NOTE;
        drawHeader(title);
        emptyRow();

        auto menuItem = [&](const char* num, const char* icon, int iconColor, const std::string& text, int textColor) {
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

        // Static Player info (if playing but piped console)
        const Song* np = manager.getNowPlaying();
        if (np != nullptr) {
            char lenBuf[128] = {0}, posBuf[128] = {0};
            mciSendStringA("status mymusic length", lenBuf, sizeof(lenBuf), NULL);
            mciSendStringA("status mymusic position", posBuf, sizeof(posBuf), NULL);
            int lengthMs  = atoi(lenBuf);
            int positionMs = atoi(posBuf);

            int filled = (lengthMs > 0) ? (positionMs * BAR_WIDTH / lengthMs) : 0;
            if (filled > BAR_WIDTH) filled = BAR_WIDTH;
            int empty = BAR_WIDTH - filled;

            std::string timeL = formatTime(positionMs);
            std::string timeR = formatTime(lengthMs);

            std::cout << "\n";
            setColor(CLR_GRAY);
            std::cout << "  " << ICON_PLAY << "  ";
            setColor(CLR_GREEN);
            std::cout << np->getTitle();
            setColor(CLR_GRAY);
            std::cout << " - " << np->getArtist() << "\n";

            std::cout << "     " << timeL << " [";
            setColor(CLR_GREEN);
            for (int i = 0; i < filled; ++i) std::cout << "=";
            if (empty > 0) {
                setColor(CLR_WHITE);
                std::cout << ">";
                empty--;
            }
            setColor(CLR_GRAY);
            for (int i = 0; i < empty; ++i) std::cout << "-";
            std::cout << "] " << timeR << "\n";
        } else {
            std::cout << "\n\n\n";
        }

        std::cout << "\n";
        setColor(CLR_CYAN);
        std::cout << "  " << ICON_ARROW << " ";
        resetColor();
        std::cout << "Select: ";

        int choice = readIntChoice();
        if (choice == 1)      currentScreen = Screen::LIBRARY;
        else if (choice == 2) currentScreen = Screen::SEARCH;
        else if (choice == 3) currentScreen = Screen::NOW_PLAYING;
        else if (choice == 4) currentScreen = Screen::FAVORITES;
        else if (choice == 5) currentScreen = Screen::PLAYLISTS;
        else if (choice == 6) currentScreen = Screen::EXIT;
        return;
    }

    // --- Live Menu Mode (Ticking progress bar in real console) ---
    hideCursor();
    bool firstDraw = true;

    while (true) {
        manager.refreshFromFolder("E:\\MusicPlayer\\MusicPlayer\\Music");

        // If music stopped, fallback to static menu
        if (!manager.isPlaying()) {
            showCursor();
            currentScreen = Screen::MENU;
            return;
        }

        if (firstDraw) {
            clearScreen();
            firstDraw = false;
        } else {
            cursorToTop();
        }

        // Draw Menu
        std::string title = std::string(ICON_NOTE) + "  MUSIC PLAYER  " + ICON_NOTE;
        drawHeader(title);
        emptyRow();

        auto menuItem = [&](const char* num, const char* icon, int iconColor, const std::string& text, int textColor) {
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

        // Live player bar
        const Song* np = manager.getNowPlaying();
        char lenBuf[128] = {0}, posBuf[128] = {0};
        mciSendStringA("status mymusic length", lenBuf, sizeof(lenBuf), NULL);
        mciSendStringA("status mymusic position", posBuf, sizeof(posBuf), NULL);
        int lengthMs  = atoi(lenBuf);
        int positionMs = atoi(posBuf);

        int filled = (lengthMs > 0) ? (positionMs * BAR_WIDTH / lengthMs) : 0;
        if (filled > BAR_WIDTH) filled = BAR_WIDTH;
        int empty = BAR_WIDTH - filled;

        std::string timeL = formatTime(positionMs);
        std::string timeR = formatTime(lengthMs);

        std::cout << "\n";
        setColor(CLR_GRAY);
        std::cout << "  " << ICON_PLAY << "  ";
        setColor(CLR_GREEN);
        std::cout << np->getTitle();
        setColor(CLR_GRAY);
        std::cout << " - " << np->getArtist() << "\n";

        std::cout << "     " << timeL << " [";
        setColor(CLR_GREEN);
        for (int i = 0; i < filled; ++i) std::cout << "=";
        if (empty > 0) {
            setColor(CLR_WHITE);
            std::cout << ">";
            empty--;
        }
        setColor(CLR_GRAY);
        for (int i = 0; i < empty; ++i) std::cout << "-";
        std::cout << "] " << timeR << "\n";

        // Auto-advance if song finishes
        if (lengthMs > 0 && positionMs >= lengthMs - 200) {
            manager.playNext();
            audioPlayingIndex = manager.getNowPlayingIndex();
        }

        std::cout << "\n";
        setColor(CLR_CYAN);
        std::cout << "  " << ICON_ARROW << " ";
        resetColor();
        std::cout << "Select option: ";

        if (_kbhit()) {
            char key = _getch();
            showCursor();
            if (key == '1')      { currentScreen = Screen::LIBRARY; return; }
            else if (key == '2') { currentScreen = Screen::SEARCH; return; }
            else if (key == '3') { currentScreen = Screen::NOW_PLAYING; return; }
            else if (key == '4') { currentScreen = Screen::FAVORITES; return; }
            else if (key == '5') { currentScreen = Screen::PLAYLISTS; return; }
            else if (key == '6') { currentScreen = Screen::EXIT; return; }
            hideCursor();
        }

        Sleep(400);
    }
}

// ════════════════════════════════════════════════════════════
// Screen: Library
// ════════════════════════════════════════════════════════════
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

            // Currently playing indicator
            if (manager.getNowPlayingIndex() == static_cast<int>(i)) {
                setColor(CLR_GREEN);
                std::cout << " " << ICON_PLAY << " ";
                chars += 3; // space + icon(1) + space
            } else {
                std::cout << "   ";
                chars += 3;
            }

            // Song number
            setColor(CLR_YELLOW);
            std::ostringstream num;
            num << "[" << i + 1 << "]";
            std::string numStr = num.str();
            std::cout << std::left << std::setw(5) << numStr;
            chars += 5;

            // Favorite heart
            if (songs[i].isFavorite()) {
                setColor(CLR_RED);
                std::cout << ICON_HEART << " ";
                chars += 2;
            } else {
                std::cout << "  ";
                chars += 2;
            }

            // Song title - artist
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
        manager.play(choice - 1);
        currentScreen = Screen::NOW_PLAYING;
    } else {
        setColor(CLR_RED);
        std::cout << "  Invalid selection.\n";
        resetColor();
        pause();
    }
}

// ════════════════════════════════════════════════════════════
// Screen: Search
// ════════════════════════════════════════════════════════════
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

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
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

    if (choice >= 1 && choice <= static_cast<int>(results.size())) {
        manager.play(results[choice - 1]);
        currentScreen = Screen::NOW_PLAYING;
    } else {
        currentScreen = Screen::MENU;
    }
}

// ════════════════════════════════════════════════════════════
// Screen: Now Playing (LIVE with real-time progress bar)
//
// This screen uses _kbhit()/_getch() for non-blocking input
// so the progress bar can update in real time. Just press a
// key — no Enter needed.
// ════════════════════════════════════════════════════════════
void UIManager::showNowPlaying() {
    const Song* current = manager.getNowPlaying();

    // If nothing is selected, show a message and go back
    if (!current) {
        std::string title = std::string(ICON_PLAY) + "  NOW PLAYING";
        drawHeader(title);
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

    hideCursor();
    bool firstDraw = true;
    const int BAR_WIDTH = 36;

    // ── Live playback loop ──
    while (true) {
        current = manager.getNowPlaying();
        if (!current) break;
        int currentIndex = manager.getNowPlayingIndex();

        // Start or restart playback only if the song changed
        if (currentIndex != audioPlayingIndex) {
            mciSendStringA("close all", NULL, 0, NULL);
            std::string cmd = "open \"" + current->getFilePath() + "\" type mpegvideo alias mymusic";
            mciSendStringA(cmd.c_str(), NULL, 0, NULL);
            mciSendStringA("play mymusic", NULL, 0, NULL);
            audioPlayingIndex = currentIndex;
            firstDraw = true; // force full redraw for new song
        }

        // Query MCI for current position and total length (in milliseconds)
        char lenBuf[128] = {0}, posBuf[128] = {0};
        mciSendStringA("status mymusic length", lenBuf, sizeof(lenBuf), NULL);
        mciSendStringA("status mymusic position", posBuf, sizeof(posBuf), NULL);
        int lengthMs  = atoi(lenBuf);
        int positionMs = atoi(posBuf);

        // ── Draw / Redraw screen ──
        if (firstDraw) {
            clearScreen();
            firstDraw = false;
        } else {
            cursorToTop();
        }

        // Header
        std::cout << "\n";
        setColor(CLR_GREEN);
        std::cout << "  " << BOX_TL; hBar(UI_WIDTH); std::cout << BOX_TR << "\n";

        std::string title = std::string(ICON_PLAY) + "  NOW PLAYING";
        int dispLen = 14; // "▶  NOW PLAYING" = 14 display chars
        int headerPad = (UI_WIDTH - dispLen) / 2;
        std::cout << "  " << BOX_V;
        setColor(CLR_WHITE);
        std::cout << std::string(headerPad, ' ') << title << std::string(UI_WIDTH - headerPad - dispLen, ' ');
        setColor(CLR_GREEN);
        std::cout << BOX_V << "\n";

        std::cout << "  " << BOX_BL; hBar(UI_WIDTH); std::cout << BOX_BR << "\n";

        // Empty row
        emptyRow();

        // ♫ Song Title
        std::cout << "  " << BOX_V;
        setColor(CLR_CYAN);
        std::cout << "   " << ICON_NOTE << " ";
        setColor(CLR_WHITE);
        std::string titleText = current->getTitle();
        std::cout << titleText;
        closeRow(3 + 2 + static_cast<int>(titleText.size()));

        // Artist
        std::cout << "  " << BOX_V;
        setColor(CLR_GRAY);
        std::string artistText = "     " + current->getArtist();
        std::cout << artistText;
        closeRow(static_cast<int>(artistText.size()));

        emptyRow();

        // ── Progress Bar ──
        int filled = (lengthMs > 0) ? (positionMs * BAR_WIDTH / lengthMs) : 0;
        if (filled > BAR_WIDTH) filled = BAR_WIDTH;
        int empty = BAR_WIDTH - filled;

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

        // Filled portion
        for (int i = 0; i < filled; ++i) std::cout << "=";
        chars += filled;

        // Playhead
        if (empty > 0) {
            setColor(CLR_WHITE);
            std::cout << ">";
            chars += 1;
            empty--;
        }

        // Empty portion
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

        // ♥ Like status
        std::cout << "  " << BOX_V;
        if (current->isFavorite()) {
            setColor(CLR_RED);
            std::string likeText = "   " + std::string(ICON_HEART) + " Liked";
            std::cout << likeText;
            closeRow(3 + 1 + 6); // "   " + heart(1) + " Liked"
        } else {
            setColor(CLR_GRAY);
            std::string likeText = "   Not liked (press 4 to like)";
            std::cout << likeText;
            closeRow(static_cast<int>(likeText.size()));
        }

        emptyRow();

        // ── Controls bar ──
        drawDivider();
        setColor(CLR_GREEN);
        std::cout << "  " << BOX_V;
        int cc = 0;

        setColor(CLR_WHITE);
        std::cout << " ";
        cc += 1;

        setColor(CLR_YELLOW); std::cout << "[1]"; cc += 3;
        setColor(CLR_WHITE);  std::cout << ICON_STOP << " "; cc += 2;

        setColor(CLR_YELLOW); std::cout << "[2]"; cc += 3;
        setColor(CLR_WHITE);  std::cout << ">> "; cc += 3;

        setColor(CLR_YELLOW); std::cout << "[3]"; cc += 3;
        setColor(CLR_WHITE);  std::cout << "<< "; cc += 3;

        setColor(CLR_YELLOW); std::cout << "[4]"; cc += 3;
        setColor(CLR_RED);    std::cout << ICON_HEART << " "; cc += 2;

        setColor(CLR_YELLOW); std::cout << "[5]"; cc += 3;
        setColor(CLR_WHITE);  std::cout << "+List "; cc += 6;

        setColor(CLR_YELLOW); std::cout << "[0]"; cc += 3;
        setColor(CLR_GRAY);   std::cout << "Back"; cc += 4;

        closeRow(cc);
        drawFooter();

        // Prompt line
        std::cout << "\n";
        setColor(CLR_GRAY);
        std::cout << "  Press a key (no Enter needed)";
        std::cout << std::string(30, ' '); // clear any leftover chars
        resetColor();

        // ── Handle non-blocking input ──
        if (_kbhit()) {
            char key = _getch();

            if (key == '0') {
                // Back to menu (music keeps playing)
                showCursor();
                currentScreen = Screen::MENU;
                return;
            }
            else if (key == '1') {
                // Stop playback
                manager.stop();
                mciSendStringA("close all", NULL, 0, NULL);
                audioPlayingIndex = -1;
                showCursor();
                currentScreen = Screen::MENU;
                return;
            }
            else if (key == '2') {
                // Next song
                manager.playNext();
                // audioPlayingIndex mismatch will trigger playback restart at top of loop
            }
            else if (key == '3') {
                // Previous song
                manager.playPrevious();
            }
            else if (key == '4') {
                // Toggle favorite
                manager.toggleFavorite(currentIndex);
            }
            else if (key == '5') {
                // Add to playlist (briefly switch to blocking interaction)
                showCursor();
                clearScreen();
                auto playlists = manager.getPlaylists();
                if (playlists.empty()) {
                    setColor(CLR_GRAY);
                    std::cout << "\n  No playlists yet. Create one from the Playlists menu.\n";
                    std::cout << "  Press any key...";
                    resetColor();
                    _getch();
                } else {
                    setColor(CLR_WHITE);
                    std::cout << "\n  Add to playlist:\n\n";
                    for (size_t i = 0; i < playlists.size(); ++i) {
                        setColor(CLR_YELLOW);
                        std::cout << "  [" << i + 1 << "] ";
                        setColor(CLR_WHITE);
                        std::cout << playlists[i].name << "\n";
                    }
                    setColor(CLR_GRAY);
                    std::cout << "\n  Press playlist # (or 0 to cancel): ";
                    resetColor();
                    char plKey = _getch();
                    int plNum = plKey - '0';
                    if (plNum > 0 && plNum <= static_cast<int>(playlists.size())) {
                        manager.addToPlaylist(plNum - 1, currentIndex);
                        setColor(CLR_GREEN);
                        std::cout << "\n  Added!\n";
                        resetColor();
                        Sleep(800);
                    }
                }
                hideCursor();
                firstDraw = true; // force full redraw
            }
        }

        // ── Auto-advance when song finishes ──
        if (lengthMs > 0 && positionMs >= lengthMs - 200) {
            manager.playNext();
            // The index mismatch will restart playback at the top of the loop
        }

        Sleep(400); // Refresh ~2.5 times per second
    }

    showCursor();
    currentScreen = Screen::MENU;
}

// ════════════════════════════════════════════════════════════
// Screen: Favorites
// ════════════════════════════════════════════════════════════
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
        manager.play(favs[choice - 1]);
        currentScreen = Screen::NOW_PLAYING;
    } else {
        currentScreen = Screen::MENU;
    }
}

// ════════════════════════════════════════════════════════════
// Screen: Playlists
// ════════════════════════════════════════════════════════════
void UIManager::showPlaylists() {
    std::string title = std::string(ICON_STAR) + "  PLAYLISTS";
    drawHeader(title);

    auto playlists = manager.getPlaylists();

    // "Create new" option
    setColor(CLR_GREEN);
    std::cout << "  " << BOX_V;
    setColor(CLR_YELLOW);
    std::cout << "   [1]  ";
    setColor(CLR_CYAN);
    std::string createText = "+ Create New Playlist";
    std::cout << createText;
    closeRow(8 + static_cast<int>(createText.size()));

    // Existing playlists
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
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
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
            manager.play(pl.songIndices[sChoice - 1]);
            currentScreen = Screen::NOW_PLAYING;
            return;
        }
    } else if (choice == 0) {
        currentScreen = Screen::MENU;
    }
}

// ════════════════════════════════════════════════════════════
// Main loop
// ════════════════════════════════════════════════════════════
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
