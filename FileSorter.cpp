#include "FileSorter.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <windows.h> 

using namespace std;

// Проверяет, нет ли в имени файла запрещенных символов (типа < > *)
bool isValidFilename(const string& filename) {
    if (filename.empty()) return false;
    return filename.find_first_of("<>:\"/\\|?*") == string::npos;
}

// Проверяет, открывается ли файл. Если открылся — значит он существует.
bool fileExists(const string& filename) {
    ifstream file(filename.c_str());
    return file.is_open();
}

// Ищем до 10 текстовых файлов в папке с программой
vector<string> getAvailableFiles() {
    vector<string> files;
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA("*.txt", &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            string name = findData.cFileName;
            // Пропускаем инструкцию
            if (name.find("instr") == string::npos) {
                files.push_back(name);
                if (files.size() >= 10) break; // Показываем максимум 10 файлов
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    return files;
}

// Защита от перезаписи. Если файл есть, делаем Имя(1).txt, Имя(2).txt и т.д.
string getIndexedName(const string& baseName) {
    string finalName = baseName + ".txt";
    if (!fileExists(finalName)) return finalName;
    int index = 1;
    while (true) {
        string testName = baseName + "(" + to_string(index) + ").txt";
        if (!fileExists(testName)) return testName;
        index++;
    }
}

// Критерии сортировки, переведенный в строку
string getCriteriaName(SortCriteria c) {
    if (c == BY_NAME) return "Название организации";
    if (c == BY_ADDR) return "Адрес";
    if (c == BY_DIR) return "Руководитель";
    if (c == BY_TYPE) return "Вид корреспонденции";
    return "Дата";
}

// Создает имя файла для вывода с учетом критерия сортировки и направления
string generateOutputFilename(const string& inputName, SortCriteria criteria, bool isAscending) {
    string suffix;
    switch (criteria) {
    case BY_NAME: suffix = "_Имя"; break;
    case BY_ADDR: suffix = "_Адрес"; break;
    case BY_DIR:  suffix = "_Директор";  break;
    case BY_TYPE: suffix = "_ТипКорреспонденции"; break;
    case BY_DATE: suffix = "_Дата"; break;
    }
    suffix += (isAscending ? "_Возр" : "_Убыв");
    size_t dotPos = inputName.find_last_of('.');
    return (dotPos == string::npos ? inputName + suffix : inputName.substr(0, dotPos) + suffix);
}

// Переводит дату из формата ДД.ММ.ГГГГ в ГГГГММДД для корректного сравнения их как обычные строки
string parseToISO(string dateStr) {
    dateStr.erase(0, dateStr.find_first_not_of(" \t"));
    if (dateStr.find("нет данных") != string::npos || dateStr.length() < 10) {
        return ISO_MAX;
    }
    return dateStr.substr(6, 4) + dateStr.substr(3, 2) + dateStr.substr(0, 2);
}

// Читаем один блок между дефисами и сразу бьем его на части (адреса, письма и т.д.)
Organization readNext(ifstream& file) {
    Organization org;
    org.isEmpty = true;
    string line;

    while (getline(file, line)) {
        if (line.find(BLOCK_SEP) != string::npos) break; // Нашли конец блока

        org.isEmpty = false;
        if (line.find("Название") != string::npos) org.nameLine = line;
        else if (line.find("Адрес:") != string::npos) org.addresses.push_back(line);
        else if (line.find("Фамилия") != string::npos) org.directors.push_back(line);
        else if (line.find("Корреспонденция") != string::npos) org.corrHeader = line;
        else if (line.find("- Вид:") != string::npos) org.docs.push_back(line);
    }
    return org;
}

// Достает чистое значение нужного поля для сортировки
// Достает чистое значение нужного поля для сортировки
vector<string> extractValues(const Organization& org, SortCriteria criteria) {
    vector<string> results;

    switch (criteria) {
    case BY_NAME: {
        size_t p = org.nameLine.find(":");
        if (p != string::npos) results.push_back(org.nameLine.substr(p + 2));
        break;
    }
    case BY_ADDR: {
        for (const auto& a : org.addresses) {
            size_t p = a.find(":");
            if (p != string::npos) results.push_back(a.substr(p + 2));
        }
        break;
    }
    case BY_DIR: {
        for (const auto& d : org.directors) {
            size_t p = d.find(":");
            if (p != string::npos) results.push_back(d.substr(p + 2));
        }
        break;
    }
    case BY_TYPE: {
        for (const auto& doc : org.docs) {
            size_t p = doc.find("- Вид:");
            size_t end = doc.find(',');
            if (p != string::npos) {
                results.push_back(doc.substr(p + 6, (end == string::npos ? doc.length() : end) - (p + 6)));
            }
        }
        break;
    }
    case BY_DATE: {
        for (const auto& doc : org.docs) {
            size_t p = doc.find("Дата:");
            if (p != string::npos) results.push_back(parseToISO(doc.substr(p + 5)));
        }
        break;
    }
    }

    // Убираем лишние пробелы по краям
    for (auto& str : results) {
        str.erase(0, str.find_first_not_of(" \t"));
        str.erase(str.find_last_not_of(" \r\n\t") + 1);
    }

    // Выкидываем дубликаты
    sort(results.begin(), results.end());
    results.erase(unique(results.begin(), results.end()), results.end());

    // Техническая заглушка для пустых данных
    if (results.empty()) {
        if (criteria == BY_DATE) results.push_back(ISO_MAX);
        else results.push_back("нет данных");
    }

    return results;
}


// ПОДФУНКЦИИ СОРТИРОВКИ 

// Первый проход - ищет следующее по списку значение (ключ группы)
bool findNextKey(const string& inputFile, SortCriteria criteria, bool isAscending, const string& lastValue, bool isFirstPass, string& outBestValue) {
    ifstream fileIn(inputFile);
    bool found = false;

    while (fileIn.peek() != EOF) {
        Organization org = readNext(fileIn);
        if (org.isEmpty) continue;

        for (const auto& value : extractValues(org, criteria)) {
            // Проверяем, обрабатывали ли мы это значение на прошлых шагах
            bool isNew = isFirstPass || (isAscending ? value > lastValue : value < lastValue);

            if (isNew) {
                if (!found) {
                    outBestValue = value;
                    found = true;
                }
                else {
                    // Ищем экстремум среди оставшихся новых значений
                    if (isAscending && value < outBestValue) outBestValue = value;
                    if (!isAscending && value > outBestValue) outBestValue = value;
                }
            }
        }
    }
    return found; // Вернет false, если мы дошли до конца и всё отсортировали
}

// Второй проход - записывает в файл все организации, у которых есть найденный ключ
void writeGroup(const string& inputFile, ofstream& resFile, SortCriteria criteria, const string& currentKey) {
    ifstream fileIn(inputFile);

    while (fileIn.peek() != EOF) {
        Organization org = readNext(fileIn);
        if (org.isEmpty) continue;

        vector<string> orgVals = extractValues(org, criteria);

        // Если в этой организации есть нужный нам ключ
        if (find(orgVals.begin(), orgVals.end(), currentKey) != orgVals.end()) {
            resFile << org.nameLine << "\n";

            // Фильтруем адреса, чтобы не выводить чужие
            vector<string> printedAddrs;
            for (const auto& a : org.addresses) {
                if (criteria != BY_ADDR || extractValues({ "", {a}, {}, "", {}, false }, BY_ADDR)[0] == currentKey) {
                    if (find(printedAddrs.begin(), printedAddrs.end(), a) == printedAddrs.end()) {
                        resFile << a << "\n";
                        printedAddrs.push_back(a);
                    }
                }
            }

            // Фильтруем директоров
            vector<string> printedDirs;
            for (const auto& d : org.directors) {
                if (criteria != BY_DIR || extractValues({ "", {}, {d}, "", {}, false }, BY_DIR)[0] == currentKey) {
                    if (find(printedDirs.begin(), printedDirs.end(), d) == printedDirs.end()) {
                        resFile << d << "\n";
                        printedDirs.push_back(d);
                    }
                }
            }

            // Оставляем только нужные письма
            if (!org.docs.empty()) {
                resFile << org.corrHeader << "\n";
                for (const auto& doc : org.docs) {
                    Organization tempDocOrg; tempDocOrg.docs.push_back(doc);
                    bool passType = (criteria != BY_TYPE || extractValues(tempDocOrg, BY_TYPE)[0] == currentKey);
                    bool passDate = (criteria != BY_DATE || extractValues(tempDocOrg, BY_DATE)[0] == currentKey);

                    if (passType && passDate) resFile << doc << "\n";
                }
            }
            resFile << BLOCK_SEP << "\n\n";
        }
    }
}

// Запускает процесс сортировки в два прохода - findNextKey и writeGroup
void performSelectionSort(const string& inputFile, const string& outputFile, SortCriteria criteria, bool isAscending) {
    ofstream out(outputFile);
    out << "============================================================\n";
    out << " ОТЧЕТ СОРТИРОВКИ ДАННЫХ\n";
    out << " Критерий: " << getCriteriaName(criteria) << "\n";
    out << " Порядок : " << (isAscending ? "По возрастанию" : "По убыванию") << "\n";
    out << "============================================================\n\n";
    out.close();

    string lastValue = "";
    bool isFirstPass = true;
    cout << "Выполняется сортировка" << endl;

    // Крутим цикл, пока findNextKey находит новые элементы
    string currentKey;
    while (findNextKey(inputFile, criteria, isAscending, lastValue, isFirstPass, currentKey)) {

        ofstream resFile(outputFile, ios::app);

        // Возвращаем дату из 20240115 обратно в 15.01.2024
        string displayHeader = currentKey;
        if (criteria == BY_DATE && displayHeader == ISO_MAX) displayHeader = "нет данных";
        else if (criteria == BY_DATE && displayHeader.length() == 8) {
            displayHeader = displayHeader.substr(6, 2) + "." + displayHeader.substr(4, 2) + "." + displayHeader.substr(0, 4);
        }

        resFile << "============================================================\n";
        resFile << " ЗНАЧЕНИЕ ПОЛЯ: " << displayHeader << "\n";
        resFile << "============================================================\n";

        // Вызываем функцию записи всей группы в файл
        writeGroup(inputFile, resFile, criteria, currentKey);

        resFile.close();
        lastValue = currentKey;
        isFirstPass = false;
        cout << "."; // Показываем, что программа не зависла
    }
    cout << "\nЗапись сортировки успешно завершена.\n";
}

// Выводит файл-инструкцию в консоль
void printInstructions(const string& helpFile) {
    ifstream file(helpFile);
    if (!file) {
        cout << "\n [!] Инструкция (" << helpFile << ") не найдена в папке с программой.\n";
        return;
    }
    string line;
    cout << "\n------------------------------------------------------------\n";
    while (getline(file, line)) {
        cout << " " << line << "\n";
    }
    cout << "------------------------------------------------------------\n";
}