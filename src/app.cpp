#include "app.hpp"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>

namespace {

std::string read_line(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

std::optional<double> parse_double(const std::string& s) {
    try {
        size_t pos = 0;
        double value = std::stod(s, &pos);
        if (pos != s.size()) return std::nullopt;
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<int> parse_int(const std::string& s) {
    try {
        size_t pos = 0;
        int value = std::stoi(s, &pos);
        if (pos != s.size()) return std::nullopt;
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

double read_positive_amount() {
    while (true) {
        std::string s = read_line("Сумма: ");
        auto value = parse_double(s);
        if (value && *value > 0) return *value;
        std::cout << "Введите положительное число.\n";
    }
}

int read_menu_choice(const std::string& prompt) {
    while (true) {
        std::string s = read_line(prompt);
        auto value = parse_int(s);
        if (value) return *value;
        std::cout << "Введите число.\n";
    }
}

std::string current_date() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

std::string read_date_or_today() {
    std::string today = current_date();
    std::string s = read_line("Дата (ГГГГ-ММ-ДД, Enter = сегодня " + today + "): ");
    if (s.empty()) return today;
    return s;
}

void print_money(double value) {
    std::cout << std::fixed << std::setprecision(2) << value;
}

} // namespace

App::App(Database& db) : db_(db) {}

void App::run() {
    while (true) {
        show_main_menu();
        int choice = read_menu_choice("Выберите пункт: ");
        std::cout << "\n";
        switch (choice) {
            case 1: add_transaction_flow(TransactionType::Income); break;
            case 2: add_transaction_flow(TransactionType::Expense); break;
            case 3: categories_menu(); break;
            case 4: transactions_menu(); break;
            case 5: statistics_menu(); break;
            case 0: return;
            default: std::cout << "Нет такого пункта меню.\n"; break;
        }
        std::cout << "\n";
    }
}

void App::show_main_menu() {
    std::cout << "=== Семейный бюджет ===\n";
    std::cout << "1. Добавить доход\n";
    std::cout << "2. Добавить расход\n";
    std::cout << "3. Категории\n";
    std::cout << "4. Операции\n";
    std::cout << "5. Статистика\n";
    std::cout << "0. Выход\n";
}

void App::add_transaction_flow(TransactionType type) {
    auto categories = db_.list_categories(type);
    if (categories.empty()) {
        std::cout << "Нет ни одной категории для " << (type == TransactionType::Income ? "доходов" : "расходов")
                   << ". Сначала добавьте категорию.\n";
        return;
    }

    std::cout << "Категории:\n";
    for (size_t i = 0; i < categories.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << categories[i].name << "\n";
    }

    int index = read_menu_choice("Выберите категорию: ");
    if (index < 1 || static_cast<size_t>(index) > categories.size()) {
        std::cout << "Нет такой категории.\n";
        return;
    }
    const Category& category = categories[index - 1];

    double amount = read_positive_amount();
    std::string date = read_date_or_today();
    std::string note = read_line("Комментарий (необязательно): ");

    db_.add_transaction(category.id, amount, type, date, note);
    std::cout << (type == TransactionType::Income ? "Доход" : "Расход") << " добавлен.\n";
}

void App::categories_menu() {
    while (true) {
        std::cout << "--- Категории ---\n";
        std::cout << "1. Список категорий\n";
        std::cout << "2. Добавить категорию\n";
        std::cout << "3. Удалить категорию\n";
        std::cout << "0. Назад\n";
        int choice = read_menu_choice("Выберите пункт: ");
        std::cout << "\n";
        switch (choice) {
            case 1: list_categories_flow(); break;
            case 2: add_category_flow(); break;
            case 3: delete_category_flow(); break;
            case 0: return;
            default: std::cout << "Нет такого пункта меню.\n"; break;
        }
        std::cout << "\n";
    }
}

void App::list_categories_flow() {
    auto categories = db_.list_categories();
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

void App::add_category_flow() {
    std::string name = read_line("Название категории: ");
    if (name.empty()) {
        std::cout << "Название не может быть пустым.\n";
        return;
    }
    std::string typeChoice = read_line("Тип (1 - доход, 2 - расход): ");
    TransactionType type = (typeChoice == "1") ? TransactionType::Income : TransactionType::Expense;

    if (db_.category_exists(name, type)) {
        std::cout << "Такая категория уже существует.\n";
        return;
    }
    db_.add_category(name, type);
    std::cout << "Категория добавлена.\n";
}

void App::delete_category_flow() {
    list_categories_flow();
    int id = read_menu_choice("Введите ID категории для удаления (0 - отмена): ");
    if (id == 0) return;
    bool deleted = db_.delete_category(id);
    std::cout << (deleted ? "Категория удалена.\n" : "Категория не найдена.\n");
}

void App::transactions_menu() {
    while (true) {
        std::cout << "--- Операции ---\n";
        std::cout << "1. Список операций\n";
        std::cout << "2. Удалить операцию\n";
        std::cout << "0. Назад\n";
        int choice = read_menu_choice("Выберите пункт: ");
        std::cout << "\n";
        switch (choice) {
            case 1: show_transactions_flow(); break;
            case 2: delete_transaction_flow(); break;
            case 0: return;
            default: std::cout << "Нет такого пункта меню.\n"; break;
        }
        std::cout << "\n";
    }
}

void App::show_transactions_flow() {
    auto transactions = db_.list_transactions(50);
    if (transactions.empty()) {
        std::cout << "Операций пока нет.\n";
        return;
    }
    std::cout << "Последние операции (не более 50):\n";
    for (const auto& t : transactions) {
        std::cout << "[" << t.id << "]  " << t.date << "  "
                   << (t.type == TransactionType::Income ? "+" : "-");
        print_money(t.amount);
        std::cout << "  " << t.categoryName;
        if (!t.note.empty()) {
            std::cout << "  (" << t.note << ")";
        }
        std::cout << "\n";
    }
}

void App::delete_transaction_flow() {
    show_transactions_flow();
    int id = read_menu_choice("Введите ID операции для удаления (0 - отмена): ");
    if (id == 0) return;
    bool deleted = db_.delete_transaction(id);
    std::cout << (deleted ? "Операция удалена.\n" : "Операция не найдена.\n");
}

void App::statistics_menu() {
    while (true) {
        std::cout << "--- Статистика ---\n";
        std::cout << "1. Баланс\n";
        std::cout << "2. Расходы по категориям\n";
        std::cout << "3. Доходы по категориям\n";
        std::cout << "4. По месяцам\n";
        std::cout << "0. Назад\n";
        int choice = read_menu_choice("Выберите пункт: ");
        std::cout << "\n";
        switch (choice) {
            case 1: show_balance_flow(); break;
            case 2: show_category_breakdown_flow(TransactionType::Expense); break;
            case 3: show_category_breakdown_flow(TransactionType::Income); break;
            case 4: show_monthly_breakdown_flow(); break;
            case 0: return;
            default: std::cout << "Нет такого пункта меню.\n"; break;
        }
        std::cout << "\n";
    }
}

void App::show_balance_flow() {
    double income = db_.total_by_type(TransactionType::Income);
    double expense = db_.total_by_type(TransactionType::Expense);
    std::cout << "Доходы:  +"; print_money(income); std::cout << "\n";
    std::cout << "Расходы: -"; print_money(expense); std::cout << "\n";
    std::cout << "Баланс:   "; print_money(income - expense); std::cout << "\n";
}

void App::show_category_breakdown_flow(TransactionType type) {
    auto totals = db_.totals_by_category(type);
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
        print_money(ct.total);
        std::cout << " (" << std::fixed << std::setprecision(1) << percent << "%)\n";
    }
    std::cout << "Итого: ";
    print_money(sum);
    std::cout << "\n";
}

void App::show_monthly_breakdown_flow() {
    auto months = db_.totals_by_month(12);
    if (months.empty()) {
        std::cout << "Данных пока нет.\n";
        return;
    }
    std::cout << "Месяц      Доход        Расход       Баланс\n";
    for (const auto& m : months) {
        std::cout << m.month << "   +";
        print_money(m.income);
        std::cout << "   -";
        print_money(m.expense);
        std::cout << "   ";
        print_money(m.income - m.expense);
        std::cout << "\n";
    }
}
