#pragma once

#include <string>

enum class TransactionType {
    Income,
    Expense
};

inline const char* to_string(TransactionType type) {
    return type == TransactionType::Income ? "income" : "expense";
}

inline TransactionType transaction_type_from_string(const std::string& s) {
    return s == "income" ? TransactionType::Income : TransactionType::Expense;
}

struct Category {
    int id = 0;
    std::string name;
    TransactionType type = TransactionType::Expense;
};

struct Transaction {
    int id = 0;
    int categoryId = 0;
    std::string categoryName;
    double amount = 0.0;
    TransactionType type = TransactionType::Expense;
    std::string date;
    std::string note;
};

struct CategoryTotal {
    std::string categoryName;
    double total = 0.0;
};

struct MonthTotal {
    std::string month;
    double income = 0.0;
    double expense = 0.0;
};
