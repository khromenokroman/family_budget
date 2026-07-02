#pragma once

#include "database.hpp"

/// \brief Консольное меню для работы с семейным бюджетом.
class App {
public:
    explicit App(Database& db);

    /// \brief Запускает главный цикл меню.
    void run();

private:
    Database& m_db;

    void show_main_menu();
    void add_transaction_flow(TransactionType type);
    void categories_menu();
    void list_categories_flow();
    void add_category_flow();
    void delete_category_flow();
    void transactions_menu();
    void show_transactions_flow();
    void delete_transaction_flow();
    void statistics_menu();
    void show_balance_flow();
    void show_category_breakdown_flow(TransactionType type);
    void show_monthly_breakdown_flow();
};
