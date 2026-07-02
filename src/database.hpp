#pragma once

#include <string>
#include <vector>

#include <sqlite3.h>

#include "models.hpp"

class Database {
public:
    explicit Database(const std::string& path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    int add_category(const std::string& name, TransactionType type);
    bool delete_category(int id);
    std::vector<Category> list_categories(TransactionType type) const;
    std::vector<Category> list_categories() const;
    bool category_exists(const std::string& name, TransactionType type) const;

    int add_transaction(int category_id, double amount, TransactionType type,
                         const std::string& date, const std::string& note);
    std::vector<Transaction> list_transactions(int limit) const;
    bool delete_transaction(int id);

    double total_by_type(TransactionType type) const;
    std::vector<CategoryTotal> totals_by_category(TransactionType type) const;
    std::vector<MonthTotal> totals_by_month(int months_back) const;

private:
    sqlite3* m_db = nullptr;

    void init_schema();
};
