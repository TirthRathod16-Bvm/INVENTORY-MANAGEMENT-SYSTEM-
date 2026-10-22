// ============================================================
//  ConsoleUI.h  –  Console colours, formatting helpers
// ============================================================
#pragma once
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <climits>

// ANSI colour codes (work on modern Windows 10+ terminals)
namespace Color {
    const std::string RESET   = "\033[0m";
    const std::string BOLD    = "\033[1m";
    const std::string RED     = "\033[31m";
    const std::string GREEN   = "\033[32m";
    const std::string YELLOW  = "\033[33m";
    const std::string BLUE    = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN    = "\033[36m";
    const std::string WHITE   = "\033[37m";
    const std::string BG_RED  = "\033[41m";
    const std::string BG_GREEN= "\033[42m";
    const std::string BG_BLUE = "\033[44m";
}

namespace UI {

inline void enableAnsi() {
    // Enable ANSI on Windows via VT-processing
#ifdef _WIN32
    system(""); // triggers ANSI mode on modern Windows
#endif
}

inline void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

inline void printLine(char c = '-', int width = 60) {
    std::cout << std::string(width, c) << "\n";
}

inline void printHeader(const std::string& title) {
    printLine('=', 60);
    int pad = (60 - (int)title.size()) / 2;
    std::cout << Color::BOLD << Color::CYAN
              << std::string(std::max(0,pad), ' ') << title << "\n"
              << Color::RESET;
    printLine('=', 60);
}

inline void printSuccess(const std::string& msg) {
    std::cout << Color::GREEN << "[✓] " << msg << Color::RESET << "\n";
}

inline void printError(const std::string& msg) {
    std::cout << Color::RED << "[✗] " << msg << Color::RESET << "\n";
}

inline void printWarning(const std::string& msg) {
    std::cout << Color::YELLOW << "[!] " << msg << Color::RESET << "\n";
}

inline void printInfo(const std::string& msg) {
    std::cout << Color::CYAN << "[i] " << msg << Color::RESET << "\n";
}

inline std::string formatMoney(double amount) {
    std::ostringstream oss;
    oss << "Rs " << std::fixed << std::setprecision(2) << amount;
    return oss.str();
}

// Read a non-empty string from stdin
inline std::string inputString(const std::string& prompt) {
    std::string s;
    do {
        std::cout << Color::YELLOW << prompt << Color::RESET;
        std::getline(std::cin, s);
        if (s.empty()) std::cout << "  Input cannot be empty.\n";
    } while (s.empty());
    return s;
}

// Read an integer in [lo, hi]
inline int inputInt(const std::string& prompt, int lo = INT_MIN, int hi = INT_MAX) {
    int v;
    while (true) {
        std::cout << Color::YELLOW << prompt << Color::RESET;
        std::string line;
        std::getline(std::cin, line);
        try {
            v = std::stoi(line);
            if (v >= lo && v <= hi) return v;
            std::cout << "  Enter a number between " << lo << " and " << hi << ".\n";
        } catch (...) {
            std::cout << "  Invalid number, please try again.\n";
        }
    }
}

// Read a double >= 0
inline double inputDouble(const std::string& prompt, double lo = 0.0) {
    double v;
    while (true) {
        std::cout << Color::YELLOW << prompt << Color::RESET;
        std::string line;
        std::getline(std::cin, line);
        try {
            v = std::stod(line);
            if (v >= lo) return v;
            std::cout << "  Value must be >= " << lo << ".\n";
        } catch (...) {
            std::cout << "  Invalid number, please try again.\n";
        }
    }
}

// Password input (no masking cross-platform; just prompt)
inline std::string inputPassword(const std::string& prompt) {
    std::cout << Color::YELLOW << prompt << Color::RESET;
    std::string s;
    std::getline(std::cin, s);
    return s;
}

// Confirm yes/no
inline bool confirm(const std::string& prompt) {
    std::cout << Color::YELLOW << prompt << " (y/n): " << Color::RESET;
    std::string s;
    std::getline(std::cin, s);
    return !s.empty() && (s[0]=='y' || s[0]=='Y');
}

inline void pause() {
    std::cout << "\nPress ENTER to continue...";
    std::cin.ignore(10000, '\n');
    std::cin.get();
}

}  // namespace UI
