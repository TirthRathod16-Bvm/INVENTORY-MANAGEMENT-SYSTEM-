// ============================================================
//  FileHandler.h  –  CSV read / write helpers + backup
// ============================================================
#pragma once
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include "Exceptions.h"

class FileHandler {
public:
    // ---- CSV helpers ----------------------------------------
    // Split a CSV line respecting quoted fields
    static std::vector<std::string> splitCSV(const std::string& line) {
        std::vector<std::string> fields;
        std::string cur;
        bool inQ = false;
        for (char c : line) {
            if (c == '"') { inQ = !inQ; }
            else if (c == ',' && !inQ) { fields.push_back(cur); cur.clear(); }
            else { cur += c; }
        }
        fields.push_back(cur);
        return fields;
    }

    // Escape a field for CSV
    static std::string escapeCSV(const std::string& s) {
        if (s.find(',') != std::string::npos ||
            s.find('"') != std::string::npos ||
            s.find('\n') != std::string::npos) {
            std::string r = "\"";
            for (char c : s) { if (c=='"') r += '"'; r += c; }
            r += "\"";
            return r;
        }
        return s;
    }

    // ---- File I/O -------------------------------------------
    static std::vector<std::vector<std::string>> readCSV(const std::string& path) {
        std::vector<std::vector<std::string>> rows;
        std::ifstream f(path);
        if (!f.is_open()) return rows;   // new file – empty
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty() || line[0]=='#') continue;
            rows.push_back(splitCSV(line));
        }
        return rows;
    }

    // Overwrite entire file with rows
    static void writeCSV(const std::string& path,
                         const std::vector<std::vector<std::string>>& rows) {
        std::ofstream f(path);
        if (!f.is_open()) throw FileIOException(path);
        for (auto& row : rows) {
            for (size_t i = 0; i < row.size(); ++i) {
                if (i) f << ',';
                f << escapeCSV(row[i]);
            }
            f << '\n';
        }
    }

    // Append a single row
    static void appendCSV(const std::string& path,
                          const std::vector<std::string>& row) {
        std::ofstream f(path, std::ios::app);
        if (!f.is_open()) throw FileIOException(path);
        for (size_t i = 0; i < row.size(); ++i) {
            if (i) f << ',';
            f << escapeCSV(row[i]);
        }
        f << '\n';
    }

    // ---- Backup / Restore -----------------------------------
    static bool fileExists(const std::string& path) {
        std::ifstream f(path);
        return f.good();
    }

    static void ensureDir(const std::string& dir) {
        if (dir.empty()) return;
        std::string current;
        for (char c : dir) {
            if (c == '/' || c == '\\') {
                if (!current.empty()) {
#ifdef _WIN32
                    _mkdir(current.c_str());
#else
                    mkdir(current.c_str(), 0755);
#endif
                }
            }
            current += c;
        }
        if (!current.empty()) {
#ifdef _WIN32
            _mkdir(current.c_str());
#else
            mkdir(current.c_str(), 0755);
#endif
        }
    }

    static void copyFile(const std::string& src, const std::string& dest) {
        std::ifstream in(src, std::ios::binary);
        if (!in.is_open()) return;
        std::ofstream out(dest, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) throw FileIOException(dest);
        out << in.rdbuf();
    }

    static std::string getFilename(const std::string& path) {
        size_t idx = path.find_last_of("/\\");
        if (idx == std::string::npos) return path;
        return path.substr(idx + 1);
    }

    static void backup(const std::string& srcPath, const std::string& backupDir) {
        ensureDir(backupDir);
        if (!fileExists(srcPath)) return;
        std::string dest = backupDir + "/" + getFilename(srcPath) + ".bak";
        copyFile(srcPath, dest);
    }

    static void restore(const std::string& backupDir, const std::string& destPath) {
        std::string src = backupDir + "/" + getFilename(destPath) + ".bak";
        if (!fileExists(src)) throw FileIOException(src);
        copyFile(src, destPath);
    }
};
