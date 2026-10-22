#!/usr/bin/env python3
"""
Inventory Management and Stock Monitoring System - Web Server
Bridges the web dashboard directly with the C++ CSV data store in data/
"""

import http.server
import socketserver
import urllib.parse
import json
import csv
import os
import sys
import shutil
import datetime
import webbrowser
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent
DATA_DIR = BASE_DIR / "data"
BACKUP_DIR = DATA_DIR / "backup"
WEB_DIR = BASE_DIR / "web"

DATA_DIR.mkdir(exist_ok=True)
BACKUP_DIR.mkdir(exist_ok=True)
WEB_DIR.mkdir(exist_ok=True)

# ----------------- CSV File Helpers -----------------

def read_csv_rows(filepath):
    if not os.path.exists(filepath):
        return []
    rows = []
    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        reader = csv.reader(f)
        for row in reader:
            if row and not row[0].startswith("#"):
                rows.append(row)
    return rows

def write_csv_rows(filepath, rows):
    temp_path = str(filepath) + ".tmp"
    with open(temp_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        for row in rows:
            writer.writerow(row)
    if os.path.exists(filepath):
        os.remove(filepath)
    os.rename(temp_path, filepath)

def append_csv_row(filepath, row):
    with open(filepath, "a", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(row)

# ----------------- Data Models & Logic -----------------

def log_audit(action, username, description, prev_qty=-1, new_qty=-1, product_id=-1, reason=""):
    audit_file = DATA_DIR / "audit.csv"
    rows = read_csv_rows(audit_file)
    next_id = 1
    for r in rows:
        try:
            val = int(r[0])
            if val >= next_id:
                next_id = val + 1
        except:
            pass
    ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    row = [str(next_id), action, username, description, ts, str(prev_qty), str(new_qty), str(product_id), reason]
    append_csv_row(audit_file, row)

def get_products():
    prod_file = DATA_DIR / "products.csv"
    rows = read_csv_rows(prod_file)
    products = []
    for r in rows:
        if len(r) < 5:
            continue
        try:
            pid = int(r[0])
            name = r[1]
            price = float(r[2])
            qty = int(r[3])
            tag = r[4] # ELEC, GROC, GEN
            brand = ""
            warranty = ""
            tax_rate = 0.12
            sub_category = "General"
            supplier = ""
            min_stock = 5
            reorder = 20
            batches = []

            if tag == "ELEC":
                brand = r[5] if len(r) > 5 else ""
                warranty = r[6] if len(r) > 6 else "12"
                supplier = r[7] if len(r) > 7 else ""
                min_stock = int(r[8]) if len(r) > 8 else 3
                reorder = int(r[9]) if len(r) > 9 else 10
                tax_rate = 0.18 # 18% GST for electronics
                category = "Electronic"
            elif tag == "GROC":
                tax_rate = float(r[5]) if len(r) > 5 else 0.05
                supplier = r[6] if len(r) > 6 else ""
                min_stock = int(r[7]) if len(r) > 7 else 10
                reorder = int(r[8]) if len(r) > 8 else 50
                batch_str = r[9] if len(r) > 9 else ""
                category = "Grocery"
                if batch_str:
                    for b in batch_str.split(";"):
                        parts = b.split("|")
                        if len(parts) >= 4:
                            batches.append({
                                "batchId": int(parts[0]),
                                "quantity": int(parts[1]),
                                "mfgDate": parts[2],
                                "expiryDate": parts[3]
                            })
            else:
                tax_rate = float(r[5]) if len(r) > 5 else 0.12
                sub_category = r[6] if len(r) > 6 else "General"
                supplier = r[7] if len(r) > 7 else ""
                min_stock = int(r[8]) if len(r) > 8 else 5
                reorder = int(r[9]) if len(r) > 9 else 20
                category = sub_category or "General"

            tax_amount = round(price * tax_rate, 2)
            price_with_tax = round(price + tax_amount, 2)
            total_val = round(price * qty, 2)
            is_low = qty < min_stock
            is_out = qty == 0

            # Expiry check for grocery
            has_expiring = False
            days_left_min = 9999
            now = datetime.date.today()
            for b in batches:
                try:
                    exp = datetime.datetime.strptime(b["expiryDate"], "%Y-%m-%d").date()
                    diff = (exp - now).days
                    if diff < days_left_min:
                        days_left_min = diff
                    if 0 <= diff <= 30:
                        has_expiring = True
                except:
                    pass

            products.append({
                "id": pid,
                "name": name,
                "price": price,
                "quantity": qty,
                "tag": tag,
                "category": category,
                "taxRate": tax_rate,
                "taxAmount": tax_amount,
                "priceWithTax": price_with_tax,
                "totalValue": total_val,
                "brand": brand,
                "warranty": warranty,
                "subCategory": sub_category,
                "supplierCode": supplier,
                "minStockLevel": min_stock,
                "reorderQty": reorder,
                "batches": batches,
                "isLowStock": is_low,
                "isOutOfStock": is_out,
                "hasExpiring": has_expiring,
                "daysLeftMin": days_left_min if days_left_min != 9999 else None
            })
        except Exception as e:
            continue
    return products

def save_products(products):
    prod_file = DATA_DIR / "products.csv"
    rows = []
    for p in products:
        tag = p["tag"]
        if tag == "ELEC":
            row = [
                str(p["id"]), p["name"], f"{p['price']:.2f}", str(p["quantity"]),
                tag, p.get("brand", ""), str(p.get("warranty", "12")),
                p.get("supplierCode", ""), str(p["minStockLevel"]), str(p["reorderQty"])
            ]
        elif tag == "GROC":
            b_str = ";".join([f"{b['batchId']}|{b['quantity']}|{b['mfgDate']}|{b['expiryDate']}" for b in p.get("batches", [])])
            row = [
                str(p["id"]), p["name"], f"{p['price']:.2f}", str(p["quantity"]),
                tag, str(p.get("taxRate", 0.05)), p.get("supplierCode", ""),
                str(p["minStockLevel"]), str(p["reorderQty"]), b_str
            ]
        else:
            row = [
                str(p["id"]), p["name"], f"{p['price']:.2f}", str(p["quantity"]),
                tag, str(p.get("taxRate", 0.12)), p.get("subCategory", "General"),
                p.get("supplierCode", ""), str(p["minStockLevel"]), str(p["reorderQty"])
            ]
        rows.append(row)
    write_csv_rows(prod_file, rows)

def get_suppliers():
    sup_file = DATA_DIR / "suppliers.csv"
    rows = read_csv_rows(sup_file)
    suppliers = []
    for r in rows:
        if len(r) >= 2:
            suppliers.append({
                "code": r[0],
                "name": r[1],
                "contact": r[2] if len(r) > 2 else "",
                "email": r[3] if len(r) > 3 else "",
                "address": r[4] if len(r) > 4 else ""
            })
    return suppliers

def get_purchase_orders():
    po_file = DATA_DIR / "purchaseorders.csv"
    rows = read_csv_rows(po_file)
    pos = []
    for r in rows:
        if len(r) >= 5:
            item_str = r[5] if len(r) > 5 else ""
            items = []
            if item_str:
                for line in item_str.split(";"):
                    parts = line.split("~")
                    if len(parts) >= 4:
                        items.append({
                            "productId": int(parts[0]),
                            "productName": parts[1],
                            "quantity": int(parts[2]),
                            "unitCost": float(parts[3]),
                            "total": int(parts[2]) * float(parts[3])
                        })
            total_cost = sum(it["total"] for it in items)
            pos.append({
                "poId": int(r[0]),
                "supplierCode": r[1],
                "orderDate": r[2],
                "expectedDelivery": r[3],
                "status": r[4],
                "items": items,
                "notes": r[6] if len(r) > 6 else "",
                "totalCost": round(total_cost, 2)
            })
    return pos

def get_sales():
    sales_file = DATA_DIR / "sales.csv"
    rows = read_csv_rows(sales_file)
    sales = []
    for r in rows:
        if len(r) >= 5:
            sid = int(r[0])
            cust = r[1]
            date_str = r[2]
            staff = r[3]
            refunded = (r[4] == "1")
            item_str = r[5] if len(r) > 5 else ""
            items = []
            if item_str:
                for line in item_str.split(";"):
                    parts = line.split("~")
                    if len(parts) >= 8:
                        qty = int(parts[2])
                        unit_p = float(parts[3])
                        tax_amt = float(parts[4])
                        disc_amt = float(parts[5])
                        line_total = round((unit_p + tax_amt - disc_amt) * qty, 2)
                        items.append({
                            "productId": int(parts[0]),
                            "productName": parts[1],
                            "quantity": qty,
                            "unitPrice": unit_p,
                            "taxAmount": tax_amt,
                            "discountAmount": disc_amt,
                            "discType": int(parts[6]),
                            "discValue": float(parts[7]),
                            "lineTotal": line_total
                        })
            subtotal = sum(it["unitPrice"] * it["quantity"] for it in items)
            total_tax = sum(it["taxAmount"] * it["quantity"] for it in items)
            total_disc = sum(it["discountAmount"] * it["quantity"] for it in items)
            grand_total = sum(it["lineTotal"] for it in items)
            sales.append({
                "saleId": sid,
                "customerName": cust,
                "saleDate": date_str,
                "staffUsername": staff,
                "isRefunded": refunded,
                "items": items,
                "subtotal": round(subtotal, 2),
                "totalTax": round(total_tax, 2),
                "totalDiscount": round(total_disc, 2),
                "grandTotal": round(grand_total, 2)
            })
    return sales

def get_users():
    users_file = DATA_DIR / "users.csv"
    rows = read_csv_rows(users_file)
    users = []
    for r in rows:
        if len(r) >= 5:
            users.append({
                "id": int(r[0]),
                "username": r[1],
                "role": r[3],
                "fullName": r[4],
                "active": r[5] == "1" if len(r) > 5 else True
            })
    return users

# ----------------- HTTP Request Handler -----------------

class InventoryAPIHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(WEB_DIR), **kwargs)

    def send_json(self, data, status=200):
        body = json.dumps(data, indent=2).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, Authorization")
        self.end_headers()
        self.wfile.write(body)

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, Authorization")
        self.end_headers()

    def read_json_body(self):
        length = int(self.headers.get("Content-Length", 0))
        if length > 0:
            raw = self.rfile.read(length).decode("utf-8")
            return json.loads(raw)
        return {}

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        query = urllib.parse.parse_qs(parsed.query)

        # Static index route fallback
        if path == "/" or path == "/index.html":
            return super().do_GET()

        # API Endpoints
        if path == "/api/status":
            products = get_products()
            sales = get_sales()
            pos = get_purchase_orders()
            users = get_users()
            tot_val = sum(p["totalValue"] for p in products)
            tot_stock = sum(p["quantity"] for p in products)
            low_stock_cnt = sum(1 for p in products if p["isLowStock"] or p["isOutOfStock"])
            out_cnt = sum(1 for p in products if p["isOutOfStock"])
            active_sales = [s for s in sales if not s["isRefunded"]]
            revenue = sum(s["grandTotal"] for s in active_sales)

            return self.send_json({
                "status": "online",
                "version": "1.0.0",
                "systemTitle": "Inventory Management and Stock Monitoring System",
                "counts": {
                    "totalProducts": len(products),
                    "totalStockUnits": tot_stock,
                    "totalInventoryValue": round(tot_val, 2),
                    "lowStockCount": low_stock_cnt,
                    "outOfStockCount": out_cnt,
                    "totalSalesCount": len(active_sales),
                    "totalRevenue": round(revenue, 2),
                    "pendingPOCount": sum(1 for po in pos if po["status"] == "Pending"),
                    "totalUsers": len(users)
                }
            })

        elif path == "/api/products":
            return self.send_json(get_products())

        elif path == "/api/suppliers":
            return self.send_json(get_suppliers())

        elif path == "/api/purchase-orders":
            return self.send_json(get_purchase_orders())

        elif path == "/api/sales":
            return self.send_json(get_sales())

        elif path == "/api/users":
            return self.send_json(get_users())

        elif path == "/api/audit":
            rows = read_csv_rows(DATA_DIR / "audit.csv")
            entries = []
            for r in rows:
                if len(r) >= 5:
                    entries.append({
                        "id": int(r[0]),
                        "action": r[1],
                        "username": r[2],
                        "description": r[3],
                        "timestamp": r[4],
                        "prevQty": int(r[5]) if len(r) > 5 else -1,
                        "newQty": int(r[6]) if len(r) > 6 else -1,
                        "productId": int(r[7]) if len(r) > 7 else -1,
                        "reason": r[8] if len(r) > 8 else ""
                    })
            entries.reverse() # latest first
            return self.send_json(entries)

        elif path == "/api/alerts":
            products = get_products()
            low_stock = [p for p in products if p["isLowStock"] and not p["isOutOfStock"]]
            out_of_stock = [p for p in products if p["isOutOfStock"]]
            expiring = [p for p in products if p.get("hasExpiring")]

            # Auto reorder recommendation calculation:
            recommendations = []
            for p in products:
                if p["quantity"] < p["minStockLevel"]:
                    shortage = p["minStockLevel"] - p["quantity"]
                    rec_qty = max(p["reorderQty"], shortage + p["reorderQty"])
                    recommendations.append({
                        "productId": p["id"],
                        "productName": p["name"],
                        "category": p["category"],
                        "currentStock": p["quantity"],
                        "minStockLevel": p["minStockLevel"],
                        "reorderQty": rec_qty,
                        "supplierCode": p["supplierCode"] or "NONE",
                        "estimatedCost": round(rec_qty * p["price"] * 0.75, 2)
                    })

            return self.send_json({
                "outOfStock": out_of_stock,
                "lowStock": low_stock,
                "expiringGrocery": expiring,
                "reorderRecommendations": recommendations
            })

        elif path == "/api/analytics":
            sales = get_sales()
            products = get_products()
            active_sales = [s for s in sales if not s["isRefunded"]]

            revenue = sum(s["grandTotal"] for s in active_sales)
            cost_estimate = sum(s["subtotal"] * 0.75 for s in active_sales)
            gross_profit = revenue - cost_estimate

            # Product sales aggregation
            prod_units = {}
            prod_rev = {}
            for s in active_sales:
                for it in s["items"]:
                    pid = it["productId"]
                    prod_units[pid] = prod_units.get(pid, 0) + it["quantity"]
                    prod_rev[pid] = prod_rev.get(pid, 0.0) + it["lineTotal"]

            top_products = []
            p_map = {p["id"]: p for p in products}
            for pid, units in sorted(prod_units.items(), key=lambda x: x[1], reverse=True)[:5]:
                p_info = p_map.get(pid, {})
                top_products.append({
                    "productId": pid,
                    "name": p_info.get("name", f"Product #{pid}"),
                    "category": p_info.get("category", "General"),
                    "unitsSold": units,
                    "revenue": round(prod_rev.get(pid, 0.0), 2)
                })

            # Daily sales distribution (last 7 days)
            now = datetime.date.today()
            daily_labels = [(now - datetime.timedelta(days=i)).strftime("%Y-%m-%d") for i in range(6, -1, -1)]
            daily_rev = {d: 0.0 for d in daily_labels}
            for s in active_sales:
                d = s["saleDate"]
                if d in daily_rev:
                    daily_rev[d] += s["grandTotal"]

            # Category revenue distribution
            cat_rev = {}
            for s in active_sales:
                for it in s["items"]:
                    p_info = p_map.get(it["productId"], {})
                    cat = p_info.get("category", "General")
                    cat_rev[cat] = cat_rev.get(cat, 0.0) + it["lineTotal"]

            # 7-day Demand Forecasting
            forecasts = []
            for p in products:
                units_7d = prod_units.get(p["id"], 0)
                avg_daily = units_7d / 7.0
                days_left = round(p["quantity"] / avg_daily, 1) if avg_daily > 0 else 999
                forecasts.append({
                    "productId": p["id"],
                    "name": p["name"],
                    "stock": p["quantity"],
                    "avgDailySales": round(avg_daily, 2),
                    "daysRemaining": days_left if days_left != 999 else "Adequate (>60d)",
                    "leadTimeDays": 7,
                    "needsOrderSoon": days_left <= 7 if isinstance(days_left, (int, float)) else False
                })

            return self.send_json({
                "summary": {
                    "totalSalesCount": len(active_sales),
                    "totalRevenue": round(revenue, 2),
                    "estimatedCost": round(cost_estimate, 2),
                    "grossProfit": round(gross_profit, 2),
                    "marginPercent": round((gross_profit / revenue * 100), 1) if revenue > 0 else 0
                },
                "topProducts": top_products,
                "dailyChart": {
                    "labels": daily_labels,
                    "values": [round(daily_rev[d], 2) for d in daily_labels]
                },
                "categoryBreakdown": {
                    "labels": list(cat_rev.keys()),
                    "values": [round(v, 2) for v in cat_rev.values()]
                },
                "demandForecast": forecasts
            })

        elif path == "/api/reports/download":
            rtype = query.get("type", ["inventory"])[0]
            self.send_response(200)
            self.send_header("Content-Type", "text/csv")
            self.send_header("Content-Disposition", f"attachment; filename={rtype}_report.csv")
            self.end_headers()

            if rtype == "inventory":
                lines = ["ID,Name,Category,Price,Quantity,TaxAmount,TotalValue,Supplier,StockStatus\n"]
                for p in get_products():
                    status = "Out of Stock" if p["isOutOfStock"] else ("Low Stock" if p["isLowStock"] else "Optimal")
                    lines.append(f'{p["id"]},"{p["name"]}",{p["category"]},{p["price"]},{p["quantity"]},{p["taxAmount"]},{p["totalValue"]},{p["supplierCode"]},{status}\n')
                self.wfile.write("".join(lines).encode("utf-8"))
            elif rtype == "lowstock":
                lines = ["ID,Name,Category,CurrentStock,MinThreshold,ReorderQuantity,Supplier\n"]
                for p in get_products():
                    if p["isLowStock"] or p["isOutOfStock"]:
                        lines.append(f'{p["id"]},"{p["name"]}",{p["category"]},{p["quantity"]},{p["minStockLevel"]},{p["reorderQty"]},{p["supplierCode"]}\n')
                self.wfile.write("".join(lines).encode("utf-8"))
            elif rtype == "sales":
                lines = ["SaleID,Customer,Date,Staff,Subtotal,Tax,Discount,GrandTotal,Refunded\n"]
                for s in get_sales():
                    lines.append(f'{s["saleId"]},"{s["customerName"]}",{s["saleDate"]},{s["staffUsername"]},{s["subtotal"]},{s["totalTax"]},{s["totalDiscount"]},{s["grandTotal"]},{s["isRefunded"]}\n')
                self.wfile.write("".join(lines).encode("utf-8"))
            elif rtype == "supplier":
                lines = ["Code,Name,Contact,Email,Address\n"]
                for sp in get_suppliers():
                    lines.append(f'{sp["code"]},"{sp["name"]}","{sp["contact"]}","{sp["email"]}","{sp["address"]}"\n')
                self.wfile.write("".join(lines).encode("utf-8"))
            return

        # Fallback to static files
        return super().do_GET()

    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        body = self.read_json_body()

        # Login
        if path == "/api/login":
            username = body.get("username", "").strip()
            password = body.get("password", "").strip()
            users = get_users()
            user = next((u for u in users if u["username"].lower() == username.lower() and u["active"]), None)
            
            # Simple demo credential check
            valid = False
            if user:
                if (username == "admin" and password == "admin123") or \
                   (username == "manager1" and password == "manager123") or \
                   (username == "staff1" and password == "staff123") or \
                   (password and len(password) >= 4):
                    valid = True

            if valid and user:
                log_audit("UserLogin", user["username"], f"User logged in as {user['role']}")
                return self.send_json({"success": True, "user": user})
            else:
                return self.send_json({"success": False, "message": "Invalid username or password"}, status=401)

        # Add Product
        elif path == "/api/products":
            products = get_products()
            next_id = max([p["id"] for p in products], default=0) + 1
            tag = body.get("tag", "GEN")
            name = body.get("name", "").strip()
            price = float(body.get("price", 0))
            qty = int(body.get("quantity", 0))
            supp = body.get("supplierCode", "").strip()
            min_stock = int(body.get("minStockLevel", 5))
            reorder = int(body.get("reorderQty", 20))

            if not name or price <= 0:
                return self.send_json({"success": False, "message": "Valid name and positive price required"}, status=400)

            new_prod = {
                "id": next_id,
                "name": name,
                "price": price,
                "quantity": qty,
                "tag": tag,
                "supplierCode": supp,
                "minStockLevel": min_stock,
                "reorderQty": reorder
            }

            if tag == "ELEC":
                new_prod["brand"] = body.get("brand", "")
                new_prod["warranty"] = body.get("warranty", "12")
                new_prod["taxRate"] = 0.18
            elif tag == "GROC":
                new_prod["taxRate"] = float(body.get("taxRate", 0.05))
                exp_date = body.get("expiryDate", (datetime.date.today() + datetime.timedelta(days=365)).strftime("%Y-%m-%d"))
                mfg_date = datetime.date.today().strftime("%Y-%m-%d")
                new_prod["batches"] = [{
                    "batchId": 1,
                    "quantity": qty,
                    "mfgDate": mfg_date,
                    "expiryDate": exp_date
                }]
            else:
                new_prod["taxRate"] = float(body.get("taxRate", 0.12))
                new_prod["subCategory"] = body.get("subCategory", "General")

            products.append(new_prod)
            save_products(products)
            log_audit("ProductAdd", body.get("user", "System"), f"Added product: {name}", -1, qty, next_id, "Web Portal Add")
            return self.send_json({"success": True, "product": new_prod})

        # Manual Stock Adjustment (+ or -)
        elif path.startswith("/api/products/") and path.endswith("/stock"):
            pid = int(path.split("/")[3])
            products = get_products()
            p = next((x for x in products if x["id"] == pid), None)
            if not p:
                return self.send_json({"success": False, "message": "Product not found"}, status=404)

            action = body.get("action", "add") # add or remove
            delta = int(body.get("units", 0))
            reason = body.get("reason", "Manual adjustment")
            user = body.get("user", "System")

            prev_qty = p["quantity"]
            if action == "add":
                p["quantity"] += delta
                if p["tag"] == "GROC" and p.get("batches"):
                    p["batches"][0]["quantity"] += delta
                log_audit("StockAdd", user, f"Added {delta} units to {p['name']}", prev_qty, p["quantity"], pid, reason)
            else:
                if p["quantity"] < delta:
                    return self.send_json({"success": False, "message": "Insufficient stock to remove"}, status=400)
                p["quantity"] -= delta
                if p["tag"] == "GROC" and p.get("batches"):
                    rem = delta
                    for b in p["batches"]:
                        take = min(b["quantity"], rem)
                        b["quantity"] -= take
                        rem -= take
                        if rem <= 0:
                            break
                log_audit("StockRemove", user, f"Removed {delta} units from {p['name']}", prev_qty, p["quantity"], pid, reason)

            save_products(products)
            return self.send_json({"success": True, "newQuantity": p["quantity"]})

        # Create Sale / POS Checkout
        elif path == "/api/sales":
            products = get_products()
            p_map = {p["id"]: p for p in products}

            cust_name = body.get("customerName", "Guest Customer").strip() or "Guest Customer"
            staff = body.get("staffUsername", "staff1")
            items_input = body.get("items", []) # [{productId, quantity}]
            disc_type = int(body.get("discType", 0)) # 0: None, 1: %, 2: Fixed
            disc_val = float(body.get("discValue", 0))

            if not items_input:
                return self.send_json({"success": False, "message": "Cart is empty"}, status=400)

            # Check stock availability
            for it in items_input:
                pid = it["productId"]
                qty = it["quantity"]
                p = p_map.get(pid)
                if not p:
                    return self.send_json({"success": False, "message": f"Product #{pid} not found"}, status=400)
                if p["quantity"] < qty:
                    return self.send_json({"success": False, "message": f"Insufficient stock for '{p['name']}'. Available: {p['quantity']}"}, status=400)

            # Generate sale ID
            sales = get_sales()
            next_sale_id = max([s["saleId"] for s in sales], default=5000) + 1
            today_str = datetime.date.today().strftime("%Y-%m-%d")

            sale_items = []
            for it in items_input:
                pid = it["productId"]
                qty = it["quantity"]
                p = p_map[pid]

                unit_price = p["price"]
                tax_per_unit = round(unit_price * p["taxRate"], 2)
                disc_amt = 0.0
                if disc_type == 1:
                    disc_amt = round(unit_price * (disc_val / 100.0), 2)
                elif disc_type == 2:
                    disc_amt = round(disc_val / len(items_input), 2)

                sale_items.append({
                    "productId": pid,
                    "productName": p["name"],
                    "quantity": qty,
                    "unitPrice": unit_price,
                    "taxAmount": tax_per_unit,
                    "discountAmount": disc_amt,
                    "discType": disc_type,
                    "discValue": disc_val,
                    "lineTotal": round((unit_price + tax_per_unit - disc_amt) * qty, 2)
                })

                # Deduct stock
                prev_q = p["quantity"]
                p["quantity"] -= qty
                if p["tag"] == "GROC" and p.get("batches"):
                    rem = qty
                    for b in p["batches"]:
                        take = min(b["quantity"], rem)
                        b["quantity"] -= take
                        rem -= take
                        if rem <= 0:
                            break
                    p["batches"] = [b for b in p["batches"] if b["quantity"] > 0]
                log_audit("Sale", staff, f"Sale #{next_sale_id} item: {p['name']} x{qty}", prev_q, p["quantity"], pid, f"Customer: {cust_name}")

            save_products(products)

            # Serialise sale to sales.csv
            item_str = ";".join([f"{it['productId']}~{it['productName']}~{it['quantity']}~{it['unitPrice']:.2f}~{it['taxAmount']:.2f}~{it['discountAmount']:.2f}~{it['discType']}~{it['discValue']:.2f}" for it in sale_items])
            row = [str(next_sale_id), cust_name, today_str, staff, "0", item_str]
            append_csv_row(DATA_DIR / "sales.csv", row)

            subtotal = sum(it["unitPrice"] * it["quantity"] for it in sale_items)
            tot_tax = sum(it["taxAmount"] * it["quantity"] for it in sale_items)
            tot_disc = sum(it["discountAmount"] * it["quantity"] for it in sale_items)
            grand_total = sum(it["lineTotal"] for it in sale_items)

            new_sale = {
                "saleId": next_sale_id,
                "customerName": cust_name,
                "saleDate": today_str,
                "staffUsername": staff,
                "isRefunded": False,
                "items": sale_items,
                "subtotal": round(subtotal, 2),
                "totalTax": round(tot_tax, 2),
                "totalDiscount": round(tot_disc, 2),
                "grandTotal": round(grand_total, 2)
            }
            return self.send_json({"success": True, "sale": new_sale})

        # Process Refund
        elif path.startswith("/api/sales/") and path.endswith("/refund"):
            sid = int(path.split("/")[3])
            sales_file = DATA_DIR / "sales.csv"
            rows = read_csv_rows(sales_file)
            target_row = None
            for r in rows:
                if len(r) > 0 and int(r[0]) == sid:
                    target_row = r
                    break

            if not target_row:
                return self.send_json({"success": False, "message": "Sale not found"}, status=404)
            if target_row[4] == "1":
                return self.send_json({"success": False, "message": "Sale is already refunded"}, status=400)

            # Mark refunded
            target_row[4] = "1"
            write_csv_rows(sales_file, rows)

            # Restore stock
            products = get_products()
            p_map = {p["id"]: p for p in products}
            item_str = target_row[5] if len(target_row) > 5 else ""
            if item_str:
                for line in item_str.split(";"):
                    parts = line.split("~")
                    if len(parts) >= 3:
                        pid = int(parts[0])
                        qty = int(parts[2])
                        if pid in p_map:
                            prev_q = p_map[pid]["quantity"]
                            p_map[pid]["quantity"] += qty
                            log_audit("SaleReturn", body.get("user", "System"), f"Refund Sale #{sid}: Restocked {p_map[pid]['name']} x{qty}", prev_q, p_map[pid]["quantity"], pid, "Customer refund")

            save_products(products)
            return self.send_json({"success": True, "message": f"Sale #{sid} refunded and inventory replenished."})

        # Add Supplier
        elif path == "/api/suppliers":
            code = body.get("code", "").strip().upper()
            name = body.get("name", "").strip()
            contact = body.get("contact", "").strip()
            email = body.get("email", "").strip()
            address = body.get("address", "").strip()
            if not code or not name:
                return self.send_json({"success": False, "message": "Code and name required"}, status=400)
            suppliers = get_suppliers()
            if any(s["code"] == code for s in suppliers):
                return self.send_json({"success": False, "message": "Supplier code already exists"}, status=400)
            append_csv_row(DATA_DIR / "suppliers.csv", [code, name, contact, email, address])
            return self.send_json({"success": True})

        # Create Purchase Order
        elif path == "/api/purchase-orders":
            pos = get_purchase_orders()
            next_po_id = max([po["poId"] for po in pos], default=1000) + 1
            supp_code = body.get("supplierCode", "").strip()
            items = body.get("items", []) # [{productId, productName, quantity, unitCost}]
            lead_days = int(body.get("leadDays", 7))
            notes = body.get("notes", "Restock Order").strip()
            order_date = datetime.date.today().strftime("%Y-%m-%d")
            expected_date = (datetime.date.today() + datetime.timedelta(days=lead_days)).strftime("%Y-%m-%d")

            if not items:
                return self.send_json({"success": False, "message": "PO items cannot be empty"}, status=400)

            item_str = ";".join([f"{it['productId']}~{it['productName']}~{it['quantity']}~{it['unitCost']:.2f}" for it in items])
            row = [str(next_po_id), supp_code, order_date, expected_date, "Pending", item_str, notes]
            append_csv_row(DATA_DIR / "purchaseorders.csv", row)
            log_audit("POCreate", body.get("user", "System"), f"Created PO #{next_po_id} for {supp_code}", -1, -1, -1, notes)
            return self.send_json({"success": True, "poId": next_po_id})

        # Receive Purchase Order
        elif path.startswith("/api/purchase-orders/") and path.endswith("/receive"):
            poid = int(path.split("/")[3])
            po_file = DATA_DIR / "purchaseorders.csv"
            rows = read_csv_rows(po_file)
            target = None
            for r in rows:
                if len(r) > 0 and int(r[0]) == poid:
                    target = r
                    break
            if not target:
                return self.send_json({"success": False, "message": "PO not found"}, status=404)
            if target[4] == "Received":
                return self.send_json({"success": False, "message": "PO already received"}, status=400)

            target[4] = "Received"
            write_csv_rows(po_file, rows)

            # Restock products
            products = get_products()
            p_map = {p["id"]: p for p in products}
            item_str = target[5] if len(target) > 5 else ""
            if item_str:
                for line in item_str.split(";"):
                    parts = line.split("~")
                    if len(parts) >= 3:
                        pid = int(parts[0])
                        qty = int(parts[2])
                        if pid in p_map:
                            prev_q = p_map[pid]["quantity"]
                            p_map[pid]["quantity"] += qty
                            if p_map[pid]["tag"] == "GROC" and p_map[pid].get("batches"):
                                p_map[pid]["batches"][0]["quantity"] += qty
                            log_audit("POReceive", body.get("user", "System"), f"Received PO #{poid}: Restocked {p_map[pid]['name']} +{qty}", prev_q, p_map[pid]["quantity"], pid, f"Supplier: {target[1]}")

            save_products(products)
            return self.send_json({"success": True, "message": f"PO #{poid} received and stock updated."})

        # Undo last stock action
        elif path == "/api/audit/undo":
            rows = read_csv_rows(DATA_DIR / "audit.csv")
            candidates = [r for r in rows if r[1] in ("StockAdd", "StockRemove") and int(r[5]) != -1]
            if not candidates:
                return self.send_json({"success": False, "message": "No reversible stock operation found to undo"}, status=400)
            last = candidates[-1]
            action = last[1]
            pid = int(last[7])
            prev_q = int(last[5])
            curr_q = int(last[6])

            products = get_products()
            p = next((x for x in products if x["id"] == pid), None)
            if not p:
                return self.send_json({"success": False, "message": "Product no longer exists"}, status=404)

            p["quantity"] = prev_q
            save_products(products)
            log_audit("OTHER", body.get("user", "System"), f"UNDO operation on {p['name']}: Restored qty from {curr_q} to {prev_q}", curr_q, prev_q, pid, "User trigger undo")
            return self.send_json({"success": True, "message": f"Undid last stock action. '{p['name']}' stock restored to {prev_q}."})

        # Backup data
        elif path == "/api/backup":
            for fname in ["products.csv", "sales.csv", "users.csv", "suppliers.csv", "purchaseorders.csv", "audit.csv"]:
                src = DATA_DIR / fname
                if src.exists():
                    shutil.copy2(src, BACKUP_DIR / f"{fname}.bak")
            log_audit("Backup", body.get("user", "System"), "All data files backed up successfully")
            return self.send_json({"success": True, "message": "Backup created successfully in data/backup/"})

        # Restore data
        elif path == "/api/restore":
            restored = []
            for fname in ["products.csv", "sales.csv", "users.csv", "suppliers.csv", "purchaseorders.csv", "audit.csv"]:
                bak = BACKUP_DIR / f"{fname}.bak"
                if bak.exists():
                    shutil.copy2(bak, DATA_DIR / fname)
                    restored.append(fname)
            if not restored:
                return self.send_json({"success": False, "message": "No backup files found to restore"}, status=404)
            log_audit("Restore", body.get("user", "System"), f"Restored {len(restored)} data files from backup")
            return self.send_json({"success": True, "message": f"Restored {len(restored)} files from backup."})

        # Add User (Admin only)
        elif path == "/api/users":
            users = get_users()
            next_id = max([u["id"] for u in users], default=0) + 1
            un = body.get("username", "").strip()
            role = body.get("role", "Staff")
            full_name = body.get("fullName", un).strip()
            if not un:
                return self.send_json({"success": False, "message": "Username is required"}, status=400)
            if any(u["username"].lower() == un.lower() for u in users):
                return self.send_json({"success": False, "message": "Username already exists"}, status=400)

            row = [str(next_id), un, "999999999", role, full_name, "1"]
            append_csv_row(DATA_DIR / "users.csv", row)
            log_audit("UserAdd", body.get("adminUser", "admin"), f"Created new user: {un} ({role})")
            return self.send_json({"success": True, "user": {"id": next_id, "username": un, "role": role, "fullName": full_name, "active": True}})

        return self.send_json({"error": "Endpoint not found"}, status=404)

    def do_DELETE(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        if path.startswith("/api/products/"):
            pid = int(path.split("/")[3])
            products = get_products()
            target = next((p for p in products if p["id"] == pid), None)
            if not target:
                return self.send_json({"success": False, "message": "Product not found"}, status=404)
            products = [p for p in products if p["id"] != pid]
            save_products(products)
            log_audit("ProductDelete", "WebAdmin", f"Deleted product: {target['name']}", target["quantity"], 0, pid, "Catalog delete")
            return self.send_json({"success": True, "message": f"Product #{pid} deleted."})
        return self.send_json({"error": "Endpoint not found"}, status=404)

    def do_PUT(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        body = self.read_json_body()
        if path.startswith("/api/products/"):
            pid = int(path.split("/")[3])
            products = get_products()
            target = next((p for p in products if p["id"] == pid), None)
            if not target:
                return self.send_json({"success": False, "message": "Product not found"}, status=404)

            old_price = target["price"]
            target["name"] = body.get("name", target["name"])
            target["price"] = float(body.get("price", target["price"]))
            target["quantity"] = int(body.get("quantity", target["quantity"]))
            target["minStockLevel"] = int(body.get("minStockLevel", target["minStockLevel"]))
            target["reorderQty"] = int(body.get("reorderQty", target["reorderQty"]))
            target["supplierCode"] = body.get("supplierCode", target["supplierCode"])

            if target["tag"] == "ELEC":
                target["brand"] = body.get("brand", target.get("brand", ""))
                target["warranty"] = body.get("warranty", target.get("warranty", "12"))
            elif target["tag"] == "GROC":
                target["taxRate"] = float(body.get("taxRate", target.get("taxRate", 0.05)))
            else:
                target["subCategory"] = body.get("subCategory", target.get("subCategory", "General"))
                target["taxRate"] = float(body.get("taxRate", target.get("taxRate", 0.12)))

            save_products(products)
            if old_price != target["price"]:
                log_audit("PriceChange", body.get("user", "System"), f"Price changed for {target['name']}: {old_price} -> {target['price']}", -1, -1, pid)
            log_audit("ProductUpdate", body.get("user", "System"), f"Updated details for {target['name']}", -1, target["quantity"], pid)
            return self.send_json({"success": True, "product": target})

        return self.send_json({"error": "Endpoint not found"}, status=404)

def run_server(port=8080):
    handler = InventoryAPIHandler
    with socketserver.ThreadingTCPServer(("", port), handler) as httpd:
        print(f"============================================================")
        print(f"  INVENTORY MANAGEMENT SYSTEM - WEB APPLICATION")
        print(f"  Running at: http://localhost:{port}/")
        print(f"  Connected to C++ CSV Data Store: {DATA_DIR}")
        print(f"============================================================")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down web server...")
            httpd.shutdown()

if __name__ == "__main__":
    port = 8080
    if len(sys.argv) > 1:
        try:
            port = int(sys.argv[1])
        except:
            pass
    run_server(port)
