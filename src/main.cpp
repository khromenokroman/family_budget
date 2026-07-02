#include <cstdlib>
#include <iostream>
#include <string>

#include "app.hpp"
#include "database.hpp"

namespace {

std::string databasePath() {
    const char* home = std::getenv("HOME");
    std::string dir = home ? home : ".";
    return dir + "/.family_budget.db";
}

} // namespace

int main() {
    try {
        Database db(databasePath());
        App app(db);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
