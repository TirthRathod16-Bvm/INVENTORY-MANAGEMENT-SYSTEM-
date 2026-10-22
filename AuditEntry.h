// ============================================================
//  AuditEntry.h  –  Immutable audit log entry
// ============================================================
#pragma once
#include <string>
#include <vector>
#include "../utils/DateUtils.h"

enum class AuditAction {
    PRODUCT_ADD, PRODUCT_UPDATE, PRODUCT_DELETE,
    STOCK_ADD, STOCK_REMOVE,
    SALE, SALE_RETURN,
    PO_CREATE, PO_RECEIVE, PO_CANCEL,
    USER_LOGIN, USER_ADD, USER_UPDATE,
    BACKUP, RESTORE,
    PRICE_CHANGE, OTHER
};

inline std::string auditActionToStr(AuditAction a) {
    switch(a) {
        case AuditAction::PRODUCT_ADD:    return "ProductAdd";
        case AuditAction::PRODUCT_UPDATE: return "ProductUpdate";
        case AuditAction::PRODUCT_DELETE: return "ProductDelete";
        case AuditAction::STOCK_ADD:      return "StockAdd";
        case AuditAction::STOCK_REMOVE:   return "StockRemove";
        case AuditAction::SALE:           return "Sale";
        case AuditAction::SALE_RETURN:    return "SaleReturn";
        case AuditAction::PO_CREATE:      return "POCreate";
        case AuditAction::PO_RECEIVE:     return "POReceive";
        case AuditAction::PO_CANCEL:      return "POCancel";
        case AuditAction::USER_LOGIN:     return "UserLogin";
        case AuditAction::USER_ADD:       return "UserAdd";
        case AuditAction::USER_UPDATE:    return "UserUpdate";
        case AuditAction::BACKUP:         return "Backup";
        case AuditAction::RESTORE:        return "Restore";
        case AuditAction::PRICE_CHANGE:   return "PriceChange";
        default:                          return "Other";
    }
}

inline AuditAction strToAuditAction(const std::string& s) {
    if (s=="ProductAdd")    return AuditAction::PRODUCT_ADD;
    if (s=="ProductUpdate") return AuditAction::PRODUCT_UPDATE;
    if (s=="ProductDelete") return AuditAction::PRODUCT_DELETE;
    if (s=="StockAdd")      return AuditAction::STOCK_ADD;
    if (s=="StockRemove")   return AuditAction::STOCK_REMOVE;
    if (s=="Sale")          return AuditAction::SALE;
    if (s=="SaleReturn")    return AuditAction::SALE_RETURN;
    if (s=="POCreate")      return AuditAction::PO_CREATE;
    if (s=="POReceive")     return AuditAction::PO_RECEIVE;
    if (s=="POCancel")      return AuditAction::PO_CANCEL;
    if (s=="UserLogin")     return AuditAction::USER_LOGIN;
    if (s=="UserAdd")       return AuditAction::USER_ADD;
    if (s=="UserUpdate")    return AuditAction::USER_UPDATE;
    if (s=="Backup")        return AuditAction::BACKUP;
    if (s=="Restore")       return AuditAction::RESTORE;
    if (s=="PriceChange")   return AuditAction::PRICE_CHANGE;
    return AuditAction::OTHER;
}

struct AuditEntry {
    int         entryId;
    AuditAction action;
    std::string username;
    std::string description;
    std::string timestamp;
    int         prevQty;
    int         newQty;
    int         productId;
    std::string reason;

    AuditEntry(int id, AuditAction act, const std::string& user,
               const std::string& desc, int pid=0,
               int pq=-1, int nq=-1, const std::string& reason="")
        : entryId(id), action(act), username(user), description(desc),
          timestamp(nowTimestamp()), prevQty(pq), newQty(nq),
          productId(pid), reason(reason) {}

    std::vector<std::string> toCSVRow() const {
        return {
            std::to_string(entryId), auditActionToStr(action),
            username, description, timestamp,
            std::to_string(productId),
            std::to_string(prevQty), std::to_string(newQty),
            reason
        };
    }

    void print() const {
        std::cout << "  [" << timestamp << "] "
                  << auditActionToStr(action)
                  << " | User: " << username
                  << " | " << description;
        if (prevQty >= 0)
            std::cout << " | Qty: " << prevQty << " → " << newQty;
        if (!reason.empty())
            std::cout << " | Reason: " << reason;
        std::cout << "\n";
    }
};
