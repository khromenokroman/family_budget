#pragma once

#include <string>

/// \brief Тип операции: доход или расход.
enum class TransactionType {
    Income,
    Expense
};

/// \brief Преобразует тип операции в строку для хранения в БД.
inline const char* to_string(TransactionType type) {
    return type == TransactionType::Income ? "income" : "expense";
}

/// \brief Восстанавливает тип операции из строки, прочитанной из БД.
inline TransactionType transaction_type_from_string(const std::string& s) {
    return s == "income" ? TransactionType::Income : TransactionType::Expense;
}

/// \brief Категория дохода или расхода.
struct Category {
    int id = 0;
    std::string name;
    TransactionType type = TransactionType::Expense;
};

/// \brief Одна операция (доход или расход).
struct Transaction {
    int id = 0;
    int categoryId = 0;
    std::string categoryName;
    double amount = 0.0;
    TransactionType type = TransactionType::Expense;
    std::string date;
    std::string note;
};

/// \brief Сумма операций по одной категории.
struct CategoryTotal {
    std::string categoryName;
    double total = 0.0;
};

/// \brief Суммы доходов и расходов за один месяц.
struct MonthTotal {
    std::string month;
    double income = 0.0;
    double expense = 0.0;
};
