#pragma once
#include <string>
#include <vector>
#include <fstream>

const std::string BLOCK_SEP = "--------------------";
const std::string ISO_MIN = "00000000";
const std::string ISO_MAX = "99999999";

enum SortCriteria { BY_NAME, BY_ADDR, BY_DIR, BY_TYPE, BY_DATE };

struct Organization {
    std::string rawBlock;
    bool isEmpty;
};

bool isValidFilename(const std::string& filename);
bool fileExists(const std::string& filename);
std::string findSmartInputFile();
std::string getIndexedName(const std::string& baseName);
std::string generateOutputFilename(const std::string& input, SortCriteria crit, bool asc);
void printInstructions(const std::string& helpFile);
void performGroupingSort(const std::string& inputFile, const std::string& outputFile, SortCriteria criteria, bool ascending);