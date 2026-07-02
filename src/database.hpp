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

    int addCategory(const std::string& name, TransactionType type);
    bool deleteCategory(int id);
    std::vector<Category> listCategories(TransactionType type) const;
    std::vector<Category> listCategories() const;
    bool categoryExists(const std::string& name, TransactionType type) const;

    int addTransaction(int categoryId, double amount, TransactionType type,
                        const std::string& date, const std::string& note);
    std::vector<Transaction> listTransactions(int limit) const;
    bool deleteTransaction(int id);

    double totalByType(TransactionType type) const;
    std::vector<CategoryTotal> totalsByCategory(TransactionType type) const;
    std::vector<MonthTotal> totalsByMonth(int monthsBack) const;

private:
    sqlite3* db_ = nullptr;

    void initSchema();
};
