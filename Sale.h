// ============================================================
//  Sale.h  –  Sale record, SaleItem, SaleReturn
// ============================================================
#pragma once
#include <string>
#include <vector>
#include <iomanip>
#include <iostream>
#include "../utils/DateUtils.h"

// ---- Discount types -----------------------------------------
enum class DiscountType { NONE, PERCENTAGE, FIXED };

// ---- Individual sale line item ------------------------------
struct SaleItem {
    int    productId;
    std::string productName;
    int    quantity;
    double unitPrice;       // before tax/discount
    double taxAmount;       // per unit
    double discountAmount;  // total discount for this line
    DiscountType discType;
    double discValue;       // percent or fixed amount

    double lineTotal() const {
        return (unitPrice + taxAmount - discountAmount) * quantity;
    }

    std::string toCSV() const {
        return std::to_string(productId) + "~" + productName + "~" +
               std::to_string(quantity) + "~" + std::to_string(unitPrice) + "~" +
               std::to_string(taxAmount) + "~" + std::to_string(discountAmount) + "~" +
               std::to_string((int)discType) + "~" + std::to_string(discValue);
    }

    static SaleItem fromCSV(const std::string& s) {
        SaleItem it{};
        std::vector<std::string> p;
        std::string cur;
        for (char c : s) { if (c=='~') { p.push_back(cur); cur.clear(); } else cur+=c; }
        p.push_back(cur);
        if (p.size() >= 8) {
            it.productId     = std::stoi(p[0]);
            it.productName   = p[1];
            it.quantity      = std::stoi(p[2]);
            it.unitPrice     = std::stod(p[3]);
            it.taxAmount     = std::stod(p[4]);
            it.discountAmount= std::stod(p[5]);
            it.discType      = (DiscountType)std::stoi(p[6]);
            it.discValue     = std::stod(p[7]);
        }
        return it;
    }
};

// ---- Full Sale Record ---------------------------------------
class Sale {
private:
    int         saleId;
    std::string customerName;
    Date        saleDate;
    std::vector<SaleItem> items;
    bool        isRefunded;
    std::string staffUsername;

public:
    Sale(int id, const std::string& customer,
         const Date& date, const std::string& staff)
        : saleId(id), customerName(customer), saleDate(date),
          isRefunded(false), staffUsername(staff) {}

    void addItem(const SaleItem& item) { items.push_back(item); }

    double subtotal() const {
        double t = 0;
        for (auto& it : items) t += it.unitPrice * it.quantity;
        return t;
    }
    double totalTax() const {
        double t = 0;
        for (auto& it : items) t += it.taxAmount * it.quantity;
        return t;
    }
    double totalDiscount() const {
        double t = 0;
        for (auto& it : items) t += it.discountAmount * it.quantity;
        return t;
    }
    double grandTotal() const {
        double t = 0;
        for (auto& it : items) t += it.lineTotal();
        return t;
    }

    int         getId()          const { return saleId; }
    std::string getCustomer()    const { return customerName; }
    Date        getDate()        const { return saleDate; }
    bool        getRefunded()    const { return isRefunded; }
    std::string getStaff()       const { return staffUsername; }
    const std::vector<SaleItem>& getItems() const { return items; }

    void markRefunded() { isRefunded = true; }

    // Print a professional bill / receipt
    void printBill() const {
        using namespace std;
        cout << "\033[1m";
        cout << "============================================================\n";
        cout << "               INVENTORY MANAGEMENT SYSTEM                  \n";
        cout << "                         RECEIPT                            \n";
        cout << "============================================================\033[0m\n";
        cout << "  Bill No   : " << saleId << "\n";
        cout << "  Customer  : " << customerName << "\n";
        cout << "  Date      : " << saleDate.toString() << "\n";
        cout << "  Staff     : " << staffUsername << "\n";
        cout << "------------------------------------------------------------\n";
        cout << left << setw(22) << "Product"
             << right << setw(6) << "Qty"
             << setw(12) << "Unit"
             << setw(10) << "Tax"
             << setw(10) << "Disc"
             << setw(12) << "Total" << "\n";
        cout << "------------------------------------------------------------\n";
        for (auto& it : items) {
            cout << left  << setw(22) << it.productName.substr(0,21)
                 << right << setw(6)  << it.quantity
                 << setw(12) << fixed << setprecision(2) << it.unitPrice
                 << setw(10) << it.taxAmount
                 << setw(10) << it.discountAmount
                 << setw(12) << it.lineTotal() << "\n";
        }
        cout << "------------------------------------------------------------\n";
        cout << right << setw(50) << "Subtotal  : Rs " << setw(10) << subtotal()    << "\n";
        cout << right << setw(50) << "Total Tax : Rs " << setw(10) << totalTax()    << "\n";
        cout << right << setw(50) << "Discount  : Rs " << setw(10) << totalDiscount()<< "\n";
        cout << "\033[1m";
        cout << right << setw(50) << "GRAND TOTAL: Rs " << setw(10) << grandTotal() << "\033[0m\n";
        if (isRefunded) cout << "\033[31m  ** THIS SALE HAS BEEN REFUNDED **\033[0m\n";
        cout << "============================================================\n";
        cout << "           Thank you for your purchase!\n";
        cout << "============================================================\n";
    }

    // Serialise items
    std::vector<std::string> toCSVRow() const {
        std::string itemStr;
        for (size_t i=0; i<items.size(); ++i) {
            if (i) itemStr += ";";
            itemStr += items[i].toCSV();
        }
        return {
            std::to_string(saleId), customerName, saleDate.toString(),
            staffUsername, isRefunded ? "1" : "0", itemStr
        };
    }
};
