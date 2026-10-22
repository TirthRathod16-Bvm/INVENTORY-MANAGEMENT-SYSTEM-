// ============================================================
//  GeneralProduct.h  –  Concrete Product for non-typed items
// ============================================================
#pragma once
#include "Product.h"
#include <vector>
#include <string>

class GeneralProduct : public Product {
private:
    double taxRate;
    std::string subCategory;

public:
    GeneralProduct(int id, const std::string& name, double price, int qty,
                   double taxRate = 0.12,
                   const std::string& sub = "General",
                   const std::string& supplier = "",
                   int minStock = 5, int reorder = 20)
        : Product(id, name, price, qty, supplier, minStock, reorder),
          taxRate(taxRate), subCategory(sub) {}

    GeneralProduct(const GeneralProduct&) = default;

    double calculateTax() const override { return getPrice() * taxRate; }
    std::string getCategory() const override { return subCategory.empty() ? "General" : subCategory; }
    std::string getTypeTag()  const override { return "GEN"; }

    void setSubCategory(const std::string& s) { subCategory = s; }
    void setTaxRate(double r) { taxRate = r; }
    double getTaxRate() const { return taxRate; }

    std::vector<std::string> toCSVRow() const override {
        return {
            std::to_string(id), name, std::to_string(getPrice()),
            std::to_string(getQuantity()), getTypeTag(),
            std::to_string(taxRate), subCategory, supplierCode,
            std::to_string(minStockLevel), std::to_string(reorderQty)
        };
    }

    void printDetails() const override {
        Product::printDetails();
        std::cout << "\033[1m  Sub-Category:\033[0m " << subCategory << "\n"
                  << "\033[1m  Tax Rate    :\033[0m " << (taxRate*100) << "%\n";
    }
};
