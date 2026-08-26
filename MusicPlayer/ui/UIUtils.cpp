#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <limits>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <conio.h>
#else
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include "../song_management/MusicManager.cpp"

namespace UIUtils {
    // Helper to get visual width of UTF-8 strings
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

    void clearScreen() {
        // Move cursor to home (top-left) instead of system("cls") to prevent flickering
        std::cout << "\x1B[H";
    }

    void pause() {
        std::cout << "\n  Press ENTER to continue...\x1B[J" << std::flush;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
    }

    int readIntChoice() {
        std::cout << "\x1B[J" << std::flush; // Clear from cursor to end of screen before prompt
        int choice;
        std::cin >> choice;
        if (std::cin.fail()) {
            std::cin.clear();
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return choice;
    }

    int readSingleKeyWithTimeout(int timeoutMs) {
        std::cout << "\x1B[J" << std::flush; // Clear from cursor to end of screen
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

    void printBoxTop(const std::string& title) {
        clearScreen();
        std::cout << "\n  ╔════════════════════════════════════════════════════════════════╗\n";
        
        int totalWidth = 64;
        int titleLen = visualLength(title);
        int padding = (totalWidth - titleLen) / 2;
        int rightPad = totalWidth - titleLen - padding;
        
        std::cout << "  ║" << std::string(padding, ' ') << title << std::string(rightPad, ' ') << "║\n";
        std::cout << "  ╚════════════════════════════════════════════════════════════════╝\n";
        std::cout << "  ║                                                                ║\n";
    }

    void printBoxLine(const std::string& text) {
        int totalWidth = 64;
        int len = visualLength(text);
        int padding = totalWidth - len;
        if (padding < 0) padding = 0;
        
        std::cout << "  ║ " << text << std::string(padding - 1, ' ') << "║\n";
    }

    void printBoxBottom() {
        std::cout << "  ║                                                                ║\n";
        std::cout << "  ╚════════════════════════════════════════════════════════════════╝\n";
    }

    void printHeader(const std::string& title) {
        printBoxTop(title);
    }
    
    // Playback integration
    void playSystemAudio(const Song* current) {
        if (!current) return;
#ifdef _WIN32
        mciSendStringA("close all", NULL, 0, NULL);
        std::string command = "open \"" + current->getFilePath() + "\" type mpegvideo alias mymusic";
        mciSendStringA(command.c_str(), NULL, 0, NULL);
        mciSendStringA("play mymusic", NULL, 0, NULL);
#else
        system("killall -9 ffplay > /dev/null 2>&1 || pkill -9 -f ffplay > /dev/null 2>&1");
        std::string command = "ffplay -nodisp -autoexit \"" + current->getFilePath() + "\" > /dev/null 2>&1 &";
        system(command.c_str());
#endif
    }
    
    void updateLinuxPlaybackState(MusicManager& manager) {
#ifndef _WIN32
        // Linux playback tracking logic (simplified)
#endif
    }
}
