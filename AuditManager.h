// ============================================================
//  AuditManager.h  –  Audit trail + Undo/Redo (Command pattern)
// ============================================================
#pragma once
#include <iomanip>
#include <vector>
#include <stack>
#include <memory>
#include <functional>
#include "../models/AuditEntry.h"
#include "../utils/FileHandler.h"
#include "../utils/ConsoleUI.h"
#include "../utils/Exceptions.h"

// ============================================================
//  Command Pattern – base interface for undo/redo
// ============================================================
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void execute() = 0;
    virtual void undo()    = 0;
    virtual std::string description() const = 0;
};

// Stock adjustment command (add/remove)
class StockAdjustCommand : public ICommand {
private:
    std::function<void(int)> doFn;    // executes the stock change
    std::function<void(int)> undoFn;  // reverses it
    int units;
    std::string desc;

public:
    StockAdjustCommand(std::function<void(int)> doFn,
                       std::function<void(int)> undoFn,
                       int units, const std::string& desc)
        : doFn(doFn), undoFn(undoFn), units(units), desc(desc) {}

    void execute() override { doFn(units); }
    void undo()    override { undoFn(units); }
    std::string description() const override { return desc; }
};

// Price change command
class PriceChangeCommand : public ICommand {
private:
    std::function<void(double)> setPrice;
    double oldPrice, newPrice;
    std::string productName;

public:
    PriceChangeCommand(std::function<void(double)> setFn,
                       double oldP, double newP, const std::string& name)
        : setPrice(setFn), oldPrice(oldP), newPrice(newP), productName(name) {}

    void execute() override { setPrice(newPrice); }
    void undo()    override { setPrice(oldPrice); }
    std::string description() const override {
        return "Price change for " + productName +
               ": Rs" + std::to_string(oldPrice) + " → Rs" + std::to_string(newPrice);
    }
};

// ============================================================
//  AuditManager
// ============================================================
class AuditManager {
private:
    std::vector<AuditEntry> entries;
    std::stack<std::unique_ptr<ICommand>> undoStack;
    std::stack<std::unique_ptr<ICommand>> redoStack;
    std::string dataFile;
    int nextId = 1;

public:
    explicit AuditManager(const std::string& file = "data/audit.csv")
        : dataFile(file) {
        FileHandler::ensureDir("data");
        load();
    }

    // Log an event
    void log(AuditAction action, const std::string& user,
             const std::string& desc, int productId=0,
             int prevQty=-1, int newQty=-1,
             const std::string& reason="") {
        entries.emplace_back(nextId++, action, user, desc, productId, prevQty, newQty, reason);
        FileHandler::appendCSV(dataFile, entries.back().toCSVRow());
    }

    // Register a command in the undo stack
    void recordCommand(std::unique_ptr<ICommand> cmd) {
        cmd->execute();
        undoStack.push(std::move(cmd));
        // Clear redo stack after new action
        while (!redoStack.empty()) redoStack.pop();
    }

    // Undo last command
    bool undo(const std::string& username) {
        if (undoStack.empty()) {
            UI::printWarning("Nothing to undo.");
            return false;
        }
        auto& cmd = undoStack.top();
        UI::printInfo("Undoing: " + cmd->description());
        cmd->undo();
        log(AuditAction::OTHER, username, "UNDO: " + cmd->description());
        redoStack.push(std::move(const_cast<std::unique_ptr<ICommand>&>(cmd)));
        undoStack.pop();
        return true;
    }

    // Redo last undone command
    bool redo(const std::string& username) {
        if (redoStack.empty()) {
            UI::printWarning("Nothing to redo.");
            return false;
        }
        auto& cmd = redoStack.top();
        UI::printInfo("Redoing: " + cmd->description());
        cmd->execute();
        log(AuditAction::OTHER, username, "REDO: " + cmd->description());
        undoStack.push(std::move(const_cast<std::unique_ptr<ICommand>&>(cmd)));
        redoStack.pop();
        return true;
    }

    // Display audit trail (latest first, paginated)
    void printAll(int limit = 50) const {
        if (entries.empty()) { UI::printWarning("Audit log is empty."); return; }
        UI::printLine('-', 80);
        std::cout << std::left
                  << std::setw(5)  << "ID"
                  << std::setw(20) << "Timestamp"
                  << std::setw(16) << "Action"
                  << std::setw(12) << "User"
                  << "Description\n";
        UI::printLine('-', 80);
        int shown = 0;
        for (int i = (int)entries.size()-1; i >= 0 && shown < limit; --i, ++shown) {
            auto& e = entries[i];
            std::cout << std::left
                      << std::setw(5)  << e.entryId
                      << std::setw(20) << e.timestamp.substr(0,19)
                      << std::setw(16) << auditActionToStr(e.action).substr(0,15)
                      << std::setw(12) << e.username.substr(0,11)
                      << e.description.substr(0,40) << "\n";
        }
    }

    void filterByUser(const std::string& user) const {
        for (auto& e : entries)
            if (e.username == user) e.print();
    }

    void filterByAction(AuditAction action) const {
        for (auto& e : entries)
            if (e.action == action) e.print();
    }

    void save() const {
        std::vector<std::vector<std::string>> rows;
        for (auto& e : entries) rows.push_back(e.toCSVRow());
        FileHandler::writeCSV(dataFile, rows);
    }

    void load() {
        entries.clear();
        nextId = 1;
        auto rows = FileHandler::readCSV(dataFile);
        for (auto& row : rows) {
            if (row.size() < 9) continue;
            try {
                int id = std::stoi(row[0]);
                AuditEntry e(id, strToAuditAction(row[1]), row[2], row[3],
                             std::stoi(row[5]),
                             std::stoi(row[6]), std::stoi(row[7]), row[8]);
                e.timestamp = row[4];
                entries.push_back(e);
                if (id >= nextId) nextId = id + 1;
            } catch (...) {}
        }
    }
};
