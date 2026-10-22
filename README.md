# Inventory Management and Stock Monitoring System
**OOP Individual Project | 204ES | C++17**

---

## Project Overview

A comprehensive console-based **Inventory Management System** built in pure C++ demonstrating all required Object-Oriented Programming concepts for the 204ES course.

---

## How to Compile & Run

### 1. Launching as a Website (Live Web Dashboard & POS)
The system includes a modern interactive web dashboard and Point-of-Sale (POS) interface running directly on top of the C++ data store.

- **Option A**: Double-click [`run_website.bat`](file:///c:/Users/25ec4/OneDrive/Desktop/HACKATHON/InventorySystem/run_website.bat)
- **Option B**: Run via command line:
  ```bash
  python server.py 8080
  ```
- **Access URL**: Open your browser at **[http://localhost:8080/](http://localhost:8080/)**

Both the website and the C++ binary read/write to the exact same CSV files in `data/`, ensuring 100% two-way data synchronisation.

---

### 2. Running as a C++ Console Application
### Requirements
- **g++** (MinGW-w64 on Windows)

### Build
```bash
# Option 1: Double-click build.bat on Windows
build.bat

# Option 2: Manual g++ command
g++ -std=c++14 -Wall -O2 src/main.cpp -o InventorySystem.exe
```

### Run
```bash
InventorySystem.exe
```

### Default Login Credentials
| Username  | Password    | Role    |
|-----------|-------------|---------|
| admin     | admin123    | Admin   |
| manager1  | manager123  | Manager |
| staff1    | staff123    | Staff   |

---

## Features Implemented

### Core Features
| Feature | Status |
|---------|--------|
| Add / Update / Delete products | ✅ |
| Display all products | ✅ |
| Search by ID (binary search) or name | ✅ |
| Track stock after sales & purchases | ✅ |
| Low stock + out-of-stock alerts | ✅ |
| Calculate total inventory value | ✅ |
| Generate itemised sales bills | ✅ |
| File-based persistence (CSV) | ✅ |
| Exception handling | ✅ |
| Inventory & sales reports | ✅ |

### Advanced Features
| Feature | Status |
|---------|--------|
| Smart Stock Alert System (Observer pattern) | ✅ |
| Sales Analytics & Profit (daily/weekly/monthly) | ✅ |
| Automatic Reorder Recommendations | ✅ |
| Batch & Expiry Management (FEFO) | ✅ |
| Inventory Audit & Transaction History | ✅ |
| Advanced Billing & Discount Engine | ✅ |
| Role-Based Login (Admin/Manager/Staff) | ✅ |
| Undo & Redo Operations (Command Pattern) | ✅ |
| Supplier & Purchase Order Management | ✅ |
| Advanced Search & Sorting (binary + STL sort) | ✅ |
| Sales Returns & Refunds | ✅ |
| Data Backup & Recovery | ✅ |
| Demand Forecasting (7-day moving average) | ✅ |
| Automated Report Generator (CSV export) | ✅ |

---

## OOP Concepts Demonstrated

| Concept | Location |
|---------|----------|
| **Abstract Class** | `Product` (pure virtual `calculateTax()`, `getCategory()`) |
| **Inheritance** | `ElectronicProduct`, `GroceryProduct`, `GeneralProduct` ← `Product` |
| **Runtime Polymorphism** | `calculateTax()` called via `Product*` – different tax rates |
| **Encapsulation** | `price`, `quantity` private with validated setters |
| **Constructors** | Parameterised + copy constructors on all model classes |
| **Function Overloading** | `Inventory::search(int)` vs `search(string)` |
| **Operator Overloading** | `operator<<` for `Product`, `operator<`, `operator==` |
| **Templates** | `Repository<T>` generic container |
| **STL Containers** | `vector`, `map`, `unordered_map`, `stack`, `priority_queue` |
| **STL Algorithms** | `sort`, `find_if`, `accumulate`, `copy_if`, `remove_if` |
| **Smart Pointers** | `shared_ptr<Product>` throughout `Inventory` |
| **Exception Handling** | 10+ custom exception classes |
| **File Handling** | CSV read/write, backup/restore via `FileHandler` |
| **Design Patterns** | Strategy (tax), Observer (alerts), Command (undo/redo), Factory (reports) |
| **Enums** | `UserRole`, `POStatus`, `AuditAction`, `DiscountType` |

---

## File Structure

```
InventorySystem/
├── src/
│   ├── main.cpp                    # Entry point + menus
│   ├── models/
│   │   ├── Product.h               # Abstract base class
│   │   ├── ElectronicProduct.h     # 18% GST override
│   │   ├── GroceryProduct.h        # FEFO batch tracking
│   │   ├── GeneralProduct.h        # Clothing / misc
│   │   ├── User.h                  # Role-based access
│   │   ├── Supplier.h              # Supplier + PurchaseOrder
│   │   ├── Sale.h                  # Sale record + billing
│   │   └── AuditEntry.h            # Audit log entry
│   ├── managers/
│   │   ├── Inventory.h             # Template repository
│   │   ├── UserManager.h           # Auth & user CRUD
│   │   ├── SalesManager.h          # Billing, refunds, analytics
│   │   ├── SupplierManager.h       # Supplier & PO lifecycle
│   │   ├── AlertManager.h          # Observer pattern alerts
│   │   ├── AuditManager.h          # Audit + undo/redo
│   │   └── ReportManager.h         # Report factory
│   └── utils/
│       ├── ConsoleUI.h             # ANSI colours, input helpers
│       ├── DateUtils.h             # Date parsing & arithmetic
│       ├── FileHandler.h           # CSV I/O, backup/restore
│       └── Exceptions.h            # Custom exception hierarchy
├── data/                           # Auto-created CSV data files
│   ├── products.csv
│   ├── sales.csv
│   ├── users.csv
│   ├── suppliers.csv
│   ├── purchaseorders.csv
│   ├── audit.csv
│   └── backup/                     # Backup copies
├── build.bat                       # Windows build script
└── README.md
```

---

## Data Files
All data is stored in CSV format under the `data/` directory (auto-created on first run). This demonstrates **file handling** with persistent storage across sessions.

---

## Notes for Examiner
- All code uses **standard C++17** and the **standard library only** — no external dependencies.
- The `data/` folder is created automatically on first run.
- Products include **polymorphic tax calculation**: Electronics = 18% GST, Grocery = 5%, General = 12%.
- The **Observer pattern** fires stock alerts automatically after every sale.
- The **Command pattern** supports undo/redo of stock operations.
- The **Factory pattern** creates different report objects through `ReportManager`.
