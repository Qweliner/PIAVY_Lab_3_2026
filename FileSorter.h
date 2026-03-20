#pragma once
#include <string>
#include <vector>
#include <fstream>

const std::string BLOCK_SEP = "--------------------";
// “ехнические заглушки дл€ дат. ѕомогают выкинуть пустые записи в самое начало или в самый конец.
const std::string ISO_MIN = "00000000";
const std::string ISO_MAX = "99999999";

enum SortCriteria { BY_NAME, BY_ADDR, BY_DIR, BY_TYPE, BY_DATE };

// —труктура разбита на векторы, чтобы при фильтрации можно было отсекать лишнее (например, чужие адреса)
struct Organization {
    std::string nameLine;
    std::vector<std::string> addresses;
    std::vector<std::string> directors;
    std::string corrHeader;
    std::vector<std::string> docs;
    bool isEmpty;
};

bool isValidFilename(const std::string& filename);
bool fileExists(const std::string& filename);
std::vector<std::string> getAvailableFiles();
std::string getIndexedName(const std::string& baseName);
std::string generateOutputFilename(const std::string& input, SortCriteria crit, bool asc);
void printInstructions(const std::string& helpFile);
void performSelectionSort(const std::string& inputFile, const std::string& outputFile, SortCriteria criteria, bool ascending);