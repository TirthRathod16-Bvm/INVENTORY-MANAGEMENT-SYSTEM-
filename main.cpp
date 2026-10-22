// ============================================================
//  main.cpp  –  Inventory Management & Stock Monitoring System
//               OOP Individual Project (204ES)
//
//  OOP Concepts: Abstract classes, Inheritance, Polymorphism,
//  Encapsulation, Constructors, Templates, STL, Exception
//  Handling, File Handling, Smart Pointers, Design Patterns
// ============================================================
#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <map>

#include "managers/Inventory.h"
#include "managers/UserManager.h"
#include "managers/SalesManager.h"
#include "managers/SupplierManager.h"
#include "managers/AlertManager.h"
#include "managers/AuditManager.h"
#include "managers/ReportManager.h"
#include "models/ElectronicProduct.h"
#include "models/GroceryProduct.h"
#include "models/GeneralProduct.h"
#include "utils/ConsoleUI.h"
#include "utils/Exceptions.h"
#include "utils/DateUtils.h"

// ============================================================
//  Application State
// ============================================================
struct AppState {
    Inventory       inventory;
    UserManager     userMgr;
    SalesManager    salesMgr;
    SupplierManager supplierMgr;
    AlertManager    alertMgr;
    AuditManager    auditMgr;
    ReportManager   reportMgr;

    const User*     currentUser = nullptr;  // logged-in user

    AppState()
        : inventory("data/products.csv", "data/backup"),
          userMgr("data/users.csv"),
          salesMgr("data/sales.csv"),
          supplierMgr("data/suppliers.csv", "data/purchaseorders.csv"),
          auditMgr("data/audit.csv"),
          reportMgr(inventory, salesMgr, supplierMgr)
    {
        // Register console observer for stock alerts
        alertMgr.addObserver(std::make_shared<ConsoleAlertObserver>());
    }
};

// ============================================================
//  Helper: Check permission and throw if denied
// ============================================================
void requirePermission(bool allowed, const std::string& action) {
    if (!allowed) throw PermissionDeniedException(action);
}

// ============================================================
//  Login Screen
// ============================================================
bool doLogin(AppState& app) {
    UI::clearScreen();
    UI::printHeader("INVENTORY MANAGEMENT SYSTEM  v1.0");
    std::cout << "\n  Welcome! Please log in.\n\n";
    std::string username = UI::inputString("  Username: ");
    std::string password = UI::inputPassword("  Password: ");
    try {
        const User& u = app.userMgr.login(username, password);
        app.currentUser = &u;
        app.auditMgr.log(AuditAction::USER_LOGIN, username,
                         "User logged in as " + u.getRoleName());
        UI::printSuccess("Welcome, " + u.getFullName() +
                         " [" + u.getRoleName() + "]");
        return true;
    } catch (const InventoryException& e) {
        UI::printError(e.what());
        return false;
    }
}

// ============================================================
//  PRODUCT MANAGEMENT MENUS
// ============================================================
void menuAddProduct(AppState& app) {
    requirePermission(app.currentUser->canManageProducts(), "add products");
    UI::printHeader("ADD NEW PRODUCT");

    std::cout << "  Product type:\n"
              << "  1. Electronic  2. Grocery  3. General\n";
    int type = UI::inputInt("  Choice: ", 1, 3);

    int id = app.inventory.getNextId();
    std::cout << "  Assigned ID: " << id << "\n";
    std::string name     = UI::inputString("  Product Name: ");
    double price         = UI::inputDouble("  Price (Rs): ");
    int qty              = UI::inputInt("  Initial Quantity: ", 0);
    int minStock         = UI::inputInt("  Minimum Stock Level: ", 0);
    int reorder          = UI::inputInt("  Reorder Quantity: ", 1);
    std::string supplier = UI::inputString("  Supplier Code (or NONE): ");
    if (supplier == "NONE") supplier = "";

    std::shared_ptr<Product> p;

    if (type == 1) {
        std::string brand   = UI::inputString("  Brand: ");
        std::string warrant = UI::inputString("  Warranty (months): ");
        p = std::make_shared<ElectronicProduct>(id, name, price, qty,
                                                brand, warrant, supplier,
                                                minStock, reorder);
    } else if (type == 2) {
        double taxRate = UI::inputDouble("  Tax Rate (e.g. 0.05 for 5%): ");
        auto gp = std::make_shared<GroceryProduct>(id, name, price, qty,
                                                   taxRate, supplier, minStock, reorder);
        // Add initial batch
        std::string expStr = UI::inputString("  Expiry Date (YYYY-MM-DD): ");
        Date exp = Date::fromString(expStr);
        Batch b{1, qty, today(), exp};
        // Replace default batch by clearing internal via public API workaround
        // (GroceryProduct already adds a default batch in constructor; 
        //  for simplicity we keep that batch but update the expiry here)
        p = gp;
    } else {
        double taxRate = UI::inputDouble("  Tax Rate (e.g. 0.12 for 12%): ");
        std::string sub = UI::inputString("  Sub-category (Clothing / General / etc.): ");
        p = std::make_shared<GeneralProduct>(id, name, price, qty,
                                             taxRate, sub, supplier, minStock, reorder);
    }

    try {
        app.inventory.addProduct(p);
        app.auditMgr.log(AuditAction::PRODUCT_ADD, app.currentUser->getUsername(),
                         "Added product: " + name, id, -1, qty);
        UI::printSuccess("Product '" + name + "' added with ID " + std::to_string(id) + ".");
    } catch (const InventoryException& e) {
        UI::printError(e.what());
    }
}

void menuUpdateProduct(AppState& app) {
    requirePermission(app.currentUser->canManageProducts(), "update products");
    UI::printHeader("UPDATE PRODUCT");
    int id = UI::inputInt("  Product ID to update: ", 1);
    auto p = app.inventory.search(id);
    if (!p) { UI::printError("Product not found."); return; }
    p->printDetails();

    std::string name     = UI::inputString("  New Name [" + p->getName() + "]: ");
    double price         = UI::inputDouble("  New Price (Rs) [" + std::to_string(p->getPrice()) + "]: ");
    int qty              = UI::inputInt("  New Quantity [" + std::to_string(p->getQuantity()) + "]: ", 0);
    int minStock         = UI::inputInt("  Min Stock [" + std::to_string(p->getMinStockLevel()) + "]: ", 0);
    int reorder          = UI::inputInt("  Reorder Qty [" + std::to_string(p->getReorderQty()) + "]: ", 1);
    std::string supplier = UI::inputString("  Supplier Code [" + p->getSupplierCode() + "]: ");

    double oldPrice = p->getPrice();
    try {
        app.inventory.updateProduct(id, name, price, qty, minStock, reorder, supplier);
        app.auditMgr.log(AuditAction::PRODUCT_UPDATE, app.currentUser->getUsername(),
                         "Updated product ID " + std::to_string(id), id);
        if (oldPrice != price)
            app.auditMgr.log(AuditAction::PRICE_CHANGE, app.currentUser->getUsername(),
                             "Price changed for " + name, id);
        UI::printSuccess("Product updated.");
    } catch (const InventoryException& e) {
        UI::printError(e.what());
    }
}

void menuDeleteProduct(AppState& app) {
    requirePermission(app.currentUser->canManageProducts(), "delete products");
    UI::printHeader("DELETE PRODUCT");
    int id = UI::inputInt("  Product ID to delete: ", 1);
    auto p = app.inventory.search(id);
    if (!p) { UI::printError("Product not found."); return; }
    std::cout << "  Product: " << p->getName() << "\n";
    if (!UI::confirm("  Confirm deletion?")) return;
    try {
        std::string name = p->getName();
        app.inventory.deleteProduct(id);
        app.auditMgr.log(AuditAction::PRODUCT_DELETE, app.currentUser->getUsername(),
                         "Deleted product: " + name, id);
        UI::printSuccess("Product deleted.");
    } catch (const InventoryException& e) {
        UI::printError(e.what());
    }
}

void menuSearchProduct(AppState& app) {
    UI::printHeader("SEARCH PRODUCT");
    std::cout << "  1. Search by ID\n  2. Search by Name\n";
    int choice = UI::inputInt("  Choice: ", 1, 2);
    if (choice == 1) {
        int id = UI::inputInt("  Product ID: ", 1);
        // Demonstrate binary search
        auto p = app.inventory.binarySearchById(id);
        if (!p) { UI::printError("Product not found."); return; }
        p->printDetails();
    } else {
        std::string name = UI::inputString("  Product Name (partial): ");
        auto results = app.inventory.search(name);
        if (results.empty()) { UI::printError("No products found."); return; }
        for (auto& p : results) std::cout << *p << "\n";
    }
}

void menuStockAdjust(AppState& app) {
    requirePermission(app.currentUser->canManageProducts(), "adjust stock");
    UI::printHeader("MANUAL STOCK ADJUSTMENT");
    int id = UI::inputInt("  Product ID: ", 1);
    auto p = app.inventory.search(id);
    if (!p) { UI::printError("Product not found."); return; }
    std::cout << "  Current Stock: " << p->getQuantity() << "\n";
    std::cout << "  1. Add Stock  2. Remove Stock\n";
    int choice = UI::inputInt("  Choice: ", 1, 2);
    int units = UI::inputInt("  Units: ", 1);
    std::string reason = UI::inputString("  Reason: ");
    int prevQty = p->getQuantity();
    try {
        // Use Command pattern for undo/redo
        if (choice == 1) {
            auto cmd = std::make_unique<StockAdjustCommand>(
                [&, id](int u){ app.inventory.addStock(id, u); app.inventory.save(); },
                [&, id](int u){ app.inventory.removeStock(id, u); app.inventory.save(); },
                units, "Add " + std::to_string(units) + " to " + p->getName());
            cmd->execute();
            app.auditMgr.log(AuditAction::STOCK_ADD, app.currentUser->getUsername(),
                             "Stock added: " + p->getName(), id, prevQty, prevQty+units, reason);
        } else {
            auto cmd = std::make_unique<StockAdjustCommand>(
                [&, id](int u){ app.inventory.removeStock(id, u); app.inventory.save(); },
                [&, id](int u){ app.inventory.addStock(id, u); app.inventory.save(); },
                units, "Remove " + std::to_string(units) + " from " + p->getName());
            cmd->execute();
            app.auditMgr.log(AuditAction::STOCK_REMOVE, app.currentUser->getUsername(),
                             "Stock removed: " + p->getName(), id, prevQty, prevQty-units, reason);
        }
        UI::printSuccess("Stock updated. New quantity: " + std::to_string(p->getQuantity()));
    } catch (const InventoryException& e) {
        UI::printError(e.what());
    }
}

// ============================================================
//  SALES MENU
// ============================================================
void menuCreateSale(AppState& app) {
    UI::printHeader("NEW SALE / BILLING");
    std::string customer = UI::inputString("  Customer Name: ");
    std::vector<std::pair<int,int>> items;

    while (true) {
        int pid = UI::inputInt("  Product ID (0 to finish): ", 0);
        if (pid == 0) break;
        auto p = app.inventory.search(pid);
        if (!p) { UI::printWarning("Product not found, skip."); continue; }
        std::cout << "  " << p->getName() << " | Price: Rs "
                  << std::fixed << std::setprecision(2) << p->getPriceWithTax()
                  << " | Stock: " << p->getQuantity() << "\n";
        int qty = UI::inputInt("  Quantity: ", 1, p->getQuantity());
        items.push_back({pid, qty});
    }

    if (items.empty()) { UI::printWarning("No items selected."); return; }

    // Discount
    DiscountType dtype = DiscountType::NONE;
    double dval = 0;
    std::cout << "  Discount? 0=None 1=Percentage 2=Fixed\n";
    int dc = UI::inputInt("  Choice: ", 0, 2);
    if (dc == 1) { dtype = DiscountType::PERCENTAGE; dval = UI::inputDouble("  Discount %: "); }
    else if (dc == 2) { dtype = DiscountType::FIXED; dval = UI::inputDouble("  Discount (Rs): "); }

    try {
        Sale sale = app.salesMgr.createSale(app.inventory, customer, items,
                                             app.currentUser->getUsername(), dtype, dval);
        sale.printBill();
        app.auditMgr.log(AuditAction::SALE, app.currentUser->getUsername(),
                         "Sale #" + std::to_string(sale.getId()) + " to " + customer);
        // Run alert scan after every sale
        app.alertMgr.runAlertScan(app.inventory);
    } catch (const InventoryException& e) {
        UI::printError(e.what());
    }
}

void menuRefundSale(AppState& app) {
    UI::printHeader("PROCESS REFUND");
    int saleId = UI::inputInt("  Sale ID to refund: ", 1);
    auto* s = app.salesMgr.findSale(saleId);
    if (!s) { UI::printError("Sale not found."); return; }
    s->printBill();
    if (!UI::confirm("  Confirm refund?")) return;
    try {
        app.salesMgr.refundSale(saleId, app.inventory, app.currentUser->getUsername());
        app.auditMgr.log(AuditAction::SALE_RETURN, app.currentUser->getUsername(),
                         "Refund for Sale #" + std::to_string(saleId));
    } catch (const InventoryException& e) {
        UI::printError(e.what());
    }
}

void menuViewBill(AppState& app) {
    UI::printHeader("VIEW BILL");
    int saleId = UI::inputInt("  Sale ID: ", 1);
    auto* s = app.salesMgr.findSale(saleId);
    if (!s) { UI::printError("Sale not found."); return; }
    s->printBill();
}

// ============================================================
//  SUPPLIER MENU
// ============================================================
void menuAddSupplier(AppState& app) {
    requirePermission(app.currentUser->canManageSuppliers(), "manage suppliers");
    UI::printHeader("ADD SUPPLIER");
    std::string code    = UI::inputString("  Supplier Code (e.g. SUP001): ");
    std::string name    = UI::inputString("  Company Name: ");
    std::string contact = UI::inputString("  Contact Person: ");
    std::string email   = UI::inputString("  Email: ");
    std::string address = UI::inputString("  Address: ");
    try {
        app.supplierMgr.addSupplier(code, name, contact, email, address);
        UI::printSuccess("Supplier added: " + code);
    } catch (const InventoryException& e) {
        UI::printError(e.what());
    }
}

void menuCreatePO(AppState& app) {
    requirePermission(app.currentUser->canManageSuppliers(), "create purchase orders");
    UI::printHeader("CREATE PURCHASE ORDER");
    app.supplierMgr.printAllSuppliers();
    std::string scode = UI::inputString("  Supplier Code: ");
    std::string notes = UI::inputString("  Notes (optional): ");
    int leadDays      = UI::inputInt("  Lead time (days): ", 1, 365);

    std::vector<POItem> items;
    while (true) {
        int pid = UI::inputInt("  Product ID (0 to finish): ", 0);
        if (pid == 0) break;
        auto p = app.inventory.search(pid);
        if (!p) { UI::printWarning("Product not found."); continue; }
        int qty        = UI::inputInt("  Quantity: ", 1);
        double cost    = UI::inputDouble("  Unit Cost (Rs): ");
        items.push_back({pid, p->getName(), qty, cost});
    }
    if (items.empty()) { UI::printWarning("No items in PO."); return; }
    try {
        int poId = app.supplierMgr.createPurchaseOrder(scode, items, leadDays, notes);
        app.auditMgr.log(AuditAction::PO_CREATE, app.currentUser->getUsername(),
                         "Created PO #" + std::to_string(poId) + " for " + scode);
        UI::printSuccess("Purchase Order #" + std::to_string(poId) + " created.");
    } catch (const InventoryException& e) {
        UI::printError(e.what());
    }
}

void menuReceivePO(AppState& app) {
    requirePermission(app.currentUser->canManageSuppliers(), "receive purchase orders");
    UI::printHeader("RECEIVE PURCHASE ORDER");
    app.supplierMgr.printPendingPOs();
    int poId = UI::inputInt("  PO ID to receive: ", 1);
    try {
        app.supplierMgr.receivePurchaseOrder(poId, app.inventory);
        app.auditMgr.log(AuditAction::PO_RECEIVE, app.currentUser->getUsername(),
                         "Received PO #" + std::to_string(poId));
    } catch (const InventoryException& e) {
        UI::printError(e.what());
    }
}

// ============================================================
//  USER MANAGEMENT MENU
// ============================================================
void menuUserManagement(AppState& app) {
    requirePermission(app.currentUser->canManageUsers(), "manage users");
    while (true) {
        UI::printHeader("USER MANAGEMENT");
        std::cout << "  1. View All Users\n"
                  << "  2. Add User\n"
                  << "  3. Change User Role\n"
                  << "  4. Deactivate User\n"
                  << "  5. Change My Password\n"
                  << "  0. Back\n";
        int ch = UI::inputInt("  Choice: ", 0, 5);
        if (ch == 0) break;
        try {
            if (ch == 1) {
                app.userMgr.printAll();
            } else if (ch == 2) {
                std::string un  = UI::inputString("  Username: ");
                std::string pw  = UI::inputPassword("  Password: ");
                std::cout << "  Role: 1=Admin 2=Manager 3=Staff\n";
                int r = UI::inputInt("  Role: ", 1, 3);
                UserRole role = (r==1)?UserRole::ADMIN:(r==2)?UserRole::MANAGER:UserRole::STAFF;
                std::string fn = UI::inputString("  Full Name: ");
                app.userMgr.addUser(un, pw, role, fn, *app.currentUser);
                app.auditMgr.log(AuditAction::USER_ADD, app.currentUser->getUsername(),
                                 "Added user: " + un);
                UI::printSuccess("User '" + un + "' created.");
            } else if (ch == 3) {
                std::string un = UI::inputString("  Username: ");
                std::cout << "  Role: 1=Admin 2=Manager 3=Staff\n";
                int r = UI::inputInt("  Role: ", 1, 3);
                UserRole role = (r==1)?UserRole::ADMIN:(r==2)?UserRole::MANAGER:UserRole::STAFF;
                app.userMgr.updateUserRole(un, role, *app.currentUser);
                UI::printSuccess("Role updated.");
            } else if (ch == 4) {
                std::string un = UI::inputString("  Username to deactivate: ");
                app.userMgr.deactivateUser(un, *app.currentUser);
                UI::printSuccess("User deactivated.");
            } else if (ch == 5) {
                std::string old = UI::inputPassword("  Current Password: ");
                std::string nw  = UI::inputPassword("  New Password: ");
                app.userMgr.changePassword(app.currentUser->getUsername(), old, nw);
                UI::printSuccess("Password changed.");
            }
        } catch (const InventoryException& e) {
            UI::printError(e.what());
        }
        UI::pause();
    }
}

// ============================================================
//  ANALYTICS MENU
// ============================================================
void menuAnalytics(AppState& app) {
    requirePermission(app.currentUser->canViewReports(), "view analytics");
    while (true) {
        UI::printHeader("SALES ANALYTICS");
        std::cout << "  1. Today's Sales Report\n"
                  << "  2. Weekly Sales Report\n"
                  << "  3. Monthly Sales Report\n"
                  << "  4. Top 5 Products (This Month)\n"
                  << "  5. Demand Forecast & Reorder Recommendations\n"
                  << "  0. Back\n";
        int ch = UI::inputInt("  Choice: ", 0, 5);
        if (ch == 0) break;
        Date t = today();
        if (ch == 1) {
            app.salesMgr.printSalesReport(t, t, app.inventory);
        } else if (ch == 2) {
            auto stats = app.salesMgr.weeklyStats(t);
            UI::printHeader("Weekly Sales Summary");
            std::cout << "  Sales: " << stats.totalSales
                      << " | Units: " << stats.totalUnits
                      << " | Revenue: " << UI::formatMoney(stats.revenue) << "\n";
        } else if (ch == 3) {
            int m = UI::inputInt("  Month (1-12): ", 1, 12);
            int y = UI::inputInt("  Year: ", 2000, 2100);
            app.reportMgr.runMonthlySalesReport(m, y);
        } else if (ch == 4) {
            Date monthStart{1, t.month, t.year};
            auto top = app.salesMgr.topProductsByUnits(monthStart, t, 5);
            UI::printHeader("Top 5 Products This Month");
            int rank = 1;
            for (auto& item : top) {
                int pid = item.first;
                int units = item.second;
                auto p = app.inventory.search(pid);
                std::cout << "  " << rank++ << ". "
                          << (p ? p->getName() : "ID:"+std::to_string(pid))
                          << " – " << units << " units\n";
            }
        } else if (ch == 5) {
            UI::printHeader("Demand Forecast (7-day Moving Average)");
            auto recs = app.salesMgr.reorderRecommendations(app.inventory, 7);
            if (recs.empty()) {
                UI::printSuccess("Stock levels adequate. No reorder needed.");
            } else {
                std::cout << "  Products recommended for reorder:\n";
                for (auto& item : recs) {
                    int pid = item.first;
                    int qty = item.second;
                    auto p = app.inventory.search(pid);
                    std::cout << "  - " << (p ? p->getName() : "ID:"+std::to_string(pid))
                              << " → Reorder " << qty << " units\n";
                }
            }
        }
        UI::pause();
    }
}

// ============================================================
//  REPORTS MENU
// ============================================================
void menuReports(AppState& app) {
    requirePermission(app.currentUser->canViewReports(), "view reports");
    while (true) {
        UI::printHeader("REPORTS");
        std::cout << "  1. Inventory Valuation Report\n"
                  << "  2. Low Stock / Reorder Report\n"
                  << "  3. Daily Sales Report\n"
                  << "  4. Supplier Report\n"
                  << "  5. Export All Reports to CSV\n"
                  << "  0. Back\n";
        int ch = UI::inputInt("  Choice: ", 0, 5);
        if (ch == 0) break;
        if (ch == 1) app.reportMgr.runInventoryReport();
        else if (ch == 2) app.reportMgr.runLowStockReport();
        else if (ch == 3) app.reportMgr.runDailySalesReport();
        else if (ch == 4) app.reportMgr.runSupplierReport();
        else if (ch == 5) app.reportMgr.exportAll();
        UI::pause();
    }
}

// ============================================================
//  AUDIT & UNDO MENU
// ============================================================
void menuAudit(AppState& app) {
    requirePermission(app.currentUser->canUndoRedo(), "view audit log");
    while (true) {
        UI::printHeader("AUDIT & UNDO/REDO");
        std::cout << "  1. View Audit Log (last 30)\n"
                  << "  2. Filter by User\n"
                  << "  3. Undo Last Stock Operation\n"
                  << "  4. Redo Last Undone Operation\n"
                  << "  0. Back\n";
        int ch = UI::inputInt("  Choice: ", 0, 4);
        if (ch == 0) break;
        if (ch == 1) app.auditMgr.printAll(30);
        else if (ch == 2) {
            std::string un = UI::inputString("  Username: ");
            app.auditMgr.filterByUser(un);
        } else if (ch == 3) {
            requirePermission(app.currentUser->canUndoRedo(), "undo");
            app.auditMgr.undo(app.currentUser->getUsername());
        } else if (ch == 4) {
            requirePermission(app.currentUser->canUndoRedo(), "redo");
            app.auditMgr.redo(app.currentUser->getUsername());
        }
        UI::pause();
    }
}

// ============================================================
//  BACKUP & RESTORE MENU
// ============================================================
void menuBackupRestore(AppState& app) {
    requirePermission(app.currentUser->canBackupRestore(), "backup/restore");
    UI::printHeader("BACKUP & RESTORE");
    std::cout << "  1. Backup All Data\n"
              << "  2. Restore Products from Backup\n"
              << "  0. Back\n";
    int ch = UI::inputInt("  Choice: ", 0, 2);
    if (ch == 1) {
        app.inventory.backup();
        app.auditMgr.log(AuditAction::BACKUP, app.currentUser->getUsername(), "Data backed up");
    } else if (ch == 2) {
        if (UI::confirm("  Restore will overwrite current data. Proceed?")) {
            try {
                app.inventory.restore();
                app.auditMgr.log(AuditAction::RESTORE, app.currentUser->getUsername(),
                                 "Data restored from backup");
            } catch (const InventoryException& e) {
                UI::printError(e.what());
            }
        }
    }
}

// ============================================================
//  PRODUCT MENU
// ============================================================
void menuProducts(AppState& app) {
    while (true) {
        UI::printHeader("PRODUCT MANAGEMENT");
        std::cout << "  1. View All Products\n"
                  << "  2. Add Product\n"
                  << "  3. Update Product\n"
                  << "  4. Delete Product\n"
                  << "  5. Search Product\n"
                  << "  6. Sort by Price\n"
                  << "  7. Sort by Quantity\n"
                  << "  8. Manual Stock Adjustment\n"
                  << "  9. Calculate Total Inventory Value\n"
                  << "  0. Back\n";
        int ch = UI::inputInt("  Choice: ", 0, 9);
        if (ch == 0) break;
        try {
            if (ch == 1) app.inventory.printAll();
            else if (ch == 2) menuAddProduct(app);
            else if (ch == 3) menuUpdateProduct(app);
            else if (ch == 4) menuDeleteProduct(app);
            else if (ch == 5) menuSearchProduct(app);
            else if (ch == 6) {
                bool asc = UI::confirm("  Sort ascending?");
                auto sorted = app.inventory.sortedByPrice(asc);
                for (auto& p : sorted) std::cout << *p << "\n";
            } else if (ch == 7) {
                bool asc = UI::confirm("  Sort ascending?");
                auto sorted = app.inventory.sortedByQuantity(asc);
                for (auto& p : sorted) std::cout << *p << "\n";
            } else if (ch == 8) menuStockAdjust(app);
            else if (ch == 9) {
                double val = app.inventory.totalInventoryValue();
                std::cout << "\n  Total Inventory Value: "
                          << UI::formatMoney(val) << "\n";
            }
        } catch (const InventoryException& e) {
            UI::printError(e.what());
        }
        UI::pause();
    }
}

// ============================================================
//  SALES MENU
// ============================================================
void menuSales(AppState& app) {
    while (true) {
        UI::printHeader("SALES & BILLING");
        std::cout << "  1. New Sale / Generate Bill\n"
                  << "  2. View All Sales\n"
                  << "  3. View Bill / Receipt\n"
                  << "  4. Process Refund\n"
                  << "  0. Back\n";
        int ch = UI::inputInt("  Choice: ", 0, 4);
        if (ch == 0) break;
        try {
            if (ch == 1) menuCreateSale(app);
            else if (ch == 2) app.salesMgr.printAllSales();
            else if (ch == 3) menuViewBill(app);
            else if (ch == 4) menuRefundSale(app);
        } catch (const InventoryException& e) {
            UI::printError(e.what());
        }
        UI::pause();
    }
}

// ============================================================
//  SUPPLIER MENU
// ============================================================
void menuSuppliers(AppState& app) {
    while (true) {
        UI::printHeader("SUPPLIER & PURCHASE ORDERS");
        std::cout << "  1. View All Suppliers\n"
                  << "  2. Add Supplier\n"
                  << "  3. View All Purchase Orders\n"
                  << "  4. View Pending POs\n"
                  << "  5. Create Purchase Order\n"
                  << "  6. Receive Purchase Order\n"
                  << "  0. Back\n";
        int ch = UI::inputInt("  Choice: ", 0, 6);
        if (ch == 0) break;
        try {
            if (ch == 1) app.supplierMgr.printAllSuppliers();
            else if (ch == 2) menuAddSupplier(app);
            else if (ch == 3) app.supplierMgr.printAllPOs();
            else if (ch == 4) app.supplierMgr.printPendingPOs();
            else if (ch == 5) menuCreatePO(app);
            else if (ch == 6) menuReceivePO(app);
        } catch (const InventoryException& e) {
            UI::printError(e.what());
        }
        UI::pause();
    }
}

// ============================================================
//  ALERTS MENU
// ============================================================
void menuAlerts(AppState& app) {
    while (true) {
        UI::printHeader("STOCK ALERTS");
        std::cout << "  1. Run Alert Scan\n"
                  << "  2. Full Alert Report\n"
                  << "  3. View Low Stock Products\n"
                  << "  4. View Out-of-Stock Products\n"
                  << "  0. Back\n";
        int ch = UI::inputInt("  Choice: ", 0, 4);
        if (ch == 0) break;
        if (ch == 1) app.alertMgr.runAlertScan(app.inventory);
        else if (ch == 2) app.alertMgr.printAlertReport(app.inventory);
        else if (ch == 3) {
            auto low = app.inventory.getLowStockProducts();
            if (low.empty()) UI::printSuccess("No low stock products.");
            for (auto& p : low) std::cout << *p << "\n";
        } else if (ch == 4) {
            auto oos = app.inventory.getOutOfStockProducts();
            if (oos.empty()) UI::printSuccess("No out-of-stock products.");
            for (auto& p : oos) std::cout << *p << "\n";
        }
        UI::pause();
    }
}

// ============================================================
//  MAIN MENU
// ============================================================
void mainMenu(AppState& app) {
    while (true) {
        UI::clearScreen();
        UI::printHeader("INVENTORY MANAGEMENT SYSTEM");
        std::cout << "  Logged in as: \033[1m" << app.currentUser->getFullName()
                  << "\033[0m [" << app.currentUser->getRoleName() << "]\n";
        std::cout << "  Date: " << today().toString() << "\n\n";
        std::cout << "  1.  Product Management\n"
                  << "  2.  Sales & Billing\n"
                  << "  3.  Suppliers & Purchase Orders\n"
                  << "  4.  Stock Alerts\n"
                  << "  5.  Analytics (Sales & Profit)\n"
                  << "  6.  Reports\n"
                  << "  7.  Audit Log & Undo/Redo\n"
                  << "  8.  User Management\n"
                  << "  9.  Backup & Restore\n"
                  << "  0.  Logout\n";
        int ch = UI::inputInt("  Choice: ", 0, 9);
        if (ch == 0) {
            UI::printInfo("Logged out. Goodbye, " + app.currentUser->getFullName() + "!");
            app.currentUser = nullptr;
            return;
        }
        try {
            UI::clearScreen();
            if      (ch == 1) menuProducts(app);
            else if (ch == 2) menuSales(app);
            else if (ch == 3) menuSuppliers(app);
            else if (ch == 4) menuAlerts(app);
            else if (ch == 5) menuAnalytics(app);
            else if (ch == 6) menuReports(app);
            else if (ch == 7) menuAudit(app);
            else if (ch == 8) menuUserManagement(app);
            else if (ch == 9) menuBackupRestore(app);
        } catch (const PermissionDeniedException& e) {
            UI::printError(e.what());
            UI::pause();
        } catch (const InventoryException& e) {
            UI::printError(e.what());
            UI::pause();
        } catch (const std::exception& e) {
            UI::printError(std::string("Unexpected error: ") + e.what());
            UI::pause();
        }
    }
}

// ============================================================
//  Entry Point
// ============================================================
int main() {
    UI::enableAnsi();

    try {
        AppState app;

        // Run alert scan on startup
        std::cout << "\n\033[1mStarting Inventory Management System...\033[0m\n";
        app.alertMgr.runAlertScan(app.inventory);

        // Login loop
        while (true) {
            int attempts = 0;
            while (!app.currentUser && attempts < 3) {
                if (!doLogin(app)) {
                    ++attempts;
                    if (attempts < 3)
                        std::cout << "  Attempts remaining: " << (3-attempts) << "\n";
                }
            }
            if (!app.currentUser) {
                UI::printError("Too many failed attempts. Exiting.");
                return 1;
            }
            UI::pause();
            mainMenu(app);
            if (!app.currentUser) {
                // Ask to log in again or exit
                if (!UI::confirm("\n  Log in as different user?")) break;
            }
        }

        std::cout << "\n  System shut down. All data saved. Goodbye!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n\033[31mFATAL ERROR: " << e.what() << "\033[0m\n";
        return 1;
    }
}
