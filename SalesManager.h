// ============================================================
//  SalesManager.h  –  Billing, refunds, analytics, forecasting
// ============================================================
#pragma once
#include <iomanip>
#include <vector>
#include <memory>
#include <map>
#include <algorithm>
#include <numeric>
#include <fstream>
#include "../models/Sale.h"
#include "../managers/Inventory.h"
#include "../utils/FileHandler.h"
#include "../utils/DateUtils.h"
#include "../utils/ConsoleUI.h"
#include "../utils/Exceptions.h"

struct SalesStats {
    double revenue    = 0;
    double cost       = 0;
    double grossProfit= 0;
    int    totalUnits = 0;
    int    totalSales = 0;
    std::map<int, int> unitsSoldPerProduct;  // productId → units
    std::map<int, double> revenuePerProduct;
};

class SalesManager {
private:
    std::vector<Sale> sales;
    std::string dataFile;
    int nextSaleId = 1;

    // Parse items from semicolon-delimited CSV field
    static std::vector<SaleItem> parseItems(const std::string& s) {
        std::vector<SaleItem> items;
        if (s.empty()) return items;
        std::string cur;
        for (char c : s) {
            if (c==';') { items.push_back(SaleItem::fromCSV(cur)); cur.clear(); }
            else cur += c;
        }
        if (!cur.empty()) items.push_back(SaleItem::fromCSV(cur));
        return items;
    }

public:
    explicit SalesManager(const std::string& file = "data/sales.csv")
        : dataFile(file) {
        FileHandler::ensureDir("data");
        load();
    }

    // ---- Create a sale (polymorphic tax from Product) -------
    Sale createSale(Inventory& inv,
                    const std::string& customerName,
                    const std::vector<std::pair<int,int>>& productQtyList, // {id, qty}
                    const std::string& staffUser,
                    DiscountType discType = DiscountType::NONE,
                    double discValue = 0.0) {

        Sale sale(nextSaleId++, customerName, today(), staffUser);

        for (auto& pair : productQtyList) {
            int pid = pair.first;
            int qty = pair.second;
            auto p = inv.search(pid);
            if (!p) throw ProductNotFoundException(std::to_string(pid));
            if (p->getQuantity() < qty)
                throw InsufficientStockException(p->getName(), p->getQuantity(), qty);

            double unitPrice = p->getPrice();
            double taxPerUnit = p->calculateTax();    // polymorphic call
            double discAmt = 0.0;
            if (discType == DiscountType::PERCENTAGE)
                discAmt = unitPrice * (discValue / 100.0);
            else if (discType == DiscountType::FIXED)
                discAmt = discValue / productQtyList.size();

            SaleItem item;
            item.productId     = pid;
            item.productName   = p->getName();
            item.quantity      = qty;
            item.unitPrice     = unitPrice;
            item.taxAmount     = taxPerUnit;
            item.discountAmount= discAmt;
            item.discType      = discType;
            item.discValue     = discValue;
            sale.addItem(item);

            // Update stock
            inv.removeStock(pid, qty);
        }

        sales.push_back(sale);
        save();
        return sale;
    }

    // ---- Refund a sale ---------------------------------------
    void refundSale(int saleId, Inventory& inv, const std::string& user) {
        for (auto& s : sales) {
            if (s.getId() == saleId) {
                if (s.getRefunded()) throw DuplicateRefundException(saleId);
                // Restore stock
                for (auto& it : s.getItems())
                    inv.addStock(it.productId, it.quantity);
                s.markRefunded();
                save();
                UI::printSuccess("Sale #" + std::to_string(saleId) + " refunded successfully.");
                return;
            }
        }
        throw SaleNotFoundException(saleId);
    }

    // ---- Retrieve sale ---------------------------------------
    const Sale* findSale(int id) const {
        for (auto& s : sales)
            if (s.getId() == id) return &s;
        return nullptr;
    }

    // ---- Analytics ------------------------------------------
    SalesStats computeStats(const Date& from, const Date& to) const {
        SalesStats stats;
        for (auto& s : sales) {
            if (s.getRefunded()) continue;
            if (s.getDate() < from || to < s.getDate()) continue;
            stats.totalSales++;
            stats.revenue += s.grandTotal();
            for (auto& it : s.getItems()) {
                stats.totalUnits += it.quantity;
                stats.unitsSoldPerProduct[it.productId]  += it.quantity;
                stats.revenuePerProduct[it.productId]    += it.lineTotal();
            }
        }
        return stats;
    }

    SalesStats dailyStats(const Date& d) const { return computeStats(d, d); }

    SalesStats weeklyStats(const Date& refDate) const {
        // Find Monday of the week containing refDate
        int dow = dayOfWeek(refDate); // 0=Sun
        int daysToMon = (dow == 0) ? 6 : dow - 1;
        Date mon{refDate.day - daysToMon, refDate.month, refDate.year};
        // Simple: just do 7 days from Monday
        Date sun{mon.day + 6, mon.month, mon.year};
        return computeStats(mon, sun);
    }

    SalesStats monthlyStats(int month, int year) const {
        Date from{1, month, year};
        // Days in month (approx)
        int days = 31;
        if (month==4||month==6||month==9||month==11) days=30;
        else if (month==2) days = (year%4==0)?29:28;
        Date to{days, month, year};
        return computeStats(from, to);
    }

    // Top N products by units sold in a period
    std::vector<std::pair<int,int>> topProductsByUnits(
            const Date& from, const Date& to, int n=5) const {
        auto stats = computeStats(from, to);
        std::vector<std::pair<int,int>> v(stats.unitsSoldPerProduct.begin(),
                                          stats.unitsSoldPerProduct.end());
        std::sort(v.begin(), v.end(), [](auto& a, auto& b){ return a.second>b.second; });
        if ((int)v.size() > n) v.resize(n);
        return v;
    }

    // ---- 7-day moving average demand forecast ---------------
    std::map<int, double> forecastDemand(const Inventory& inv) const {
        Date t = today();
        Date weekAgo{t.day-7, t.month, t.year};
        auto stats = computeStats(weekAgo, t);
        std::map<int, double> forecast;
        for (auto& entry : stats.unitsSoldPerProduct) {
            int pid = entry.first;
            int units = entry.second;
            forecast[pid] = units / 7.0;  // avg daily units
        }
        return forecast;
    }

    // Recommend reorder: returns {productId → reorder_qty}
    std::map<int, int> reorderRecommendations(const Inventory& inv,
                                               int leadTimeDays = 7) const {
        auto forecast = forecastDemand(inv);
        std::map<int, int> recs;
        for (auto& p : inv.getAll()) {
            auto it = forecast.find(p->getId());
            double avgDaily = (it != forecast.end()) ? it->second : 0;
            int safetyStock = (int)(avgDaily * leadTimeDays);
            if (p->getQuantity() <= safetyStock || p->isLowStock()) {
                int needed = (int)(avgDaily * leadTimeDays * 2) + p->getReorderQty();
                if (needed > 0) recs[p->getId()] = needed;
            }
        }
        return recs;
    }

    // ---- Display / Reports ----------------------------------
    void printAllSales() const {
        if (sales.empty()) { UI::printWarning("No sales records."); return; }
        UI::printLine('-', 65);
        std::cout << std::left
                  << std::setw(6)  << "ID"
                  << std::setw(18) << "Customer"
                  << std::setw(12) << "Date"
                  << std::setw(10) << "Staff"
                  << std::right
                  << std::setw(12) << "Total(Rs)"
                  << "  Status\n";
        UI::printLine('-', 65);
        for (auto& s : sales) {
            std::cout << std::left
                      << std::setw(6)  << s.getId()
                      << std::setw(18) << s.getCustomer().substr(0,17)
                      << std::setw(12) << s.getDate().toString()
                      << std::setw(10) << s.getStaff().substr(0,9)
                      << std::right
                      << std::setw(12) << std::fixed << std::setprecision(2) << s.grandTotal()
                      << (s.getRefunded() ? "  \033[31m[REFUNDED]\033[0m" : "  \033[32m[VALID]\033[0m")
                      << "\n";
        }
    }

    void printSalesReport(const Date& from, const Date& to,
                          const Inventory& inv) const {
        auto stats = computeStats(from, to);
        UI::printHeader("SALES REPORT: " + from.toString() + " to " + to.toString());
        std::cout << "  Total Sales  : " << stats.totalSales << "\n"
                  << "  Total Units  : " << stats.totalUnits << "\n"
                  << "  Revenue      : " << UI::formatMoney(stats.revenue) << "\n";

        // Top 5 by units
        std::vector<std::pair<int,int>> top(stats.unitsSoldPerProduct.begin(),
                                             stats.unitsSoldPerProduct.end());
        std::sort(top.begin(), top.end(), [](auto& a, auto& b){ return a.second>b.second; });
        std::cout << "\n  Top Products by Units Sold:\n";
        int rank = 1;
        for (auto& item : top) {
            int pid = item.first;
            int units = item.second;
            if (rank > 5) break;
            auto p = inv.search(pid);
            std::string nm = p ? p->getName() : "Unknown";
            std::cout << "    " << rank++ << ". " << nm << " – " << units << " units"
                      << " (Rs " << std::fixed << std::setprecision(2)
                      << stats.revenuePerProduct[pid] << ")\n";
        }
    }

    // Export to CSV
    void exportSalesCSV(const std::string& path, const Date& from, const Date& to) const {
        std::ofstream f(path);
        f << "SaleID,Customer,Date,Staff,Refunded,GrandTotal\n";
        for (auto& s : sales) {
            if (s.getDate() < from || to < s.getDate()) continue;
            f << s.getId() << "," << s.getCustomer() << "," << s.getDate().toString()
              << "," << s.getStaff() << "," << (s.getRefunded()?"Yes":"No")
              << "," << std::fixed << std::setprecision(2) << s.grandTotal() << "\n";
        }
        UI::printSuccess("Sales exported to " + path);
    }

    void save() const {
        std::vector<std::vector<std::string>> rows;
        for (auto& s : sales) rows.push_back(s.toCSVRow());
        FileHandler::writeCSV(dataFile, rows);
    }

    void load() {
        sales.clear();
        nextSaleId = 1;
        auto rows = FileHandler::readCSV(dataFile);
        for (auto& row : rows) {
            if (row.size() < 6) continue;
            try {
                int id = std::stoi(row[0]);
                Sale s(id, row[1], Date::fromString(row[2]), row[3]);
                if (row[4]=="1") s.markRefunded();
                auto items = parseItems(row[5]);
                for (auto& it : items) s.addItem(it);
                sales.push_back(s);
                if (id >= nextSaleId) nextSaleId = id + 1;
            } catch (...) {}
        }
    }

    const std::vector<Sale>& getSales() const { return sales; }
};
