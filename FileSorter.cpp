#include "FileSorter.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <windows.h> // Оставлен только для поиска файлов и кодировки

// Проверка на запрещенные символы для имени файла
bool isValidFilename(const std::string& filename) {
    if (filename.empty()) return false;
    return filename.find_first_of("<>:\"/\\|?*") == std::string::npos; // Если не найдено (npos), значит валидно
}

// Проверка существования файла путем попытки его открытия
bool fileExists(const std::string& filename) {
    std::ifstream file(filename.c_str());
    return file.is_open();
}

// Автопоиск файла в папке
std::string findSmartInputFile() {
    WIN32_FIND_DATAA findData; // Структура для хранения инфы о файле
    // Ищем есть ли файлы .txt
    HANDLE searchHandle = FindFirstFileA("*.txt", &findData);
    std::string fallbackFile = "";

    // Если поиск удался (возвращен не ошибочный номер)
    if (searchHandle != INVALID_HANDLE_VALUE) {
        do {
            std::string currentFileName = findData.cFileName;

            // Пропускаем файл с инструкцией
            if (currentFileName.find("instr") != std::string::npos) continue;


            // Создаем копию имени и приводим всё к нижнему регистру
            std::string lowerName = currentFileName;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

            // Если находим слово отчет
            if (lowerName.find("тчет") != std::string::npos) {
                FindClose(searchHandle); // Закрываем поиск
                return currentFileName;
            }

            // Запоминаем первый попавшийся файл как запасной вариант
            if (fallbackFile.empty()) fallbackFile = currentFileName;

        } while (FindNextFileA(searchHandle, &findData)); // Пока есть следующие файлы

        FindClose(searchHandle);
    }
    return fallbackFile;
}

// Поиск свободного имени файла (создание индекса)
std::string getIndexedName(const std::string& baseName) {
    int index = 1;
    while (true) {
        std::string testName = baseName + "(" + std::to_string(index) + ")";
        if (!fileExists(testName + ".txt")) {
            return testName; // Возвращаем первое свободное имя
        }
        index++;
    }
}

// Генерация авто-названия
std::string generateOutputFilename(const std::string& inputName, SortCriteria criteria, bool isAscending) {
    std::string suffix;
    switch (criteria) {
    case BY_NAME: suffix = "_Имя"; break;
    case BY_ADDR: suffix = "_Адрес"; break;
    case BY_DIR:  suffix = "_Дир";  break;
    case BY_TYPE: suffix = "_Вид"; break;
    case BY_DATE: suffix = "_Дата"; break;
    }
    suffix += (isAscending ? "_Возр" : "_Убыв");

    // Отрезаем .txt от входного имени и приклеиваем суффикс
    size_t dotPos = inputName.find_last_of('.');
    return (dotPos == std::string::npos ? inputName + suffix : inputName.substr(0, dotPos) + suffix);
}

// Конвертация даты ДД.ММ.ГГГГ в ГГГГММДД для сравнения строк
std::string parseToISO(std::string dateStr) {
    dateStr.erase(0, dateStr.find_first_not_of(" \t"));
    if (dateStr.length() < 10) return ISO_MAX;
    return dateStr.substr(6, 4) + dateStr.substr(3, 2) + dateStr.substr(0, 2);
}

// Чтение одного блока до разделителя
Organization readNext(std::ifstream& file) {
    Organization org;
    org.isEmpty = true;

    std::string line;
    std::stringstream buffer;

    while (std::getline(file, line)) {
        buffer << line << "\n";
        org.isEmpty = false;
        if (line.find(BLOCK_SEP) != std::string::npos) break; // Нашли 20 дефисов - выходим
    }
    org.rawBlock = buffer.str();
    return org;
}

// Поиск всех значений в блоке по критерию
std::vector<std::string> extractValues(const std::string& blockText, SortCriteria criteria) {
    std::vector<std::string> results;
    std::stringstream ss(blockText);
    std::string line;

    while (std::getline(ss, line)) {
        size_t colonPos = std::string::npos;

        if (criteria == BY_NAME && (colonPos = line.find("Название организации:")) != std::string::npos) {
            results.push_back(line.substr(colonPos + 22)); // 22 - длина фразы до двоеточия
        }
        else if (criteria == BY_ADDR && (colonPos = line.find("Адрес:")) != std::string::npos) {
            results.push_back(line.substr(colonPos + 7));
        }
        else if (criteria == BY_DIR && (colonPos = line.find("Фамилия руководителя:")) != std::string::npos) {
            results.push_back(line.substr(colonPos + 22));
        }
        else if (criteria == BY_TYPE && (colonPos = line.find("- Вид:")) != std::string::npos) {
            size_t commaPos = line.find(',', colonPos);
            int length = (commaPos == std::string::npos ? line.length() : commaPos) - (colonPos + 7);
            results.push_back(line.substr(colonPos + 7, length));
        }
        else if (criteria == BY_DATE && (colonPos = line.find("Дата:")) != std::string::npos) {
            results.push_back(parseToISO(line.substr(colonPos + 6)));
        }
    }

    // Очистка от пробелов и дубликатов (если в одном блоке две одинаковые даты)
    for (auto& str : results) {
        str.erase(0, str.find_first_not_of(" \t"));
        str.erase(str.find_last_not_of(" \r\n\t") + 1);
    }
    std::sort(results.begin(), results.end());
    results.erase(std::unique(results.begin(), results.end()), results.end());

    return results;
}

void performGroupingSort(const std::string& inputFile, const std::string& outputFile, SortCriteria criteria, bool isAscending) {
    std::ofstream(outputFile).close(); // Создаем пустой файл

    std::string lastProcessedValue = "";
    bool isFirstPass = true;

    while (true) {
        std::ifstream fileIn(inputFile);
        std::string currentBestValue = "";
        bool isGroupFound = false;

        // ПРОХОД 1: Ищем следующее по порядку уникальное значение
        while (fileIn.peek() != EOF) { // Пока не конец файла
            Organization org = readNext(fileIn);
            if (org.isEmpty) continue;

            for (const auto& value : extractValues(org.rawBlock, criteria)) {
                // Подходит ли значение? (Оно должно быть больше того, что мы уже обработали)
                bool isNewValue = isFirstPass || (isAscending ? value > lastProcessedValue : value < lastProcessedValue);

                if (isNewValue) {
                    if (!isGroupFound) {
                        currentBestValue = value;
                        isGroupFound = true;
                    }
                    else {
                        // Ищем минимум из оставшихся
                        if (isAscending && value < currentBestValue) currentBestValue = value;
                        if (!isAscending && value > currentBestValue) currentBestValue = value;
                    }
                }
            }
        }
        fileIn.close();

        if (!isGroupFound) break; // Все значения обработаны

        // ПРОХОД 2: Записываем в файл все блоки с найденным значением
        std::ofstream fileOut(outputFile, std::ios::app);
        std::string headerLabel = currentBestValue;

        // Возвращаем дату к нормальному виду для заголовка
        if (criteria == BY_DATE && headerLabel.length() == 8) {
            headerLabel = headerLabel.substr(6, 2) + "." + headerLabel.substr(4, 2) + "." + headerLabel.substr(0, 4);
        }

        fileOut << "============================================================\n";
        fileOut << " ГРУППА: " << headerLabel << "\n";
        fileOut << "============================================================\n";

        fileIn.open(inputFile);
        while (fileIn.peek() != EOF) {
            Organization org = readNext(fileIn);
            if (org.isEmpty) continue;

            std::vector<std::string> orgValues = extractValues(org.rawBlock, criteria);
            // Если в блоке есть наше значение - записываем
            if (std::find(orgValues.begin(), orgValues.end(), currentBestValue) != orgValues.end()) {
                fileOut << org.rawBlock << "\n";
            }
        }
        fileOut.close();
        fileIn.close();

        lastProcessedValue = currentBestValue;
        isFirstPass = false;
        std::cout << "."; // Индикатор загрузки
    }
    std::cout << "\n Запись успешно завершена.\n";
}

void printInstructions(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cout << "\n ! Инструкция (instructions.txt) не найдена.\n";
        return;
    }
    std::string line;
    std::cout << "\n------------------------------------------------------------\n";
    while (std::getline(file, line)) std::cout << " " << line << "\n";
    std::cout << "------------------------------------------------------------\n";
}