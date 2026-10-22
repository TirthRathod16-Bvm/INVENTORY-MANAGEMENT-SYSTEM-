// ============================================================
//  ElectronicProduct.h  –  Derived class (Inheritance,
//                           Polymorphism – overrides calculateTax)
// ============================================================
#pragma once
#include "Product.h"
#include <vector>
#include <string>

class ElectronicProduct : public Product {
private:
    std::string warrantyMonths;  // e.g. "24"
    std::string brand;

    static constexpr double GST_RATE = 0.18;  // 18 % GST

public:
    ElectronicProduct(int id, const std::string& name, double price, int qty,
                      const std::string& brand = "",
                      const std::string& warranty = "12",
                      const std::string& supplier = "",
                      int minStock = 3, int reorder = 10)
        : Product(id, name, price, qty, supplier, minStock, reorder),
          warrantyMonths(warranty), brand(brand) {}

    // Copy constructor
    ElectronicProduct(const ElectronicProduct&) = default;

    // ------- Polymorphism: override -------------------------
    double calculateTax() const override {
        return getPrice() * GST_RATE;
    }

    std::string getCategory() const override { return "Electronic"; }
    std::string getTypeTag()  const override { return "ELEC"; }

    // Getters / setters
    std::string getBrand()          const { return brand; }
    std::string getWarrantyMonths() const { return warrantyMonths; }
    void setBrand(const std::string& b) { brand = b; }
    void setWarranty(const std::string& w) { warrantyMonths = w; }

    std::vector<std::string> toCSVRow() const override {
        return {
            std::to_string(id), name, std::to_string(getPrice()),
            std::to_string(getQuantity()), getTypeTag(),
            brand, warrantyMonths, supplierCode,
            std::to_string(minStockLevel), std::to_string(reorderQty)
        };
    }

    void printDetails() const override {
        Product::printDetails();
        std::cout << "\033[1m  Brand       :\033[0m " << brand << "\n"
                  << "\033[1m  Warranty    :\033[0m " << warrantyMonths << " months\n"
                  << "\033[1m  GST Rate    :\033[0m 18%\n";
    }
};
