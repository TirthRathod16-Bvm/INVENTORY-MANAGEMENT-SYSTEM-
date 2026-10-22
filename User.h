// ============================================================
//  User.h  –  Role-Based Login System (Encapsulation, Classes)
// ============================================================
#pragma once
#include <string>
#include <vector>
#include <functional>
#include "../utils/Exceptions.h"

enum class UserRole { ADMIN, MANAGER, STAFF };

inline std::string roleToStr(UserRole r) {
    switch(r) {
        case UserRole::ADMIN:   return "Admin";
        case UserRole::MANAGER: return "Manager";
        default:                return "Staff";
    }
}

inline UserRole strToRole(const std::string& s) {
    if (s == "Admin")   return UserRole::ADMIN;
    if (s == "Manager") return UserRole::MANAGER;
    return UserRole::STAFF;
}

// Simple hash (not cryptographic – demo only)
inline std::string hashPassword(const std::string& pw) {
    std::hash<std::string> h;
    return std::to_string(h(pw + "inv_salt_2024"));
}

class User {
private:
    int         id;
    std::string username;
    std::string passwordHash;   // encapsulated – never exposed as plain text
    UserRole    role;
    std::string fullName;
    bool        active;

public:
    User(int id, const std::string& username, const std::string& plainPassword,
         UserRole role, const std::string& fullName = "", bool active = true)
        : id(id), username(username), passwordHash(hashPassword(plainPassword)),
          role(role), fullName(fullName), active(active) {}

    // Constructor for loading from file (password already hashed)
    static User fromHash(int id, const std::string& username,
                         const std::string& hash, UserRole role,
                         const std::string& fullName, bool active) {
        User u(id, username, "", role, fullName, active);
        u.passwordHash = hash;
        return u;
    }

    // Authenticate
    bool checkPassword(const std::string& plain) const {
        return passwordHash == hashPassword(plain);
    }

    void changePassword(const std::string& oldPw, const std::string& newPw) {
        if (!checkPassword(oldPw)) throw AuthenticationException("Old password is incorrect.");
        if (newPw.size() < 4) throw InvalidInputException("Password must be at least 4 characters.");
        passwordHash = hashPassword(newPw);
    }

    // Role-based permission checks
    bool canManageProducts()  const { return role == UserRole::ADMIN || role == UserRole::MANAGER; }
    bool canManageUsers()     const { return role == UserRole::ADMIN; }
    bool canViewReports()     const { return role != UserRole::STAFF; }
    bool canMakeSales()       const { return true; }
    bool canManageSuppliers() const { return role == UserRole::ADMIN || role == UserRole::MANAGER; }
    bool canUndoRedo()        const { return role == UserRole::ADMIN || role == UserRole::MANAGER; }
    bool canBackupRestore()   const { return role == UserRole::ADMIN; }

    // Getters
    int         getId()          const { return id; }
    std::string getUsername()    const { return username; }
    std::string getPasswordHash()const { return passwordHash; }
    UserRole    getRole()        const { return role; }
    std::string getRoleName()    const { return roleToStr(role); }
    std::string getFullName()    const { return fullName; }
    bool        isActive()       const { return active; }

    // Setters
    void setRole(UserRole r)         { role = r; }
    void setFullName(const std::string& n){ fullName = n; }
    void setActive(bool a)           { active = a; }

    // CSV serialisation
    std::vector<std::string> toCSVRow() const {
        return { std::to_string(id), username, passwordHash,
                 roleToStr(role), fullName, active ? "1" : "0" };
    }

    void print() const {
        std::cout << "  ID: " << id
                  << " | Username: " << username
                  << " | Role: " << roleToStr(role)
                  << " | Name: " << fullName
                  << " | Active: " << (active ? "Yes" : "No") << "\n";
    }
};
