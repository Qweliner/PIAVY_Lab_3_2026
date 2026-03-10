#pragma once
#include <string>
#include <vector>
#include <fstream>

const std::string BLOCK_SEP = "--------------------";
// Якоря для сортировки отсутствующих данных
const std::string ISO_MIN = "00000000";
const std::string ISO_MAX = "99999999";

enum SortCriteria { BY_NAME, BY_ADDR, BY_DIR, BY_TYPE, BY_DATE };

// Структура теперь хранит блок не сплошным текстом, а по полочкам.
// Это нужно, чтобы при выводе отсекать лишние письма или адре
struct Organization {
    std::string nameLine;               // Строка "Название: ..."
    std::vector<std::string> addresses; // Все строки "Адрес: ..."
    std::vector<std::string> directors; // Все строки "Фамилия: ..."
    std::string corrHeader;             // "Корреспонденция:"
    std::vector<std::string> docs;      // Строки "- Вид: ... Дата: ..."
    bool isEmpty;
};

bool isValidFilename(const std::string& filename);
bool fileExists(const std::string& filename);
std::vector<std::string> getAvailableFiles(); // Получить список txt файлов
std::string getIndexedName(const std::string& baseName);
std::string generateOutputFilename(const std::string& input, SortCriteria crit, bool asc);
void printInstructions(const std::string& helpFile);
void performSelectionSort(const std::string& inputFile, const std::string& outputFile, SortCriteria criteria, bool ascending);