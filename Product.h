// ============================================================
//  Product.h  –  Abstract base class (demonstrating abstraction,
//                encapsulation, constructors, operator overloading)
// ============================================================
#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include "../utils/Exceptions.h"

enum class ProductCategory { ELECTRONIC, GROCERY, CLOTHING, GENERAL };

inline std::string categoryToStr(ProductCategory c) {
    switch(c) {
        case ProductCategory::ELECTRONIC: return "Electronic";
        case ProductCategory::GROCERY:    return "Grocery";
        case ProductCategory::CLOTHING:   return "Clothing";
        default:                           return "General";
    }
}

inline ProductCategory strToCategory(const std::string& s) {
    if (s == "Electronic") return ProductCategory::ELECTRONIC;
    if (s == "Grocery")    return ProductCategory::GROCERY;
    if (s == "Clothing")   return ProductCategory::CLOTHING;
    return ProductCategory::GENERAL;
}

// ============================================================
//  Abstract Product
// ============================================================
class Product {
private:
    double price;        // encapsulated – private
    int    quantity;     // encapsulated – private

protected:
    int         id;
    std::string name;
    std::string supplierCode;
    int         minStockLevel;
    int         reorderQty;

public:
    // Parameterised constructor
    Product(int id, const std::string& name, double price, int qty,
            const std::string& supplierCode = "",
            int minStock = 5, int reorderQty = 20)
        : id(id), name(name), supplierCode(supplierCode),
          minStockLevel(minStock), reorderQty(reorderQty)
    {
        setPrice(price);
        setQuantity(qty);
    }

    // Copy constructor
    Product(const Product&) = default;
    virtual ~Product() = default;

    // ------- Pure virtual interface (abstraction) ----------
    virtual double          calculateTax()     const = 0;
    virtual std::string     getCategory()      const = 0;
    virtual std::string     getTypeTag()       const = 0;   // for serialisation
    virtual std::vector<std::string> toCSVRow()const = 0;

    // ------- Validated setters (encapsulation) -------------
    void setPrice(double p) {
        if (p < 0) throw InvalidPriceException(p);
        price = p;
    }
    void setQuantity(int q) {
        if (q < 0) throw InvalidQuantityException(q);
        quantity = q;
    }
    void setName(const std::string& n) {
        if (n.empty()) throw InvalidInputException("Name cannot be empty.");
        name = n;
    }
    void setMinStockLevel(int m) { minStockLevel = m; }
    void setReorderQty(int r)    { reorderQty = r; }
    void setSupplierCode(const std::string& s) { supplierCode = s; }

    // ------- Getters ---------------------------------------
    int         getId()           const { return id; }
    std::string getName()         const { return name; }
    double      getPrice()        const { return price; }
    int         getQuantity()     const { return quantity; }
    std::string getSupplierCode() const { return supplierCode; }
    int         getMinStockLevel()const { return minStockLevel; }
    int         getReorderQty()   const { return reorderQty; }

    // Final price including tax
    double getPriceWithTax() const { return price + calculateTax(); }

    // Total value of all stock of this product
    double totalValue() const { return price * quantity; }

    // Is stock below minimum?
    bool isLowStock() const { return quantity < minStockLevel; }
    bool isOutOfStock() const { return quantity == 0; }
    int  unitsToReorder() const {
        return (quantity < minStockLevel) ? reorderQty : 0;
    }

    // Stock operations
    void addStock(int units) {
        if (units <= 0) throw InvalidQuantityException(units);
        quantity += units;
    }
    void removeStock(int units) {
        if (units <= 0) throw InvalidQuantityException(units);
        if (units > quantity) throw InsufficientStockException(name, quantity, units);
        quantity -= units;
    }

    // ------- Operator overloading --------------------------
    bool operator==(const Product& o) const { return id == o.id; }
    bool operator< (const Product& o) const { return id <  o.id; }

    // Stream insertion
    friend std::ostream& operator<<(std::ostream& os, const Product& p) {
        os << std::left
           << std::setw(6)  << p.id
           << std::setw(22) << p.name.substr(0, 21)
           << std::setw(13) << p.getCategory().substr(0, 12)
           << std::right
           << std::setw(10) << std::fixed << std::setprecision(2) << p.price
           << std::setw(8)  << p.quantity;
        if (p.isOutOfStock())
            os << "  \033[31m[OUT]\033[0m";
        else if (p.isLowStock())
            os << "  \033[33m[LOW]\033[0m";
        else
            os << "  \033[32m[OK] \033[0m";
        return os;
    }

    // Print detailed record
    virtual void printDetails() const {
        using namespace std;
        cout << "\033[1m  ID          :\033[0m " << id          << "\n"
             << "\033[1m  Name        :\033[0m " << name        << "\n"
             << "\033[1m  Category    :\033[0m " << getCategory()<< "\n"
             << "\033[1m  Price       :\033[0m Rs " << fixed << setprecision(2) << price << "\n"
             << "\033[1m  Tax         :\033[0m Rs " << calculateTax() << "\n"
             << "\033[1m  Price+Tax   :\033[0m Rs " << getPriceWithTax() << "\n"
             << "\033[1m  Stock       :\033[0m " << quantity << "\n"
             << "\033[1m  Min Level   :\033[0m " << minStockLevel << "\n"
             << "\033[1m  Reorder Qty :\033[0m " << reorderQty << "\n"
             << "\033[1m  Supplier    :\033[0m " << supplierCode << "\n"
             << "\033[1m  Total Value :\033[0m Rs " << totalValue() << "\n";
    }
};
