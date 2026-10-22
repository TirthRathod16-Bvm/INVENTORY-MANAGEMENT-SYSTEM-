// ============================================================
//  Exceptions.h  –  Custom exception hierarchy
// ============================================================
#pragma once
#include <stdexcept>
#include <string>

// Base exception for all inventory errors
class InventoryException : public std::runtime_error {
public:
    explicit InventoryException(const std::string& msg)
        : std::runtime_error("[InventoryError] " + msg) {}
};

class InvalidPriceException : public InventoryException {
public:
    explicit InvalidPriceException(double price)
        : InventoryException("Invalid price: " + std::to_string(price) +
                             ". Price must be >= 0.") {}
};

class InvalidQuantityException : public InventoryException {
public:
    explicit InvalidQuantityException(int qty)
        : InventoryException("Invalid quantity: " + std::to_string(qty) +
                             ". Quantity must be >= 0.") {}
};

class InsufficientStockException : public InventoryException {
public:
    InsufficientStockException(const std::string& productName, int avail, int requested)
        : InventoryException("Insufficient stock for '" + productName +
                             "'. Available: " + std::to_string(avail) +
                             ", Requested: " + std::to_string(requested)) {}
};

class ProductNotFoundException : public InventoryException {
public:
    explicit ProductNotFoundException(const std::string& key)
        : InventoryException("Product not found: " + key) {}
};

class DuplicateProductException : public InventoryException {
public:
    explicit DuplicateProductException(int id)
        : InventoryException("Product with ID " + std::to_string(id) + " already exists.") {}
};

class AuthenticationException : public InventoryException {
public:
    explicit AuthenticationException(const std::string& msg = "Invalid username or password.")
        : InventoryException(msg) {}
};

class PermissionDeniedException : public InventoryException {
public:
    explicit PermissionDeniedException(const std::string& action)
        : InventoryException("Permission denied: You cannot perform '" + action + "'.") {}
};

class FileIOException : public InventoryException {
public:
    explicit FileIOException(const std::string& filename)
        : InventoryException("Failed to open file: " + filename) {}
};

class InvalidInputException : public InventoryException {
public:
    explicit InvalidInputException(const std::string& msg)
        : InventoryException("Invalid input: " + msg) {}
};

class SaleNotFoundException : public InventoryException {
public:
    explicit SaleNotFoundException(int saleId)
        : InventoryException("Sale record not found: ID " + std::to_string(saleId)) {}
};

class DuplicateRefundException : public InventoryException {
public:
    explicit DuplicateRefundException(int saleId)
        : InventoryException("Sale ID " + std::to_string(saleId) + " has already been refunded.") {}
};

class ExpiredProductException : public InventoryException {
public:
    explicit ExpiredProductException(const std::string& productName, const std::string& expiryDate)
        : InventoryException("Product '" + productName + "' expired on " + expiryDate + ".") {}
};
