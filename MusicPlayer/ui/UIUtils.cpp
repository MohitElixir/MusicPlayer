/*
 * UIUtils.cpp
 * ===========
 * Utility functions for the terminal-based user interface.
 *
 * Provides:
 *   - Console setup (UTF-8 + ANSI escape code support)
 *   - Screen clearing using ANSI escape codes (no system("cls"))
 *   - Box-drawing for the TUI layout
 *   - User input helpers (readIntChoice, pause, non-blocking keypress)
 *   - UTF-8 visual width calculation
 *
 * All functions are grouped inside the UIUtils namespace, which
 * works similarly to a static utility class.
 */

#pragma once

#include <string>
#include <iostream>
#include <cstdlib>
#include <limits>

// Platform-specific includes for keyboard input
#ifdef _WIN32
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #include <windows.h>
    #include <conio.h>
#else
    #include <termios.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif

using namespace std;

namespace UIUtils {

    // Sets up the console for UTF-8 and ANSI escape code support.
    // Called once at program startup.
    void setupConsole() {
#ifdef _WIN32
        SetConsoleOutputCP(65001);  // enable UTF-8 output

        // Enable ANSI escape sequences for colors, cursor movement, etc.
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
#endif
        // On Linux/Mac, ANSI codes work by default
    }

    // Calculate the visual width of a UTF-8 string.
    // Multi-byte characters (♫, ♥, ★) count as 1 character each.
    int visualLength(const string& s) {
        int len = 0;
        for (size_t i = 0; i < s.length(); ) {
            unsigned char c = s[i];
            if      ((c & 0x80) == 0)    i += 1;  // ASCII
            else if ((c & 0xE0) == 0xC0) i += 2;  // 2-byte UTF-8
            else if ((c & 0xF0) == 0xE0) i += 3;  // 3-byte UTF-8
            else if ((c & 0xF8) == 0xF0) i += 4;  // 4-byte UTF-8
            else i++;
            len++;
        }
        return len;
    }

    // Move cursor to top-left (prevents flicker compared to system("cls"))
    void clearScreen() {
        cout << "\x1B[H";
    }

    // Full screen clear (used when switching between views)
    void fullClear() {
        cout << "\x1B[2J\x1B[H" << flush;
    }

    // Wait for user to press ENTER
    void pause() {
        cout << "\n  Press ENTER to continue...\x1B[J" << flush;
        cin.clear();
        cin.ignore((numeric_limits<streamsize>::max)(), '\n');
        cin.get();
    }

    // Read an integer from user input. Returns -1 if invalid.
    int readIntChoice() {
        cout << "\x1B[J" << flush;
        int choice;
        cin >> choice;
        if (cin.fail()) {
            cin.clear();
            choice = -1;
        }
        cin.ignore((numeric_limits<streamsize>::max)(), '\n');
        return choice;
    }

    // Non-blocking keypress reader with timeout (used for Now Playing screen).
    // Returns digit 0-9 if pressed, or -1 on timeout.
    int readSingleKeyWithTimeout(int timeoutMs) {
        cout << "\x1B[J" << flush;
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

            if (ch != EOF && ch >= '0' && ch <= '9')
                return ch - '0';
            usleep(50000);
#endif
            elapsed += 50;
        }
        return -1;  // timeout
    }

    // --- Box-drawing functions for the TUI layout ---

    void printBoxTop(const string& title) {
        clearScreen();
        cout << "\n  ╔════════════════════════════════════════════════════════════════╗\n";

        int totalWidth = 64;
        int titleLen = visualLength(title);
        int padding = (totalWidth - titleLen) / 2;
        int rightPad = totalWidth - titleLen - padding;

        cout << "  ║" << string(padding, ' ') << title
             << string(rightPad, ' ') << "║\n";
        cout << "  ╚════════════════════════════════════════════════════════════════╝\n";
        cout << "  ║                                                                ║\n";
    }

    void printBoxLine(const string& text) {
        int totalWidth = 64;
        int len = visualLength(text);
        int padding = totalWidth - len;
        if (padding < 0) padding = 0;
        cout << "  ║ " << text << string(padding - 1, ' ') << "║\n";
    }

    void printBoxBottom() {
        cout << "  ║                                                                ║\n";
        cout << "  ╚════════════════════════════════════════════════════════════════╝\n";
    }

    void printHeader(const string& title) {
        printBoxTop(title);
    }

}  // namespace UIUtils
