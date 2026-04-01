// FileSorter.h
#pragma once
#include <string>
#include <vector>
#include <fstream>

// Разделитель блоков данных во входном файле.
const std::string BLOCK_SEP = "--------------------";

// Технические константы дат. 
// Применяются для управления позицией записей без дат при лексикографической сортировке.
const std::string ISO_MIN = "00000000";
const std::string ISO_MAX = "99999999";

// Перечисление доступных полей для группировки и сортировки.
enum SortCriteria { BY_NAME, BY_ADDR, BY_DIR, BY_TYPE, BY_DATE };

// Структура декомпозированного блока данных организации.
// Векторы используются для фильтрации множественных вхождений (несколько адресов/писем).
struct Organization {
    std::string nameLine;
    std::vector<std::string> addresses;
    std::vector<std::string> directors;
    std::string corrHeader;
    std::vector<std::string> docs;
    bool isEmpty;
};

// Валидация строки на отсутствие запрещенных символов файловой системы ОС.
bool isValidFilename(const std::string& filename);

// Проверка физического существования файла через попытку открытия потока.
bool fileExists(const std::string& filename);

// Сканирование текущей директории на наличие текстовых файлов.
std::vector<std::string> getAvailableFiles();

// Разрешение коллизий имен файлов путем инкрементирования числового индекса.
std::string getIndexedName(const std::string& baseName);

// Формирование суффикса выходного файла на основе выбранных параметров.
std::string generateOutputFilename(const std::string& input, SortCriteria crit, bool asc);

// Вывод справочной информации из внешнего файла.
void printInstructions(const std::string& helpFile);

// Алгоритм внешней сортировки и группировки (Selection Sort).
void performSelectionSort(const std::string& inputFile, const std::string& outputFile, SortCriteria criteria, bool ascending);