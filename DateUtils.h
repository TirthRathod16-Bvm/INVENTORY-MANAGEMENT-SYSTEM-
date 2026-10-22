// ============================================================
//  DateUtils.h  –  Simple date helpers (no external libs)
// ============================================================
#pragma once
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <stdexcept>

struct Date {
    int day, month, year;

    Date() : day(1), month(1), year(2000) {}
    Date(int d, int m, int y) : day(d), month(m), year(y) {}

    // Serialise to "YYYY-MM-DD"
    std::string toString() const {
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(4) << year << "-"
            << std::setw(2) << month << "-"
            << std::setw(2) << day;
        return oss.str();
    }

    // Parse from "YYYY-MM-DD"
    static Date fromString(const std::string& s) {
        if (s.size() < 10) return Date{};
        try {
            int y = std::stoi(s.substr(0, 4));
            int mo = std::stoi(s.substr(5, 2));
            int d  = std::stoi(s.substr(8, 2));
            return Date{d, mo, y};
        } catch (...) { return Date{}; }
    }

    bool operator<(const Date& o) const {
        if (year  != o.year)  return year  < o.year;
        if (month != o.month) return month < o.month;
        return day < o.day;
    }
    bool operator==(const Date& o) const {
        return day==o.day && month==o.month && year==o.year;
    }
    bool operator<=(const Date& o) const { return !(o < *this); }
    bool operator>(const Date& o)  const { return o < *this; }

    // Days between two dates (approximation)
    int daysUntil(const Date& other) const {
        auto toJulian = [](const Date& d) -> long {
            int a = (14 - d.month) / 12;
            int y = d.year + 4800 - a;
            int m = d.month + 12 * a - 3;
            return d.day + (153*m+2)/5 + 365L*y + y/4 - y/100 + y/400 - 32045;
        };
        return static_cast<int>(toJulian(other) - toJulian(*this));
    }
};

// Return today's date
inline Date today() {
    std::time_t t = std::time(nullptr);
    std::tm* tm_p = std::localtime(&t);
    return Date{tm_p->tm_mday, tm_p->tm_mon + 1, tm_p->tm_year + 1900};
}

// Return current timestamp string "YYYY-MM-DD HH:MM:SS"
inline std::string nowTimestamp() {
    std::time_t t = std::time(nullptr);
    std::tm* tm_p = std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(tm_p, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// Day-of-week index (0=Sun)
inline int dayOfWeek(const Date& d) {
    // Tomohiko Sakamoto's algorithm
    static const int t[] = {0,3,2,5,0,3,5,1,4,6,2,4};
    int y = d.year;
    if (d.month < 3) y--;
    return (y + y/4 - y/100 + y/400 + t[d.month-1] + d.day) % 7;
}

// Week number (ISO-ish)
inline int weekOfYear(const Date& d) {
    Date jan1{1,1,d.year};
    return (jan1.daysUntil(d) + dayOfWeek(jan1)) / 7 + 1;
}
