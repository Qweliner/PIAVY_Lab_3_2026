#pragma once
#include <string>
#include <vector>
#include <fstream>

// Разделитель для блоков данных в файле
const std::string BLOCK_SEP = "--------------------";

// Заглушки для пустых или невалидных дат
const std::string ISO_MIN = "00000000";
const std::string ISO_MAX = "99999999";

// Критерии сортировки (по каким полям ищем группы)
enum SortCriteria { BY_NAME, BY_ADDR, BY_DIR, BY_TYPE, BY_DATE };

// Критерии сортировки, переведенный в строку
std::string getCriteriaName(SortCriteria c);

// Структура для удобного хранения данных внутри одного блока
struct Organization {
    std::string nameLine;
    std::vector<std::string> addresses;
    std::vector<std::string> directors;
    std::string corrHeader;
    std::vector<std::string> docs;
    bool isEmpty;
};

// Проверяет, нет ли в имени файла запрещенных символов (типа < > *)
bool isValidFilename(const std::string& filename);

// Проверяет, открывается ли файл. Если открылся — значит он существует.
bool fileExists(const std::string& filename);

// Ищем до 10 текстовых файлов в папке с программой
std::vector<std::string> getAvailableFiles();

// Защита от перезаписи. Если файл есть, делаем Имя(1).txt, Имя(2).txt и т.д.
std::string getIndexedName(const std::string& baseName);

// Создает имя файла для вывода с учетом критерия сортировки и направления
std::string generateOutputFilename(const std::string& input, SortCriteria crit, bool asc);

// Выводит файл-инструкцию в консоль
void printInstructions(const std::string& helpFile);

// Переводит дату из формата ДД.ММ.ГГГГ в ГГГГММДД для корректного сравнения их как обычные строки
std::string parseToISO(std::string dateStr);

// Читаем один блок между дефисами и сразу бьем его на части (адреса, письма и т.д.)
Organization readNext(std::ifstream& file);

// Достает чистое значение нужного поля для сортировки
std::vector<std::string> extractValues(const Organization& org, SortCriteria criteria);

// Запускает процесс сортировки в два прохода - findNextKey и writeGroup
void performSelectionSort(const std::string& inputFile, const std::string& outputFile, SortCriteria criteria, bool ascending);

// Проход 1: ищет следующее по списку значение (ключ группы)
bool findNextKey(const std::string& inputFile, SortCriteria criteria, bool isAscending, const std::string& lastValue, bool isFirstPass, std::string& outBestValue);

// Проход 2: записывает в файл все организации, у которых есть найденный ключ
void writeGroup(const std::string& inputFile, std::ofstream& resFile, SortCriteria criteria, const std::string& currentKey);