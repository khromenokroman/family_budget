#include "Database.hpp"

#include <stdexcept>

namespace {

class Statement {
public:
    Statement(sqlite3* db, const std::string& sql) {
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK) {
            throw std::runtime_error(std::string("Ошибка подготовки запроса: ") + sqlite3_errmsg(db));
        }
    }

    ~Statement() {
        sqlite3_finalize(stmt_);
    }

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    sqlite3_stmt* get() const { return stmt_; }

private:
    sqlite3_stmt* stmt_ = nullptr;
};

void bindText(Statement& stmt, int index, const std::string& value) {
    sqlite3_bind_text(stmt.get(), index, value.c_str(), -1, SQLITE_TRANSIENT);
}

} // namespace

Database::Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        std::string msg = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error("Не удалось открыть базу данных: " + msg);
    }
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    initSchema();
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

void Database::initSchema() {
    const char* schema = R"SQL(
        CREATE TABLE IF NOT EXISTS categories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            type TEXT NOT NULL CHECK(type IN ('income','expense')),
            UNIQUE(name, type)
        );

        CREATE TABLE IF NOT EXISTS transactions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            category_id INTEGER NOT NULL REFERENCES categories(id),
            amount REAL NOT NULL,
            type TEXT NOT NULL CHECK(type IN ('income','expense')),
            date TEXT NOT NULL,
            note TEXT NOT NULL DEFAULT ''
        );
    )SQL";

    char* errMsg = nullptr;
    if (sqlite3_exec(db_, schema, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string msg = errMsg ? errMsg : "неизвестная ошибка";
        sqlite3_free(errMsg);
        throw std::runtime_error("Не удалось создать схему БД: " + msg);
    }

    // Заполняем типичными категориями только при первом запуске (пустая таблица)
    Statement countStmt(db_, "SELECT COUNT(*) FROM categories;");
    sqlite3_step(countStmt.get());
    int count = sqlite3_column_int(countStmt.get(), 0);
    if (count == 0) {
        static const std::pair<const char*, TransactionType> defaults[] = {
            {"Продукты", TransactionType::Expense},
            {"Транспорт", TransactionType::Expense},
            {"ЖКХ", TransactionType::Expense},
            {"Развлечения", TransactionType::Expense},
            {"Здоровье", TransactionType::Expense},
            {"Одежда", TransactionType::Expense},
            {"Прочее", TransactionType::Expense},
            {"Зарплата", TransactionType::Income},
            {"Подработка", TransactionType::Income},
            {"Прочее", TransactionType::Income},
        };
        for (const auto& [name, type] : defaults) {
            addCategory(name, type);
        }
    }
}

int Database::addCategory(const std::string& name, TransactionType type) {
    Statement stmt(db_, "INSERT INTO categories (name, type) VALUES (?, ?);");
    bindText(stmt, 1, name);
    bindText(stmt, 2, toString(type));
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("Не удалось добавить категорию: ") + sqlite3_errmsg(db_));
    }
    return static_cast<int>(sqlite3_last_insert_rowid(db_));
}

bool Database::deleteCategory(int id) {
    Statement stmt(db_, "DELETE FROM categories WHERE id = ?;");
    sqlite3_bind_int(stmt.get(), 1, id);
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("Не удалось удалить категорию: ") + sqlite3_errmsg(db_));
    }
    return sqlite3_changes(db_) > 0;
}

bool Database::categoryExists(const std::string& name, TransactionType type) const {
    Statement stmt(db_, "SELECT 1 FROM categories WHERE name = ? AND type = ?;");
    bindText(stmt, 1, name);
    bindText(stmt, 2, toString(type));
    return sqlite3_step(stmt.get()) == SQLITE_ROW;
}

std::vector<Category> Database::listCategories(TransactionType type) const {
    std::vector<Category> result;
    Statement stmt(db_, "SELECT id, name, type FROM categories WHERE type = ? ORDER BY name;");
    bindText(stmt, 1, toString(type));
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        Category c;
        c.id = sqlite3_column_int(stmt.get(), 0);
        c.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
        c.type = transactionTypeFromString(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2)));
        result.push_back(std::move(c));
    }
    return result;
}

std::vector<Category> Database::listCategories() const {
    std::vector<Category> result;
    Statement stmt(db_, "SELECT id, name, type FROM categories ORDER BY type, name;");
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        Category c;
        c.id = sqlite3_column_int(stmt.get(), 0);
        c.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
        c.type = transactionTypeFromString(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2)));
        result.push_back(std::move(c));
    }
    return result;
}

int Database::addTransaction(int categoryId, double amount, TransactionType type,
                              const std::string& date, const std::string& note) {
    Statement stmt(db_, "INSERT INTO transactions (category_id, amount, type, date, note) VALUES (?, ?, ?, ?, ?);");
    sqlite3_bind_int(stmt.get(), 1, categoryId);
    sqlite3_bind_double(stmt.get(), 2, amount);
    bindText(stmt, 3, toString(type));
    bindText(stmt, 4, date);
    bindText(stmt, 5, note);
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("Не удалось добавить операцию: ") + sqlite3_errmsg(db_));
    }
    return static_cast<int>(sqlite3_last_insert_rowid(db_));
}

std::vector<Transaction> Database::listTransactions(int limit) const {
    std::vector<Transaction> result;
    Statement stmt(db_, R"SQL(
        SELECT t.id, t.category_id, c.name, t.amount, t.type, t.date, t.note
        FROM transactions t
        JOIN categories c ON c.id = t.category_id
        ORDER BY t.date DESC, t.id DESC
        LIMIT ?;
    )SQL");
    sqlite3_bind_int(stmt.get(), 1, limit);
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        Transaction t;
        t.id = sqlite3_column_int(stmt.get(), 0);
        t.categoryId = sqlite3_column_int(stmt.get(), 1);
        t.categoryName = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
        t.amount = sqlite3_column_double(stmt.get(), 3);
        t.type = transactionTypeFromString(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 4)));
        t.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 5));
        const unsigned char* note = sqlite3_column_text(stmt.get(), 6);
        t.note = note ? reinterpret_cast<const char*>(note) : "";
        result.push_back(std::move(t));
    }
    return result;
}

bool Database::deleteTransaction(int id) {
    Statement stmt(db_, "DELETE FROM transactions WHERE id = ?;");
    sqlite3_bind_int(stmt.get(), 1, id);
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("Не удалось удалить операцию: ") + sqlite3_errmsg(db_));
    }
    return sqlite3_changes(db_) > 0;
}

double Database::totalByType(TransactionType type) const {
    Statement stmt(db_, "SELECT COALESCE(SUM(amount), 0) FROM transactions WHERE type = ?;");
    bindText(stmt, 1, toString(type));
    sqlite3_step(stmt.get());
    return sqlite3_column_double(stmt.get(), 0);
}

std::vector<CategoryTotal> Database::totalsByCategory(TransactionType type) const {
    std::vector<CategoryTotal> result;
    Statement stmt(db_, R"SQL(
        SELECT c.name, SUM(t.amount) AS total
        FROM transactions t
        JOIN categories c ON c.id = t.category_id
        WHERE t.type = ?
        GROUP BY c.name
        ORDER BY total DESC;
    )SQL");
    bindText(stmt, 1, toString(type));
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        CategoryTotal ct;
        ct.categoryName = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        ct.total = sqlite3_column_double(stmt.get(), 1);
        result.push_back(std::move(ct));
    }
    return result;
}

std::vector<MonthTotal> Database::totalsByMonth(int monthsBack) const {
    std::vector<MonthTotal> result;
    Statement stmt(db_, R"SQL(
        SELECT strftime('%Y-%m', date) AS month,
               SUM(CASE WHEN type = 'income' THEN amount ELSE 0 END) AS income,
               SUM(CASE WHEN type = 'expense' THEN amount ELSE 0 END) AS expense
        FROM transactions
        GROUP BY month
        ORDER BY month DESC
        LIMIT ?;
    )SQL");
    sqlite3_bind_int(stmt.get(), 1, monthsBack);
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        MonthTotal mt;
        mt.month = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        mt.income = sqlite3_column_double(stmt.get(), 1);
        mt.expense = sqlite3_column_double(stmt.get(), 2);
        result.push_back(std::move(mt));
    }
    return result;
}
