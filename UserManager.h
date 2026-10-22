// ============================================================
//  UserManager.h  –  Authentication, user CRUD
// ============================================================
#pragma once
#include <iomanip>
#include <vector>
#include <memory>
#include <algorithm>
#include "../models/User.h"
#include "../utils/FileHandler.h"
#include "../utils/Exceptions.h"
#include "../utils/ConsoleUI.h"

class UserManager {
private:
    std::vector<User> users;
    std::string dataFile;
    int nextId = 1;

    User* findByUsername(const std::string& un) {
        for (auto& u : users)
            if (u.getUsername() == un) return &u;
        return nullptr;
    }

public:
    explicit UserManager(const std::string& file = "data/users.csv")
        : dataFile(file) {
        FileHandler::ensureDir("data");
        load();
        // Seed default admin if file was empty
        if (users.empty()) {
            users.emplace_back(nextId++, "admin", "admin123",
                               UserRole::ADMIN, "System Administrator", true);
            users.emplace_back(nextId++, "manager1", "manager123",
                               UserRole::MANAGER, "Store Manager", true);
            users.emplace_back(nextId++, "staff1", "staff123",
                               UserRole::STAFF, "Sales Staff", true);
            save();
            UI::printInfo("Default users created. admin/admin123, manager1/manager123, staff1/staff123");
        }
    }

    // Authenticate and return logged-in user (throws on failure)
    const User& login(const std::string& username, const std::string& password) {
        auto* u = findByUsername(username);
        if (!u || !u->isActive())
            throw AuthenticationException("User not found or inactive.");
        if (!u->checkPassword(password))
            throw AuthenticationException("Incorrect password.");
        return *u;
    }

    void addUser(const std::string& username, const std::string& password,
                 UserRole role, const std::string& fullName,
                 const User& requester) {
        if (!requester.canManageUsers())
            throw PermissionDeniedException("add users");
        if (findByUsername(username))
            throw InvalidInputException("Username '" + username + "' already taken.");
        users.emplace_back(nextId++, username, password, role, fullName, true);
        save();
    }

    void updateUserRole(const std::string& username, UserRole newRole, const User& requester) {
        if (!requester.canManageUsers()) throw PermissionDeniedException("update user role");
        auto* u = findByUsername(username);
        if (!u) throw InvalidInputException("User not found: " + username);
        u->setRole(newRole);
        save();
    }

    void deactivateUser(const std::string& username, const User& requester) {
        if (!requester.canManageUsers()) throw PermissionDeniedException("deactivate users");
        auto* u = findByUsername(username);
        if (!u) throw InvalidInputException("User not found: " + username);
        if (u->getUsername() == requester.getUsername())
            throw InvalidInputException("Cannot deactivate your own account.");
        u->setActive(false);
        save();
    }

    void changePassword(const std::string& username, const std::string& oldPw,
                        const std::string& newPw) {
        auto* u = findByUsername(username);
        if (!u) throw InvalidInputException("User not found.");
        u->changePassword(oldPw, newPw);
        save();
    }

    void printAll() const {
        if (users.empty()) { UI::printWarning("No users."); return; }
        UI::printLine('-', 65);
        std::cout << std::left
                  << std::setw(5)  << "ID"
                  << std::setw(16) << "Username"
                  << std::setw(12) << "Role"
                  << std::setw(24) << "Full Name"
                  << "Active\n";
        UI::printLine('-', 65);
        for (auto& u : users) {
            std::cout << std::left
                      << std::setw(5)  << u.getId()
                      << std::setw(16) << u.getUsername()
                      << std::setw(12) << u.getRoleName()
                      << std::setw(24) << u.getFullName()
                      << (u.isActive() ? "\033[32mYes\033[0m" : "\033[31mNo\033[0m") << "\n";
        }
    }

    void save() const {
        std::vector<std::vector<std::string>> rows;
        for (auto& u : users) rows.push_back(u.toCSVRow());
        FileHandler::writeCSV(dataFile, rows);
    }

    void load() {
        users.clear();
        nextId = 1;
        auto rows = FileHandler::readCSV(dataFile);
        for (auto& row : rows) {
            if (row.size() < 6) continue;
            try {
                int id = std::stoi(row[0]);
                bool active = row[5]=="1";
                users.push_back(
                    User::fromHash(id, row[1], row[2], strToRole(row[3]), row[4], active));
                if (id >= nextId) nextId = id + 1;
            } catch (...) {}
        }
    }
};
