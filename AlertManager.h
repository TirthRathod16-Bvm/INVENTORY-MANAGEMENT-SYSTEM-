// ============================================================
//  AlertManager.h  –  Observer pattern for low-stock alerts
//                      + expiry alerts for grocery products
// ============================================================
#pragma once
#include <vector>
#include <functional>
#include <memory>
#include "../managers/Inventory.h"
#include "../models/GroceryProduct.h"
#include "../utils/ConsoleUI.h"

// ============================================================
//  Observer Pattern
// ============================================================
class IAlertObserver {
public:
    virtual ~IAlertObserver() = default;
    virtual void onLowStock(const Product& p) = 0;
    virtual void onOutOfStock(const Product& p) = 0;
    virtual void onExpiringProduct(const Product& p, int daysLeft) = 0;
};

// Console observer – prints coloured alerts
class ConsoleAlertObserver : public IAlertObserver {
public:
    void onLowStock(const Product& p) override {
        UI::printWarning("LOW STOCK: " + p.getName() +
                         " (ID:" + std::to_string(p.getId()) +
                         ") – Only " + std::to_string(p.getQuantity()) +
                         " units left (min: " + std::to_string(p.getMinStockLevel()) + ")");
    }
    void onOutOfStock(const Product& p) override {
        std::cout << "\033[31m[!!!] OUT OF STOCK: " << p.getName()
                  << " (ID:" << p.getId() << ")\033[0m\n";
    }
    void onExpiringProduct(const Product& p, int daysLeft) override {
        UI::printWarning("EXPIRING: " + p.getName() +
                         " – " + std::to_string(daysLeft) + " days until expiry!");
    }
};

// ============================================================
//  AlertManager
// ============================================================
class AlertManager {
private:
    std::vector<std::shared_ptr<IAlertObserver>> observers;

public:
    void addObserver(std::shared_ptr<IAlertObserver> obs) {
        observers.push_back(obs);
    }

    void notifyLowStock(const Product& p) {
        for (auto& obs : observers) obs->onLowStock(p);
    }
    void notifyOutOfStock(const Product& p) {
        for (auto& obs : observers) obs->onOutOfStock(p);
    }
    void notifyExpiring(const Product& p, int daysLeft) {
        for (auto& obs : observers) obs->onExpiringProduct(p, daysLeft);
    }

    // Scan whole inventory and fire all relevant alerts
    void runAlertScan(const Inventory& inv, int expiryWarnDays = 30) {
        bool anyAlert = false;
        for (auto& p : inv.getAll()) {
            if (p->isOutOfStock()) {
                notifyOutOfStock(*p);
                anyAlert = true;
            } else if (p->isLowStock()) {
                notifyLowStock(*p);
                anyAlert = true;
            }
            // Expiry check for grocery products
            auto* gp = dynamic_cast<GroceryProduct*>(p.get());
            if (gp) {
                for (auto& b : gp->getBatches()) {
                    int d = b.daysUntilExpiry();
                    if (d >= 0 && d <= expiryWarnDays) {
                        notifyExpiring(*p, d);
                        anyAlert = true;
                    }
                }
            }
        }
        if (!anyAlert) UI::printSuccess("No alerts. All stock levels are healthy.");
    }

    // Print a full alert report
    void printAlertReport(const Inventory& inv, int expiryWarnDays = 30) const {
        UI::printHeader("ALERT REPORT");

        // Low stock
        auto low = inv.getLowStockProducts();
        std::cout << "\033[33m--- LOW STOCK PRODUCTS (" << low.size() << ") ---\033[0m\n";
        if (low.empty()) UI::printSuccess("None.");
        for (auto& p : low) {
            std::cout << "  * " << p->getName()
                      << " | Qty: " << p->getQuantity()
                      << " | Min: " << p->getMinStockLevel()
                      << " | Need to reorder: " << p->unitsToReorder() << " units\n";
        }

        // Out of stock
        auto oos = inv.getOutOfStockProducts();
        std::cout << "\n\033[31m--- OUT OF STOCK (" << oos.size() << ") ---\033[0m\n";
        if (oos.empty()) UI::printSuccess("None.");
        for (auto& p : oos) {
            std::cout << "  * " << p->getName() << " (ID: " << p->getId() << ")\n";
        }

        // Expiry warnings
        std::cout << "\n\033[35m--- EXPIRING WITHIN " << expiryWarnDays << " DAYS ---\033[0m\n";
        bool anyExpiry = false;
        for (auto& p : inv.getAll()) {
            auto* gp = dynamic_cast<GroceryProduct*>(p.get());
            if (!gp) continue;
            auto exp = gp->getExpiringBatches(expiryWarnDays);
            for (auto& b : exp) {
                std::cout << "  * " << p->getName()
                          << " Batch #" << b.batchId
                          << " | Qty: " << b.quantity
                          << " | Expires: " << b.expiryDate.toString()
                          << " | Days left: " << b.daysUntilExpiry() << "\n";
                anyExpiry = true;
            }
        }
        if (!anyExpiry) UI::printSuccess("No products nearing expiry.");
    }
};
