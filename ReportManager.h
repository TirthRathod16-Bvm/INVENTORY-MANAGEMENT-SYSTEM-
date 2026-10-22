// ============================================================
//  ReportManager.h  –  Automated report generation (inheritance,
//                       polymorphism, file handling)
// ============================================================
#pragma once
#include <string>
#include <fstream>
#include <iomanip>
#include "../managers/Inventory.h"
#include "../managers/SalesManager.h"
#include "../managers/SupplierManager.h"
#include "../utils/DateUtils.h"
#include "../utils/ConsoleUI.h"

// ============================================================
//  Abstract Report base class (demonstrating Abstract Classes)
// ============================================================
class IReport {
public:
    virtual ~IReport() = default;
    virtual void generate()      = 0;   // print to console
    virtual void exportToFile()  = 0;   // write to CSV/TXT
    virtual std::string getTitle() const = 0;
};

// ---- Inventory Valuation Report ----------------------------
class InventoryValuationReport : public IReport {
    const Inventory& inv;
    std::string exportPath;
public:
    InventoryValuationReport(const Inventory& i, const std::string& p="data/report_inventory.csv")
        : inv(i), exportPath(p) {}

    std::string getTitle() const override { return "Inventory Valuation Report"; }

    void generate() override {
        UI::printHeader(getTitle());
        double total = 0;
        std::cout << std::left
                  << std::setw(6) << "ID" << std::setw(24) << "Product"
                  << std::setw(12) << "Category"
                  << std::right
                  << std::setw(10) << "Price"
                  << std::setw(8)  << "Qty"
                  << std::setw(14) << "Value(Rs)" << "\n";
        UI::printLine('-', 74);
        for (auto& p : inv.getAll()) {
            double val = p->totalValue();
            total += val;
            std::cout << std::left
                      << std::setw(6)  << p->getId()
                      << std::setw(24) << p->getName().substr(0,23)
                      << std::setw(12) << p->getCategory().substr(0,11)
                      << std::right
                      << std::setw(10) << std::fixed << std::setprecision(2) << p->getPrice()
                      << std::setw(8)  << p->getQuantity()
                      << std::setw(14) << val << "\n";
        }
        UI::printLine('-', 74);
        std::cout << std::right << std::setw(60) << "TOTAL INVENTORY VALUE: Rs "
                  << std::setw(14) << total << "\n\n";
    }

    void exportToFile() override {
        std::ofstream f(exportPath);
        f << "ID,Product,Category,Price,Quantity,TotalValue\n";
        for (auto& p : inv.getAll())
            f << p->getId() << "," << p->getName() << "," << p->getCategory()
              << "," << std::fixed << std::setprecision(2) << p->getPrice()
              << "," << p->getQuantity() << "," << p->totalValue() << "\n";
        UI::printSuccess("Report exported: " + exportPath);
    }
};

// ---- Low Stock Report --------------------------------------
class LowStockReport : public IReport {
    const Inventory& inv;
    std::string exportPath;
public:
    LowStockReport(const Inventory& i, const std::string& p="data/report_lowstock.csv")
        : inv(i), exportPath(p) {}

    std::string getTitle() const override { return "Low Stock / Reorder Report"; }

    void generate() override {
        UI::printHeader(getTitle());
        auto low = inv.getLowStockProducts();
        if (low.empty()) { UI::printSuccess("All products have adequate stock."); return; }
        std::cout << std::left
                  << std::setw(6)  << "ID"
                  << std::setw(24) << "Product"
                  << std::right
                  << std::setw(8)  << "Stock"
                  << std::setw(8)  << "Min"
                  << std::setw(10) << "Reorder"
                  << "  Supplier\n";
        UI::printLine('-', 65);
        for (auto& p : low)
            std::cout << std::left
                      << std::setw(6)  << p->getId()
                      << std::setw(24) << p->getName().substr(0,23)
                      << std::right
                      << std::setw(8)  << p->getQuantity()
                      << std::setw(8)  << p->getMinStockLevel()
                      << std::setw(10) << p->unitsToReorder()
                      << "  " << p->getSupplierCode() << "\n";
    }

    void exportToFile() override {
        std::ofstream f(exportPath);
        f << "ID,Product,Stock,MinLevel,ReorderQty,Supplier\n";
        for (auto& p : inv.getLowStockProducts())
            f << p->getId() << "," << p->getName() << "," << p->getQuantity()
              << "," << p->getMinStockLevel() << "," << p->unitsToReorder()
              << "," << p->getSupplierCode() << "\n";
        UI::printSuccess("Report exported: " + exportPath);
    }
};

// ---- Daily Sales Report ------------------------------------
class DailySalesReport : public IReport {
    const SalesManager& sm;
    const Inventory& inv;
    Date date;
    std::string exportPath;
public:
    DailySalesReport(const SalesManager& s, const Inventory& i,
                     const Date& d, const std::string& p="data/report_daily_sales.csv")
        : sm(s), inv(i), date(d), exportPath(p) {}

    std::string getTitle() const override {
        return "Daily Sales Report: " + date.toString();
    }

    void generate() override {
        sm.printSalesReport(date, date, inv);
    }

    void exportToFile() override {
        sm.exportSalesCSV(exportPath, date, date);
    }
};

// ---- Supplier Report ----------------------------------------
class SupplierReport : public IReport {
    const SupplierManager& sm;
    std::string exportPath;
public:
    SupplierReport(const SupplierManager& s, const std::string& p="data/report_suppliers.csv")
        : sm(s), exportPath(p) {}

    std::string getTitle() const override { return "Supplier Report"; }

    void generate() override {
        UI::printHeader(getTitle());
        sm.printAllSuppliers();
        std::cout << "\n";
        UI::printInfo("Purchase Orders:");
        sm.printAllPOs();
    }

    void exportToFile() override {
        std::ofstream f(exportPath);
        f << "Code,Name,Contact,Email,Address\n";
        for (auto& s : sm.getSuppliers())
            f << s.getCode() << "," << s.getName() << "," << s.getContact()
              << "," << s.getEmail() << "," << s.getAddress() << "\n";
        UI::printSuccess("Supplier report exported: " + exportPath);
    }
};

// ============================================================
//  Report Manager – Factory for reports (Factory Pattern)
// ============================================================
class ReportManager {
private:
    const Inventory& inv;
    const SalesManager& sm;
    const SupplierManager& supm;

public:
    ReportManager(const Inventory& i, const SalesManager& s, const SupplierManager& su)
        : inv(i), sm(s), supm(su) {}

    void runInventoryReport()  { InventoryValuationReport r(inv); r.generate(); }
    void runLowStockReport()   { LowStockReport r(inv);           r.generate(); }
    void runDailySalesReport() { DailySalesReport r(sm, inv, today()); r.generate(); }
    void runSupplierReport()   { SupplierReport r(supm);          r.generate(); }

    void exportAll() {
        InventoryValuationReport(inv).exportToFile();
        LowStockReport(inv).exportToFile();
        DailySalesReport(sm, inv, today()).exportToFile();
        SupplierReport(supm).exportToFile();
        UI::printSuccess("All reports exported to data/ folder.");
    }

    void runMonthlySalesReport(int month, int year) {
        auto stats = sm.monthlyStats(month, year);
        UI::printHeader("Monthly Sales Report: " + std::to_string(month) + "/" + std::to_string(year));
        std::cout << "  Total Sales  : " << stats.totalSales << "\n"
                  << "  Total Units  : " << stats.totalUnits << "\n"
                  << "  Revenue      : " << UI::formatMoney(stats.revenue) << "\n";
    }
};
