#pragma once

#include "database.hpp"

class App {
public:
    explicit App(Database& db);

    void run();

private:
    Database& db_;

    void showMainMenu();
    void addTransactionFlow(TransactionType type);
    void categoriesMenu();
    void listCategoriesFlow();
    void addCategoryFlow();
    void deleteCategoryFlow();
    void transactionsMenu();
    void showTransactionsFlow();
    void deleteTransactionFlow();
    void statisticsMenu();
    void showBalanceFlow();
    void showCategoryBreakdownFlow(TransactionType type);
    void showMonthlyBreakdownFlow();
};
