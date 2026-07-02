# family_budget

Консольное приложение для семейного учёта денег: доходы, расходы, категории и статистика — всё через терминал.

## Возможности

- **Доходы и расходы** — добавление операций с категорией, суммой, датой и комментарием
- **Категории** — свои категории для доходов и расходов, при первом запуске уже есть типовой набор (Продукты, Транспорт, ЖКХ, Зарплата и т.д.)
- **Список операций** — последние записи с возможностью удалить ошибочную
- **Статистика**
  - общий баланс (доход − расход)
  - разбивка расходов и доходов по категориям с процентами
  - разбивка по месяцам

Все данные хранятся локально в SQLite-базе `~/.family_budget.db`.

## Требования

- компилятор с поддержкой C++20
- CMake ≥ 3.16
- библиотека `libsqlite3` (заголовки + shared library)
- GoogleTest (для сборки тестов)

### Linux (Debian/Ubuntu)

```bash
sudo apt install libsqlite3-dev libgtest-dev
```

### macOS

```bash
xcode-select --install   # компилятор
brew install cmake googletest sqlite3
```

Homebrew ставит `sqlite3` как keg-only (не подменяет системную версию), поэтому
CMake нужно явно направить на него и на общий префикс Homebrew через
`CMAKE_PREFIX_PATH`:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$(brew --prefix sqlite3);$(brew --prefix)"
```

## Сборка

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

На macOS вместо `nproc` используйте `sysctl -n hw.ncpu`:

```bash
cmake --build build -j"$(sysctl -n hw.ncpu)"
```

Исполняемый файл появится в `build/family_budget`.

## Запуск

```bash
./build/family_budget
```

После запуска откроется главное меню:

```
=== Семейный бюджет ===
1. Добавить доход
2. Добавить расход
3. Категории
4. Операции
5. Статистика
0. Выход
```

Дальнейшая навигация — по номерам пунктов.

## Тесты

Юнит-тесты (GoogleTest) покрывают слой работы с базой данных:

```bash
ctest --test-dir build --output-on-failure
```

