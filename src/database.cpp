#include "database.hpp"

#include <stdexcept>

namespace {

class Statement {
public:
    Statement(sqlite3* db, const std::string& sql) {
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &m_stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(std::string("Ошибка подготовки запроса: ") + sqlite3_errmsg(db));
        }
    }

    ~Statement() {
        sqlite3_finalize(m_stmt);
    }

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    sqlite3_stmt* get() const { return m_stmt; }

private:
    sqlite3_stmt* m_stmt = nullptr;
};

void bind_text(Statement& stmt, int index, const std::string& value) {
    sqlite3_bind_text(stmt.get(), index, value.c_str(), -1, SQLITE_TRANSIENT);
}

} // namespace

Database::Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &m_db) != SQLITE_OK) {
        std::string msg = sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        m_db = nullptr;
        throw std::runtime_error("Не удалось открыть базу данных: " + msg);
    }
    sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    init_schema();
}

Database::~Database() {
    if (m_db) {
        sqlite3_close(m_db);
    }
}

void Database::init_schema() {
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
    if (sqlite3_exec(m_db, schema, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string msg = errMsg ? errMsg : "неизвестная ошибка";
        sqlite3_free(errMsg);
        throw std::runtime_error("Не удалось создать схему БД: " + msg);
    }

    // Заполняем типичными категориями только при первом запуске (пустая таблица)
    Statement countStmt(m_db, "SELECT COUNT(*) FROM categories;");
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
            add_category(name, type);
        }
    }
}

int Database::add_category(const std::string& name, TransactionType type) {
    Statement stmt(m_db, "INSERT INTO categories (name, type) VALUES (?, ?);");
    bind_text(stmt, 1, name);
    bind_text(stmt, 2, to_string(type));
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("Не удалось добавить категорию: ") + sqlite3_errmsg(m_db));
    }
    return static_cast<int>(sqlite3_last_insert_rowid(m_db));
}

bool Database::delete_category(int id) {
    Statement stmt(m_db, "DELETE FROM categories WHERE id = ?;");
    sqlite3_bind_int(stmt.get(), 1, id);
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("Не удалось удалить категорию: ") + sqlite3_errmsg(m_db));
    }
    return sqlite3_changes(m_db) > 0;
}

bool Database::category_exists(const std::string& name, TransactionType type) const {
    Statement stmt(m_db, "SELECT 1 FROM categories WHERE name = ? AND type = ?;");
    bind_text(stmt, 1, name);
    bind_text(stmt, 2, to_string(type));
    return sqlite3_step(stmt.get()) == SQLITE_ROW;
}

std::vector<Category> Database::list_categories(TransactionType type) const {
    std::vector<Category> result;
    Statement stmt(m_db, "SELECT id, name, type FROM categories WHERE type = ? ORDER BY name;");
    bind_text(stmt, 1, to_string(type));
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        Category c;
        c.id = sqlite3_column_int(stmt.get(), 0);
        c.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
        c.type = transaction_type_from_string(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2)));
        result.push_back(std::move(c));
    }
    return result;
}

std::vector<Category> Database::list_categories() const {
    std::vector<Category> result;
    Statement stmt(m_db, "SELECT id, name, type FROM categories ORDER BY type, name;");
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        Category c;
        c.id = sqlite3_column_int(stmt.get(), 0);
        c.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
        c.type = transaction_type_from_string(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2)));
        result.push_back(std::move(c));
    }
    return result;
}

int Database::add_transaction(int category_id, double amount, TransactionType type,
                               const std::string& date, const std::string& note) {
    Statement stmt(m_db, "INSERT INTO transactions (category_id, amount, type, date, note) VALUES (?, ?, ?, ?, ?);");
    sqlite3_bind_int(stmt.get(), 1, category_id);
    sqlite3_bind_double(stmt.get(), 2, amount);
    bind_text(stmt, 3, to_string(type));
    bind_text(stmt, 4, date);
    bind_text(stmt, 5, note);
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("Не удалось добавить операцию: ") + sqlite3_errmsg(m_db));
    }
    return static_cast<int>(sqlite3_last_insert_rowid(m_db));
}

std::vector<Transaction> Database::list_transactions(int limit) const {
    std::vector<Transaction> result;
    Statement stmt(m_db, R"SQL(
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
        t.type = transaction_type_from_string(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 4)));
        t.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 5));
        const unsigned char* note = sqlite3_column_text(stmt.get(), 6);
        t.note = note ? reinterpret_cast<const char*>(note) : "";
        result.push_back(std::move(t));
    }
    return result;
}

bool Database::delete_transaction(int id) {
    Statement stmt(m_db, "DELETE FROM transactions WHERE id = ?;");
    sqlite3_bind_int(stmt.get(), 1, id);
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("Не удалось удалить операцию: ") + sqlite3_errmsg(m_db));
    }
    return sqlite3_changes(m_db) > 0;
}

double Database::total_by_type(TransactionType type) const {
    Statement stmt(m_db, "SELECT COALESCE(SUM(amount), 0) FROM transactions WHERE type = ?;");
    bind_text(stmt, 1, to_string(type));
    sqlite3_step(stmt.get());
    return sqlite3_column_double(stmt.get(), 0);
}

std::vector<CategoryTotal> Database::totals_by_category(TransactionType type) const {
    std::vector<CategoryTotal> result;
    Statement stmt(m_db, R"SQL(
        SELECT c.name, SUM(t.amount) AS total
        FROM transactions t
        JOIN categories c ON c.id = t.category_id
        WHERE t.type = ?
        GROUP BY c.name
        ORDER BY total DESC;
    )SQL");
    bind_text(stmt, 1, to_string(type));
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        CategoryTotal ct;
        ct.categoryName = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        ct.total = sqlite3_column_double(stmt.get(), 1);
        result.push_back(std::move(ct));
    }
    return result;
}

std::vector<MonthTotal> Database::totals_by_month(int months_back) const {
    std::vector<MonthTotal> result;
    Statement stmt(m_db, R"SQL(
        SELECT strftime('%Y-%m', date) AS month,
               SUM(CASE WHEN type = 'income' THEN amount ELSE 0 END) AS income,
               SUM(CASE WHEN type = 'expense' THEN amount ELSE 0 END) AS expense
        FROM transactions
        GROUP BY month
        ORDER BY month DESC
        LIMIT ?;
    )SQL");
    sqlite3_bind_int(stmt.get(), 1, months_back);
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        MonthTotal mt;
        mt.month = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        mt.income = sqlite3_column_double(stmt.get(), 1);
        mt.expense = sqlite3_column_double(stmt.get(), 2);
        result.push_back(std::move(mt));
    }
    return result;
}
