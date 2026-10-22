// ============================================================
//  SupplierManager.h  –  Supplier CRUD + Purchase Orders
// ============================================================
#pragma once
#include <iomanip>
#include <vector>
#include <memory>
#include <algorithm>
#include "../models/Supplier.h"
#include "../managers/Inventory.h"
#include "../utils/FileHandler.h"
#include "../utils/ConsoleUI.h"
#include "../utils/Exceptions.h"

class SupplierManager {
private:
    std::vector<Supplier> suppliers;
    std::vector<PurchaseOrder> orders;
    std::string suppFile;
    std::string poFile;
    int nextPoId = 1;

    Supplier* findByCode(const std::string& code) {
        for (auto& s : suppliers)
            if (s.getCode() == code) return &s;
        return nullptr;
    }

    PurchaseOrder* findPO(int id) {
        for (auto& po : orders)
            if (po.getId() == id) return &po;
        return nullptr;
    }

    static std::vector<POItem> parsePOItems(const std::string& s) {
        std::vector<POItem> items;
        if (s.empty()) return items;
        std::string cur;
        for (char c : s) {
            if (c==';') { items.push_back(POItem::fromCSV(cur)); cur.clear(); }
            else cur += c;
        }
        if (!cur.empty()) items.push_back(POItem::fromCSV(cur));
        return items;
    }

public:
    SupplierManager(const std::string& sf = "data/suppliers.csv",
                    const std::string& pf = "data/purchaseorders.csv")
        : suppFile(sf), poFile(pf) {
        FileHandler::ensureDir("data");
        loadSuppliers();
        loadPOs();
    }

    // ---- Supplier CRUD --------------------------------------
    void addSupplier(const std::string& code, const std::string& name,
                     const std::string& contact, const std::string& email,
                     const std::string& address) {
        if (findByCode(code))
            throw InvalidInputException("Supplier code '" + code + "' already exists.");
        suppliers.emplace_back(code, name, contact, email, address);
        saveSuppliers();
    }

    void updateSupplier(const std::string& code, const std::string& name,
                        const std::string& contact, const std::string& email,
                        const std::string& address) {
        auto* s = findByCode(code);
        if (!s) throw InvalidInputException("Supplier not found: " + code);
        s->setName(name); s->setContact(contact);
        s->setEmail(email); s->setAddress(address);
        saveSuppliers();
    }

    void deleteSupplier(const std::string& code) {
        auto it = std::remove_if(suppliers.begin(), suppliers.end(),
                                 [&](const Supplier& s){ return s.getCode()==code; });
        if (it == suppliers.end())
            throw InvalidInputException("Supplier not found: " + code);
        suppliers.erase(it, suppliers.end());
        saveSuppliers();
    }

    void printAllSuppliers() const {
        if (suppliers.empty()) { UI::printWarning("No suppliers registered."); return; }
        UI::printLine('-', 70);
        std::cout << std::left
                  << std::setw(10) << "Code"
                  << std::setw(22) << "Name"
                  << std::setw(16) << "Contact"
                  << "Email\n";
        UI::printLine('-', 70);
        for (auto& s : suppliers)
            std::cout << std::left
                      << std::setw(10) << s.getCode()
                      << std::setw(22) << s.getName().substr(0,21)
                      << std::setw(16) << s.getContact().substr(0,15)
                      << s.getEmail() << "\n";
    }

    // ---- Purchase Orders ------------------------------------
    int createPurchaseOrder(const std::string& supplierCode,
                            const std::vector<POItem>& items,
                            int leadDays = 7,
                            const std::string& notes = "") {
        if (!findByCode(supplierCode))
            throw InvalidInputException("Unknown supplier: " + supplierCode);
        Date exp{today().day + leadDays, today().month, today().year};
        PurchaseOrder po(nextPoId++, supplierCode, today(), exp, notes);
        for (auto& it : items) po.addItem(it);
        orders.push_back(po);
        savePOs();
        return po.getId();
    }

    void receivePurchaseOrder(int poId, Inventory& inv) {
        auto* po = findPO(poId);
        if (!po) throw InvalidInputException("PO not found: " + std::to_string(poId));
        if (po->getStatus() != POStatus::PENDING)
            throw InvalidInputException("PO #" + std::to_string(poId) + " is not pending.");
        for (auto& it : po->getItems())
            inv.addStock(it.productId, it.quantity);
        po->markReceived();
        savePOs();
        UI::printSuccess("PO #" + std::to_string(poId) + " received. Stock updated.");
    }

    void cancelPurchaseOrder(int poId) {
        auto* po = findPO(poId);
        if (!po) throw InvalidInputException("PO not found: " + std::to_string(poId));
        po->markCancelled();
        savePOs();
    }

    void printAllPOs() const {
        if (orders.empty()) { UI::printWarning("No purchase orders."); return; }
        for (auto& po : orders) po.print();
    }

    void printPendingPOs() const {
        bool any = false;
        for (auto& po : orders)
            if (po.getStatus() == POStatus::PENDING) { po.print(); any = true; }
        if (!any) UI::printInfo("No pending purchase orders.");
    }

    // ---- Persistence ----------------------------------------
    void saveSuppliers() const {
        std::vector<std::vector<std::string>> rows;
        for (auto& s : suppliers) rows.push_back(s.toCSVRow());
        FileHandler::writeCSV(suppFile, rows);
    }

    void loadSuppliers() {
        suppliers.clear();
        auto rows = FileHandler::readCSV(suppFile);
        for (auto& row : rows) {
            if (row.size() < 2) continue;
            std::string code = row[0], name = row[1];
            std::string contact = row.size()>2?row[2]:"";
            std::string email   = row.size()>3?row[3]:"";
            std::string addr    = row.size()>4?row[4]:"";
            suppliers.emplace_back(code, name, contact, email, addr);
        }
    }

    void savePOs() const {
        std::vector<std::vector<std::string>> rows;
        for (auto& po : orders) rows.push_back(po.toCSVRow());
        FileHandler::writeCSV(poFile, rows);
    }

    void loadPOs() {
        orders.clear();
        nextPoId = 1;
        auto rows = FileHandler::readCSV(poFile);
        for (auto& row : rows) {
            if (row.size() < 6) continue;
            try {
                int id = std::stoi(row[0]);
                PurchaseOrder po(id, row[1],
                    Date::fromString(row[2]), Date::fromString(row[3]),
                    row.size()>6 ? row[6] : "");
                auto items = parsePOItems(row[5]);
                for (auto& it : items) po.addItem(it);
                // Set status
                auto st = strToPOStatus(row[4]);
                if (st == POStatus::RECEIVED)  po.markReceived();
                if (st == POStatus::CANCELLED) po.markCancelled();
                orders.push_back(po);
                if (id >= nextPoId) nextPoId = id + 1;
            } catch (...) {}
        }
    }

    const std::vector<Supplier>& getSuppliers() const { return suppliers; }
};
