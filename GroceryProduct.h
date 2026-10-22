// ============================================================
//  GroceryProduct.h  –  Derived class (Inheritance, Polymorphism)
//                        Supports expiry date and batch tracking
// ============================================================
#pragma once
#include "Product.h"
#include "../utils/DateUtils.h"
#include <vector>
#include <string>
#include <algorithm>

// A single batch of grocery stock (for FEFO)
struct Batch {
    int    batchId;
    int    quantity;
    Date   mfgDate;
    Date   expiryDate;

    Batch(int bid, int qty, const Date& mfg, const Date& exp)
        : batchId(bid), quantity(qty), mfgDate(mfg), expiryDate(exp) {}

    bool isExpired() const { return expiryDate < today(); }

    int daysUntilExpiry() const { return today().daysUntil(expiryDate); }

    std::string toCSV() const {
        return std::to_string(batchId) + "|" + std::to_string(quantity) +
               "|" + mfgDate.toString() + "|" + expiryDate.toString();
    }

    static Batch fromCSV(const std::string& s) {
        // format: batchId|qty|mfgDate|expiryDate
        std::vector<std::string> parts;
        std::string cur;
        for (char c : s) {
            if (c=='|') { parts.push_back(cur); cur.clear(); }
            else cur += c;
        }
        parts.push_back(cur);
        if (parts.size() < 4) return Batch{0,0,Date{},Date{}};
        return Batch{std::stoi(parts[0]), std::stoi(parts[1]),
                     Date::fromString(parts[2]), Date::fromString(parts[3])};
    }
};

class GroceryProduct : public Product {
private:
    std::vector<Batch> batches;
    double taxRate; // e.g. 0.05 for 5%

public:
    GroceryProduct(int id, const std::string& name, double price, int qty,
                   double taxRate = 0.05,
                   const std::string& supplier = "",
                   int minStock = 10, int reorder = 50)
        : Product(id, name, price, qty, supplier, minStock, reorder),
          taxRate(taxRate)
    {
        // Create a default batch expiring 1 year from now
        Date exp{today().day, today().month, today().year + 1};
        batches.emplace_back(1, qty, today(), exp);
    }

    GroceryProduct(const GroceryProduct&) = default;

    // ------- Polymorphism -----------------------------------
    double calculateTax() const override {
        return getPrice() * taxRate;
    }

    std::string getCategory() const override { return "Grocery"; }
    std::string getTypeTag()  const override { return "GROC"; }

    // Batch management
    void addBatch(const Batch& b) {
        batches.push_back(b);
        // Add to total quantity
        addStock(b.quantity);
    }

    // FEFO: remove from earliest-expiring batch first
    void removeBatchStock(int units) {
        if (units > getQuantity())
            throw InsufficientStockException(name, getQuantity(), units);

        // Sort by expiry ascending
        std::sort(batches.begin(), batches.end(),
                  [](const Batch& a, const Batch& b){ return a.expiryDate < b.expiryDate; });

        int remaining = units;
        for (auto& b : batches) {
            if (b.isExpired()) continue;
            if (remaining <= 0) break;
            int take = std::min(remaining, b.quantity);
            b.quantity -= take;
            remaining  -= take;
        }
        // Remove empty batches
        batches.erase(std::remove_if(batches.begin(), batches.end(),
                       [](const Batch& b){ return b.quantity == 0; }), batches.end());
        removeStock(units);
    }

    // Products expiring within `days` days
    std::vector<Batch> getExpiringBatches(int days = 30) const {
        std::vector<Batch> res;
        for (auto& b : batches) {
            int d = b.daysUntilExpiry();
            if (d <= days && d >= 0) res.push_back(b);
        }
        return res;
    }

    bool hasExpiringStock(int days = 30) const {
        return !getExpiringBatches(days).empty();
    }

    const std::vector<Batch>& getBatches() const { return batches; }

    void setTaxRate(double r) { taxRate = r; }
    double getTaxRate() const { return taxRate; }

    std::vector<std::string> toCSVRow() const override {
        // Serialise batches as semicolon-separated
        std::string batchStr;
        for (size_t i = 0; i < batches.size(); ++i) {
            if (i) batchStr += ";";
            batchStr += batches[i].toCSV();
        }
        return {
            std::to_string(id), name, std::to_string(getPrice()),
            std::to_string(getQuantity()), getTypeTag(),
            std::to_string(taxRate), supplierCode,
            std::to_string(minStockLevel), std::to_string(reorderQty),
            batchStr
        };
    }

    void printDetails() const override {
        Product::printDetails();
        std::cout << "\033[1m  Tax Rate    :\033[0m " << (taxRate*100) << "%\n";
        std::cout << "\033[1m  Batches     :\033[0m " << batches.size() << "\n";
        for (auto& b : batches) {
            int d = b.daysUntilExpiry();
            std::cout << "    Batch #" << b.batchId
                      << " | Qty: " << b.quantity
                      << " | Exp: " << b.expiryDate.toString()
                      << " | Days left: ";
            if (d < 0) std::cout << "\033[31mEXPIRED\033[0m";
            else if (d <= 30) std::cout << "\033[33m" << d << "\033[0m";
            else std::cout << "\033[32m" << d << "\033[0m";
            std::cout << "\n";
        }
    }
};
