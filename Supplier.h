// ============================================================
//  Supplier.h  –  Supplier + PurchaseOrder models
// ============================================================
#pragma once
#include <string>
#include <vector>
#include <iomanip>
#include <iostream>
#include "../utils/DateUtils.h"
#include "../utils/Exceptions.h"

enum class POStatus { PENDING, RECEIVED, CANCELLED };

inline std::string poStatusToStr(POStatus s) {
    switch(s) {
        case POStatus::PENDING:   return "Pending";
        case POStatus::RECEIVED:  return "Received";
        case POStatus::CANCELLED: return "Cancelled";
        default:                  return "Unknown";
    }
}

inline POStatus strToPOStatus(const std::string& s) {
    if (s == "Received")  return POStatus::RECEIVED;
    if (s == "Cancelled") return POStatus::CANCELLED;
    return POStatus::PENDING;
}

// ---- Purchase Order Line Item --------------------------------
struct POItem {
    int    productId;
    std::string productName;
    int    quantity;
    double unitCost;

    std::string toCSV() const {
        return std::to_string(productId) + "~" + productName + "~" +
               std::to_string(quantity) + "~" + std::to_string(unitCost);
    }
    static POItem fromCSV(const std::string& s) {
        POItem it{};
        std::vector<std::string> p;
        std::string cur;
        for (char c : s) { if (c=='~') { p.push_back(cur); cur.clear(); } else cur+=c; }
        p.push_back(cur);
        if (p.size() >= 4) {
            it.productId   = std::stoi(p[0]);
            it.productName = p[1];
            it.quantity    = std::stoi(p[2]);
            it.unitCost    = std::stod(p[3]);
        }
        return it;
    }
};

// ---- Purchase Order -----------------------------------------
class PurchaseOrder {
private:
    int         poId;
    std::string supplierCode;
    Date        orderDate;
    Date        expectedDelivery;
    POStatus    status;
    std::vector<POItem> items;
    std::string notes;

public:
    PurchaseOrder(int id, const std::string& supplier,
                  const Date& orderDate, const Date& expected,
                  const std::string& notes = "")
        : poId(id), supplierCode(supplier), orderDate(orderDate),
          expectedDelivery(expected), status(POStatus::PENDING), notes(notes) {}

    void addItem(const POItem& item) { items.push_back(item); }

    double totalCost() const {
        double t = 0;
        for (auto& it : items) t += it.unitCost * it.quantity;
        return t;
    }

    void markReceived()  { status = POStatus::RECEIVED; }
    void markCancelled() { status = POStatus::CANCELLED; }

    int         getId()           const { return poId; }
    std::string getSupplierCode() const { return supplierCode; }
    Date        getOrderDate()    const { return orderDate; }
    Date        getExpectedDate() const { return expectedDelivery; }
    POStatus    getStatus()       const { return status; }
    const std::vector<POItem>& getItems() const { return items; }
    std::string getNotes()        const { return notes; }

    // Serialise items as semicolon-separated CSV entries
    std::vector<std::string> toCSVRow() const {
        std::string itemStr;
        for (size_t i=0; i<items.size(); ++i) {
            if (i) itemStr += ";";
            itemStr += items[i].toCSV();
        }
        return {
            std::to_string(poId), supplierCode,
            orderDate.toString(), expectedDelivery.toString(),
            poStatusToStr(status), itemStr, notes
        };
    }

    void print() const {
        std::cout << "  PO #" << poId
                  << " | Supplier: " << supplierCode
                  << " | Date: " << orderDate.toString()
                  << " | Expected: " << expectedDelivery.toString()
                  << " | Status: " << poStatusToStr(status)
                  << " | Total: Rs " << std::fixed << std::setprecision(2) << totalCost() << "\n";
        for (auto& it : items)
            std::cout << "    - " << it.productName << " x" << it.quantity
                      << " @ Rs " << it.unitCost << "\n";
    }
};

// ---- Supplier -----------------------------------------------
class Supplier {
private:
    std::string code;      // unique key e.g. "SUP001"
    std::string name;
    std::string contact;
    std::string email;
    std::string address;

public:
    Supplier(const std::string& code, const std::string& name,
             const std::string& contact = "", const std::string& email = "",
             const std::string& address = "")
        : code(code), name(name), contact(contact),
          email(email), address(address) {}

    std::string getCode()    const { return code; }
    std::string getName()    const { return name; }
    std::string getContact() const { return contact; }
    std::string getEmail()   const { return email; }
    std::string getAddress() const { return address; }

    void setName(const std::string& n)    { name = n; }
    void setContact(const std::string& c) { contact = c; }
    void setEmail(const std::string& e)   { email = e; }
    void setAddress(const std::string& a) { address = a; }

    std::vector<std::string> toCSVRow() const {
        return { code, name, contact, email, address };
    }

    void print() const {
        std::cout << "  Code: " << code
                  << " | Name: " << name
                  << " | Contact: " << contact
                  << " | Email: " << email << "\n";
    }
};

