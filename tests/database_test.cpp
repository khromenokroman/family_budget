#include "database.hpp"

#include <gtest/gtest.h>

namespace {

class DatabaseTest : public ::testing::Test {
protected:
    Database db{":memory:"};
};

TEST_F(DatabaseTest, SeedsDefaultCategories) {
    EXPECT_EQ(db.list_categories().size(), 10u);
    EXPECT_EQ(db.list_categories(TransactionType::Income).size(), 3u);
    EXPECT_EQ(db.list_categories(TransactionType::Expense).size(), 7u);
}

TEST_F(DatabaseTest, AddAndDeleteCategory) {
    EXPECT_FALSE(db.category_exists("Хобби", TransactionType::Expense));

    int id = db.add_category("Хобби", TransactionType::Expense);
    EXPECT_TRUE(db.category_exists("Хобби", TransactionType::Expense));

    EXPECT_TRUE(db.delete_category(id));
    EXPECT_FALSE(db.category_exists("Хобби", TransactionType::Expense));
    EXPECT_FALSE(db.delete_category(id));
}

TEST_F(DatabaseTest, AddTransactionAffectsTotals) {
    int categoryId = db.list_categories(TransactionType::Expense).front().id;

    db.add_transaction(categoryId, 500.0, TransactionType::Expense, "2026-07-01", "тест");

    EXPECT_DOUBLE_EQ(db.total_by_type(TransactionType::Expense), 500.0);
    EXPECT_DOUBLE_EQ(db.total_by_type(TransactionType::Income), 0.0);

    auto totals = db.totals_by_category(TransactionType::Expense);
    ASSERT_EQ(totals.size(), 1u);
    EXPECT_DOUBLE_EQ(totals[0].total, 500.0);
}

TEST_F(DatabaseTest, ListAndDeleteTransaction) {
    int categoryId = db.list_categories(TransactionType::Income).front().id;
    int txId = db.add_transaction(categoryId, 1000.0, TransactionType::Income, "2026-07-02", "зарплата");

    auto list = db.list_transactions(10);
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].id, txId);
    EXPECT_EQ(list[0].note, "зарплата");

    EXPECT_TRUE(db.delete_transaction(txId));
    EXPECT_TRUE(db.list_transactions(10).empty());
    EXPECT_FALSE(db.delete_transaction(txId));
}

TEST_F(DatabaseTest, TotalsByMonth) {
    int expenseCategoryId = db.list_categories(TransactionType::Expense).front().id;
    int incomeCategoryId = db.list_categories(TransactionType::Income).front().id;

    db.add_transaction(incomeCategoryId, 1000.0, TransactionType::Income, "2026-06-15", "");
    db.add_transaction(expenseCategoryId, 200.0, TransactionType::Expense, "2026-06-20", "");
    db.add_transaction(incomeCategoryId, 500.0, TransactionType::Income, "2026-07-01", "");

    auto months = db.totals_by_month(12);
    ASSERT_EQ(months.size(), 2u);

    EXPECT_EQ(months[0].month, "2026-07");
    EXPECT_DOUBLE_EQ(months[0].income, 500.0);
    EXPECT_DOUBLE_EQ(months[0].expense, 0.0);

    EXPECT_EQ(months[1].month, "2026-06");
    EXPECT_DOUBLE_EQ(months[1].income, 1000.0);
    EXPECT_DOUBLE_EQ(months[1].expense, 200.0);
}

} // namespace
