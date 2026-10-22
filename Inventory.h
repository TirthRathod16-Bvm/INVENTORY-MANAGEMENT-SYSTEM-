// ============================================================
//  Inventory.h  –  Template-based product repository
//                  STL: vector, map, unordered_map, algorithms
// ============================================================
#pragma once
#include <iomanip>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <numeric>
#include <string>
#include <iostream>
#include "../models/Product.h"
#include "../models/ElectronicProduct.h"
#include "../models/GroceryProduct.h"
#include "../models/GeneralProduct.h"
#include "../utils/FileHandler.h"
#include "../utils/Exceptions.h"
#include "../utils/ConsoleUI.h"

// ============================================================
//  Generic Repository template (demonstrates Templates)
// ============================================================
template<typename T>
class Repository {
protected:
    std::vector<std::shared_ptr<T>> items;

public:
    void add(std::shared_ptr<T> item) { items.push_back(item); }

    std::shared_ptr<T> findById(int id) const {
        for (auto& p : items)
            if (p->getId() == id) return p;
        return nullptr;
    }

    bool remove(int id) {
        auto it = std::find_if(items.begin(), items.end(),
                               [id](const std::shared_ptr<T>& p){ return p->getId()==id; });
        if (it == items.end()) return false;
        items.erase(it);
        return true;
    }

    const std::vector<std::shared_ptr<T>>& getAll() const { return items; }
    size_t count() const { return items.size(); }
    void clear() { items.clear(); }
};

// ============================================================
//  Inventory Manager
// ============================================================
class Inventory : public Repository<Product> {
private:
    std::string dataFile;
    std::string backupDir;
    std::unordered_map<std::string, int> nameIndex;   // lowercase name → id
    int nextId;

    void buildIndex() {
        nameIndex.clear();
        for (auto& p : items) {
            std::string lower = p->getName();
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            nameIndex[lower] = p->getId();
        }
    }

    int generateId() { return nextId++; }

public:
    explicit Inventory(const std::string& file = "data/products.csv",
                       const std::string& backup = "data/backup")
        : dataFile(file), backupDir(backup), nextId(1) {
        FileHandler::ensureDir("data");
        FileHandler::ensureDir(backupDir);
        load();
    }

    // ---- CRUD -----------------------------------------------
    void addProduct(std::shared_ptr<Product> p) {
        if (findById(p->getId()))
            throw DuplicateProductException(p->getId());
        items.push_back(p);
        buildIndex();
        save();
    }

    void updateProduct(int id, const std::string& newName, double newPrice,
                       int newQty, int newMin, int newReorder,
                       const std::string& newSupplier) {
        auto p = findById(id);
        if (!p) throw ProductNotFoundException(std::to_string(id));
        p->setName(newName);
        p->setPrice(newPrice);
        p->setQuantity(newQty);
        p->setMinStockLevel(newMin);
        p->setReorderQty(newReorder);
        p->setSupplierCode(newSupplier);
        buildIndex();
        save();
    }

    void deleteProduct(int id) {
        if (!remove(id)) throw ProductNotFoundException(std::to_string(id));
        buildIndex();
        save();
    }

    // ---- Search (function overloading) ----------------------
    std::shared_ptr<Product> search(int id) const {
        return findById(id);
    }

    std::vector<std::shared_ptr<Product>> search(const std::string& name) const {
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::vector<std::shared_ptr<Product>> results;
        for (auto& p : items) {
            std::string pname = p->getName();
            std::transform(pname.begin(), pname.end(), pname.begin(), ::tolower);
            if (pname.find(lower) != std::string::npos)
                results.push_back(p);
        }
        return results;
    }

    // Binary search by ID (items must be sorted)
    std::shared_ptr<Product> binarySearchById(int id) const {
        auto sorted = items;
        std::sort(sorted.begin(), sorted.end(),
                  [](const std::shared_ptr<Product>& a, const std::shared_ptr<Product>& b){
                      return a->getId() < b->getId(); });
        int lo=0, hi=(int)sorted.size()-1;
        while(lo<=hi) {
            int mid = (lo+hi)/2;
            if (sorted[mid]->getId() == id) return sorted[mid];
            else if (sorted[mid]->getId() < id) lo=mid+1;
            else hi=mid-1;
        }
        return nullptr;
    }

    // ---- Stock operations -----------------------------------
    void addStock(int id, int units) {
        auto p = findById(id);
        if (!p) throw ProductNotFoundException(std::to_string(id));
        p->addStock(units);
        save();
    }

    void removeStock(int id, int units) {
        auto p = findById(id);
        if (!p) throw ProductNotFoundException(std::to_string(id));
        p->removeStock(units);
        save();
    }

    // ---- Queries & Analytics --------------------------------
    std::vector<std::shared_ptr<Product>> getLowStockProducts() const {
        std::vector<std::shared_ptr<Product>> res;
        std::copy_if(items.begin(), items.end(), std::back_inserter(res),
                     [](const std::shared_ptr<Product>& p){ return p->isLowStock(); });
        return res;
    }

    std::vector<std::shared_ptr<Product>> getOutOfStockProducts() const {
        std::vector<std::shared_ptr<Product>> res;
        std::copy_if(items.begin(), items.end(), std::back_inserter(res),
                     [](const std::shared_ptr<Product>& p){ return p->isOutOfStock(); });
        return res;
    }

    double totalInventoryValue() const {
        return std::accumulate(items.begin(), items.end(), 0.0,
                               [](double acc, const std::shared_ptr<Product>& p){
                                   return acc + p->totalValue(); });
    }

    // Sorted copies (STL algorithms)
    std::vector<std::shared_ptr<Product>> sortedByPrice(bool asc=true) const {
        auto v = items;
        std::sort(v.begin(), v.end(), [asc](auto& a, auto& b){
            return asc ? a->getPrice()<b->getPrice() : a->getPrice()>b->getPrice();
        });
        return v;
    }

    std::vector<std::shared_ptr<Product>> sortedByQuantity(bool asc=true) const {
        auto v = items;
        std::sort(v.begin(), v.end(), [asc](auto& a, auto& b){
            return asc ? a->getQuantity()<b->getQuantity() : a->getQuantity()>b->getQuantity();
        });
        return v;
    }

    std::vector<std::shared_ptr<Product>> sortedByName() const {
        auto v = items;
        std::sort(v.begin(), v.end(), [](auto& a, auto& b){
            return a->getName() < b->getName();
        });
        return v;
    }

    // ---- Display --------------------------------------------
    void printAll() const {
        if (items.empty()) { UI::printWarning("No products in inventory."); return; }
        UI::printLine('-',70);
        std::cout << std::left
                  << std::setw(6)  << "ID"
                  << std::setw(22) << "Name"
                  << std::setw(13) << "Category"
                  << std::right
                  << std::setw(10) << "Price(Rs)"
                  << std::setw(8)  << "Stock"
                  << "  Status\n";
        UI::printLine('-',70);
        for (auto& p : items) std::cout << *p << "\n";
        UI::printLine('-',70);
        std::cout << "Total products: " << items.size()
                  << " | Total Value: " << UI::formatMoney(totalInventoryValue()) << "\n\n";
    }

    // ---- Persistence ----------------------------------------
    void save() const {
        std::vector<std::vector<std::string>> rows;
        for (auto& p : items) rows.push_back(p->toCSVRow());
        FileHandler::writeCSV(dataFile, rows);
    }

    void load() {
        items.clear();
        nextId = 1;
        auto rows = FileHandler::readCSV(dataFile);
        for (auto& row : rows) {
            if (row.size() < 5) continue;
            try {
                int    id    = std::stoi(row[0]);
                std::string name  = row[1];
                double price = std::stod(row[2]);
                int    qty   = std::stoi(row[3]);
                std::string tag   = row[4];

                std::shared_ptr<Product> p;
                if (tag == "ELEC") {
                    std::string brand    = row.size()>5 ? row[5] : "";
                    std::string warranty = row.size()>6 ? row[6] : "12";
                    std::string supp     = row.size()>7 ? row[7] : "";
                    int minS = row.size()>8 ? std::stoi(row[8]) : 3;
                    int reO  = row.size()>9 ? std::stoi(row[9]) : 10;
                    p = std::make_shared<ElectronicProduct>(id,name,price,qty,brand,warranty,supp,minS,reO);
                } else if (tag == "GROC") {
                    double tax = row.size()>5 ? std::stod(row[5]) : 0.05;
                    std::string supp = row.size()>6 ? row[6] : "";
                    int minS = row.size()>7 ? std::stoi(row[7]) : 10;
                    int reO  = row.size()>8 ? std::stoi(row[8]) : 50;
                    auto gp = std::make_shared<GroceryProduct>(id,name,price,qty,tax,supp,minS,reO);
                    p = gp;
                } else {
                    double tax = row.size()>5 ? std::stod(row[5]) : 0.12;
                    std::string sub  = row.size()>6 ? row[6] : "General";
                    std::string supp = row.size()>7 ? row[7] : "";
                    int minS = row.size()>8 ? std::stoi(row[8]) : 5;
                    int reO  = row.size()>9 ? std::stoi(row[9]) : 20;
                    p = std::make_shared<GeneralProduct>(id,name,price,qty,tax,sub,supp,minS,reO);
                }
                items.push_back(p);
                if (id >= nextId) nextId = id + 1;
            } catch (...) {}
        }
        buildIndex();
    }

    void backup() const {
        FileHandler::backup(dataFile, backupDir);
        UI::printSuccess("Products backed up.");
    }

    void restore() {
        FileHandler::restore(backupDir, dataFile);
        load();
        UI::printSuccess("Products restored from backup.");
    }

    int getNextId() { return generateId(); }

    // Pre-assign specific ID without side effects
    void forceNextId(int id) { if (id >= nextId) nextId = id + 1; }
};
