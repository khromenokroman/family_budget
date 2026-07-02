#include "app.hpp"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>

namespace {

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

std::optional<double> parseDouble(const std::string& s) {
    try {
        size_t pos = 0;
        double value = std::stod(s, &pos);
        if (pos != s.size()) return std::nullopt;
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<int> parseInt(const std::string& s) {
    try {
        size_t pos = 0;
        int value = std::stoi(s, &pos);
        if (pos != s.size()) return std::nullopt;
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

double readPositiveAmount() {
    while (true) {
        std::string s = readLine("Сумма: ");
        auto value = parseDouble(s);
        if (value && *value > 0) return *value;
        std::cout << "Введите положительное число.\n";
    }
}

int readMenuChoice(const std::string& prompt) {
    while (true) {
        std::string s = readLine(prompt);
        auto value = parseInt(s);
        if (value) return *value;
        std::cout << "Введите число.\n";
    }
}

std::string currentDate() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

std::string readDateOrToday() {
    std::string today = currentDate();
    std::string s = readLine("Дата (ГГГГ-ММ-ДД, Enter = сегодня " + today + "): ");
    if (s.empty()) return today;
    return s;
}

void printMoney(double value) {
    std::cout << std::fixed << std::setprecision(2) << value;
}

} // namespace

App::App(Database& db) : db_(db) {}

void App::run() {
    while (true) {
        showMainMenu();
        int choice = readMenuChoice("Выберите пункт: ");
        std::cout << "\n";
        switch (choice) {
            case 1: addTransactionFlow(TransactionType::Income); break;
            case 2: addTransactionFlow(TransactionType::Expense); break;
            case 3: categoriesMenu(); break;
            case 4: transactionsMenu(); break;
            case 5: statisticsMenu(); break;
            case 0: return;
            default: std::cout << "Нет такого пункта меню.\n"; break;
        }
        std::cout << "\n";
    }
}

void App::showMainMenu() {
    std::cout << "=== Семейный бюджет ===\n";
    std::cout << "1. Добавить доход\n";
    std::cout << "2. Добавить расход\n";
    std::cout << "3. Категории\n";
    std::cout << "4. Операции\n";
    std::cout << "5. Статистика\n";
    std::cout << "0. Выход\n";
}

void App::addTransactionFlow(TransactionType type) {
    auto categories = db_.listCategories(type);
    if (categories.empty()) {
        std::cout << "Нет ни одной категории для " << (type == TransactionType::Income ? "доходов" : "расходов")
                   << ". Сначала добавьте категорию.\n";
        return;
    }

    std::cout << "Категории:\n";
    for (size_t i = 0; i < categories.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << categories[i].name << "\n";
    }

    int index = readMenuChoice("Выберите категорию: ");
    if (index < 1 || static_cast<size_t>(index) > categories.size()) {
        std::cout << "Нет такой категории.\n";
        return;
    }
    const Category& category = categories[index - 1];

    double amount = readPositiveAmount();
    std::string date = readDateOrToday();
    std::string note = readLine("Комментарий (необязательно): ");

    db_.addTransaction(category.id, amount, type, date, note);
    std::cout << (type == TransactionType::Income ? "Доход" : "Расход") << " добавлен.\n";
}

void App::categoriesMenu() {
    while (true) {
        std::cout << "--- Категории ---\n";
        std::cout << "1. Список категорий\n";
        std::cout << "2. Добавить категорию\n";
        std::cout << "3. Удалить категорию\n";
        std::cout << "0. Назад\n";
        int choice = readMenuChoice("Выберите пункт: ");
        std::cout << "\n";
        switch (choice) {
            case 1: listCategoriesFlow(); break;
            case 2: addCategoryFlow(); break;
            case 3: deleteCategoryFlow(); break;
            case 0: return;
            default: std::cout << "Нет такого пункта меню.\n"; break;
        }
        std::cout << "\n";
    }
}

void App::listCategoriesFlow() {
    auto categories = db_.listCategories();
    if (categories.empty()) {
        std::cout << "Категорий пока нет.\n";
        return;
    }
    std::cout << "Доходы:\n";
    for (const auto& c : categories) {
        if (c.type == TransactionType::Income) {
            std::cout << "  [" << c.id << "] " << c.name << "\n";
        }
    }
    std::cout << "Расходы:\n";
    for (const auto& c : categories) {
        if (c.type == TransactionType::Expense) {
            std::cout << "  [" << c.id << "] " << c.name << "\n";
        }
    }
}

void App::addCategoryFlow() {
    std::string name = readLine("Название категории: ");
    if (name.empty()) {
        std::cout << "Название не может быть пустым.\n";
        return;
    }
    std::string typeChoice = readLine("Тип (1 - доход, 2 - расход): ");
    TransactionType type = (typeChoice == "1") ? TransactionType::Income : TransactionType::Expense;

    if (db_.categoryExists(name, type)) {
        std::cout << "Такая категория уже существует.\n";
        return;
    }
    db_.addCategory(name, type);
    std::cout << "Категория добавлена.\n";
}

void App::deleteCategoryFlow() {
    listCategoriesFlow();
    int id = readMenuChoice("Введите ID категории для удаления (0 - отмена): ");
    if (id == 0) return;
    bool deleted = db_.deleteCategory(id);
    std::cout << (deleted ? "Категория удалена.\n" : "Категория не найдена.\n");
}

void App::transactionsMenu() {
    while (true) {
        std::cout << "--- Операции ---\n";
        std::cout << "1. Список операций\n";
        std::cout << "2. Удалить операцию\n";
        std::cout << "0. Назад\n";
        int choice = readMenuChoice("Выберите пункт: ");
        std::cout << "\n";
        switch (choice) {
            case 1: showTransactionsFlow(); break;
            case 2: deleteTransactionFlow(); break;
            case 0: return;
            default: std::cout << "Нет такого пункта меню.\n"; break;
        }
        std::cout << "\n";
    }
}

void App::showTransactionsFlow() {
    auto transactions = db_.listTransactions(50);
    if (transactions.empty()) {
        std::cout << "Операций пока нет.\n";
        return;
    }
    std::cout << "Последние операции (не более 50):\n";
    for (const auto& t : transactions) {
        std::cout << "[" << t.id << "]  " << t.date << "  "
                   << (t.type == TransactionType::Income ? "+" : "-");
        printMoney(t.amount);
        std::cout << "  " << t.categoryName;
        if (!t.note.empty()) {
            std::cout << "  (" << t.note << ")";
        }
        std::cout << "\n";
    }
}

void App::deleteTransactionFlow() {
    showTransactionsFlow();
    int id = readMenuChoice("Введите ID операции для удаления (0 - отмена): ");
    if (id == 0) return;
    bool deleted = db_.deleteTransaction(id);
    std::cout << (deleted ? "Операция удалена.\n" : "Операция не найдена.\n");
}

void App::statisticsMenu() {
    while (true) {
        std::cout << "--- Статистика ---\n";
        std::cout << "1. Баланс\n";
        std::cout << "2. Расходы по категориям\n";
        std::cout << "3. Доходы по категориям\n";
        std::cout << "4. По месяцам\n";
        std::cout << "0. Назад\n";
        int choice = readMenuChoice("Выберите пункт: ");
        std::cout << "\n";
        switch (choice) {
            case 1: showBalanceFlow(); break;
            case 2: showCategoryBreakdownFlow(TransactionType::Expense); break;
            case 3: showCategoryBreakdownFlow(TransactionType::Income); break;
            case 4: showMonthlyBreakdownFlow(); break;
            case 0: return;
            default: std::cout << "Нет такого пункта меню.\n"; break;
        }
        std::cout << "\n";
    }
}

void App::showBalanceFlow() {
    double income = db_.totalByType(TransactionType::Income);
    double expense = db_.totalByType(TransactionType::Expense);
    std::cout << "Доходы:  +"; printMoney(income); std::cout << "\n";
    std::cout << "Расходы: -"; printMoney(expense); std::cout << "\n";
    std::cout << "Баланс:   "; printMoney(income - expense); std::cout << "\n";
}

void App::showCategoryBreakdownFlow(TransactionType type) {
    auto totals = db_.totalsByCategory(type);
    if (totals.empty()) {
        std::cout << "Данных пока нет.\n";
        return;
    }
    double sum = 0.0;
    for (const auto& ct : totals) sum += ct.total;

    std::cout << (type == TransactionType::Income ? "Доходы" : "Расходы") << " по категориям:\n";
    for (const auto& ct : totals) {
        double percent = sum > 0 ? (ct.total / sum * 100.0) : 0.0;
        std::cout << "  " << ct.categoryName << ": ";
        printMoney(ct.total);
        std::cout << " (" << std::fixed << std::setprecision(1) << percent << "%)\n";
    }
    std::cout << "Итого: ";
    printMoney(sum);
    std::cout << "\n";
}

void App::showMonthlyBreakdownFlow() {
    auto months = db_.totalsByMonth(12);
    if (months.empty()) {
        std::cout << "Данных пока нет.\n";
        return;
    }
    std::cout << "Месяц      Доход        Расход       Баланс\n";
    for (const auto& m : months) {
        std::cout << m.month << "   +";
        printMoney(m.income);
        std::cout << "   -";
        printMoney(m.expense);
        std::cout << "   ";
        printMoney(m.income - m.expense);
        std::cout << "\n";
    }
}
