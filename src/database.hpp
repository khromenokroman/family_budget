#pragma once

#include <string>
#include <vector>

#include <sqlite3.h>

#include "models.hpp"

/// \brief Хранилище данных бюджета поверх SQLite.
class Database {
public:
    /// \brief Открывает (или создаёт) базу данных по указанному пути и накатывает схему.
    explicit Database(const std::string& path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    /// \brief Добавляет новую категорию, возвращает её идентификатор.
    int add_category(const std::string& name, TransactionType type);
    /// \brief Удаляет категорию по идентификатору.
    bool delete_category(int id);
    /// \brief Возвращает категории заданного типа.
    std::vector<Category> list_categories(TransactionType type) const;
    /// \brief Возвращает все категории.
    std::vector<Category> list_categories() const;
    /// \brief Проверяет, существует ли категория с таким именем и типом.
    bool category_exists(const std::string& name, TransactionType type) const;

    /// \brief Добавляет операцию (доход или расход), возвращает её идентификатор.
    int add_transaction(int category_id, double amount, TransactionType type,
                         const std::string& date, const std::string& note);
    /// \brief Возвращает последние операции, не более limit штук.
    std::vector<Transaction> list_transactions(int limit) const;
    /// \brief Удаляет операцию по идентификатору.
    bool delete_transaction(int id);

    /// \brief Суммарная величина операций заданного типа.
    double total_by_type(TransactionType type) const;
    /// \brief Суммы операций заданного типа в разбивке по категориям.
    std::vector<CategoryTotal> totals_by_category(TransactionType type) const;
    /// \brief Суммы доходов и расходов в разбивке по месяцам.
    std::vector<MonthTotal> totals_by_month(int months_back) const;

private:
    sqlite3* m_db = nullptr;

    /// \brief Создаёт таблицы схемы и заполняет типовые категории при первом запуске.
    void init_schema();
};
