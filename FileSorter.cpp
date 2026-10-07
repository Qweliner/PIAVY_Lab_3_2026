#include "FileSorter.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <windows.h>

using namespace std;

// Вход: строка с предполагаемым именем файла.
// Что делает: проверяет, нет ли внутри запрещенных для Windows символов.
// Выход: true, если имя безопасное, иначе false.
bool isValidFilename(const string& filename) {
    if (filename.empty()) return false;
    return filename.find_first_of("<>:\"/\\|?*") == string::npos;
}

// Вход: строка с именем файла.
// Что делает: пытается открыть файл для чтения, проверяя его наличие.
// Выход: true, если файл существует и доступен, иначе false.
bool fileExists(const string& filename) {
    ifstream file(filename.c_str());
    return file.is_open();
}

// Вход: нет.
// Что делает: сканирует директорию программы, ищет файлы с расширением txt кроме инструкции.
// Выход: вектор с найденными именами файлов.
vector<string> getAvailableFiles() {
    vector<string> files;
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA("*.txt", &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            string name = findData.cFileName;
            if (name.find("instr") == string::npos) {
                files.push_back(name);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    return files;
}

// Вход: базовое имя файла.
// Что делает: подбирает свободное имя с числовым индексом в скобках.
// Выход: свободное имя файла.
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

// Вход: исходное имя файла, выбранный критерий и порядок сортировки.
// Что делает: формирует имя результирующего файла с суффиксами параметров.
// Выход: строка с новым именем файла.
string generateOutputFilename(const string& inputName, SortCriteria criteria, bool isAscending) {
    string suffix;
    switch (criteria) {
    case BY_NAME: suffix = "_Имя"; break;
    case BY_ADDR: suffix = "_Адрес"; break;
    case BY_DIR:  suffix = "_Директор"; break;
    case BY_TYPE: suffix = "_ТипПисьма"; break;
    case BY_DATE: suffix = "_Дата"; break;
    }
    suffix += (isAscending ? "_Возр" : "_Убыв");

    size_t dotPos = inputName.find_last_of('.');
    return (dotPos == string::npos ? inputName + suffix : inputName.substr(0, dotPos) + suffix);
}

// Вход: путь к файлу инструкции.
// Что делает: читает файл инструкции и выводит его содержимое в консоль.
// Выход: нет.
void printInstructions(const string& path) {
    ifstream file(path);
    if (!file) {
        cout << "\n ! Инструкция не найдена. Положите instructions.txt в ту же папку что и исполняемая программа.\n";
        return;
    }
    string line;
    cout << "\n------------------------------------------------------------\n";
    while (getline(file, line)) cout << " " << line << "\n";
    cout << "------------------------------------------------------------\n";
}

// Вход: критерий сортировки из перечисления.
// Что делает: возвращает текстовое название поля на русском языке.
// Выход: строка с названием критерия.
string getCriteriaName(SortCriteria c) {
    if (c == BY_NAME) return "Название организации";
    if (c == BY_ADDR) return "Адрес";
    if (c == BY_DIR) return "Руководитель";
    if (c == BY_TYPE) return "Вид корреспонденции";
    return "Дата";
}

// Вход: сырая строка с датой, направление сортировки.
// Что делает: преобразует дату в формат ГГГГММДД, пустые даты заменяет сторожевыми значениями.
// Выход: строка для лексикографического сравнения дат.
string parseToISO(string dateStr, bool isAscending) {
    dateStr.erase(0, dateStr.find_first_not_of(" \t"));

    if (dateStr.find("нет данных") != string::npos || dateStr.length() < 10) {
        return isAscending ? ISO_MAX : ISO_MIN;
    }
    return dateStr.substr(6, 4) + dateStr.substr(3, 2) + dateStr.substr(0, 2);
}

// Вход: открытый поток файла.
// Что делает: считывает строки до разделителя из 20 дефисов в структуру Organization.
// Выход: заполненная структура Organization.
Organization readNext(ifstream& file) {
    Organization org;
    org.isEmpty = true;
    string line;

    while (getline(file, line)) {
        if (line.empty() ||
            line.find("======") != string::npos ||
            line.find("ОТЧЕТ СОРТИРОВКИ") != string::npos ||
            line.find("Критерий:") != string::npos ||
            line.find("Порядок :") != string::npos ||
            line.find("ЗНАЧЕНИЕ ПОЛЯ:") != string::npos) {
            continue;
        }

        if (line.find(BLOCK_SEP) != string::npos) {
            if (!org.isEmpty) break;
            else continue;
        }

        org.isEmpty = false;
        if (line.find("Название") != string::npos) org.nameLine = line;
        else if (line.find("Адрес:") != string::npos) org.addresses.push_back(line);
        else if (line.find("Фамилия") != string::npos) org.directors.push_back(line);
        else if (line.find("Корреспонденция") != string::npos) org.corrHeader = line;
        else if (line.find("- Вид:") != string::npos) org.docs.push_back(line);
    }
    return org;
}

// Вход: структура организации, критерий сортировки, направление сортировки.
// Что делает: извлекает чистые значения выбранного поля из структуры организации.
// Выход: вектор строковых ключей.
vector<string> extractValues(const Organization& org, SortCriteria criteria, bool isAscending) {
    vector<string> results;
    switch (criteria) {
    case BY_NAME: {
        size_t p = org.nameLine.find(":");
        if (p != string::npos && p + 2 <= org.nameLine.length())
            results.push_back(org.nameLine.substr(p + 2));
        break;
    }
    case BY_ADDR: {
        for (size_t i = 0; i < org.addresses.size(); i++) {
            const string& a = org.addresses[i];
            size_t p = a.find(":");
            if (p != string::npos && p + 2 <= a.length())
                results.push_back(a.substr(p + 2));
        }
        break;
    }
    case BY_DIR: {
        for (size_t i = 0; i < org.directors.size(); i++) {
            const string& d = org.directors[i];
            size_t p = d.find(":");
            if (p != string::npos && p + 2 <= d.length())
                results.push_back(d.substr(p + 2));
        }
        break;
    }
    case BY_TYPE: {
        for (size_t i = 0; i < org.docs.size(); i++) {
            const string& doc = org.docs[i];
            size_t p = doc.find("- Вид:");
            size_t end = doc.find(',');
            if (p != string::npos) {
                results.push_back(doc.substr(p + 6, (end == string::npos ? doc.length() : end) - (p + 6)));
            }
        }
        break;
    }
    case BY_DATE: {
        for (size_t i = 0; i < org.docs.size(); i++) {
            const string& doc = org.docs[i];
            size_t p = doc.find("Дата:");
            if (p != string::npos && p + 5 <= doc.length())
                results.push_back(parseToISO(doc.substr(p + 5), isAscending));
        }
        break;
    }
    }

    for (size_t i = 0; i < results.size(); i++) {
        string& str = results[i];
        str.erase(0, str.find_first_not_of(" \t"));
        str.erase(str.find_last_not_of(" \r\n\t") + 1);
    }

    sort(results.begin(), results.end());
    results.erase(unique(results.begin(), results.end()), results.end());

    if (results.empty()) {
        if (criteria == BY_DATE) results.push_back(isAscending ? ISO_MAX : ISO_MIN);
        else results.push_back("нет данных");
    }
    return results;
}

// Вход: история напечатанных строк, строка для проверки.
// Что делает: выполняет поиск строки в списке уже выведенных.
// Выход: true, если строка уже выводилась, иначе false.
bool isAlreadyPrinted(const vector<string>& printedHistory, const string& stringToCheck) {
    for (size_t i = 0; i < printedHistory.size(); i++) {
        if (printedHistory[i] == stringToCheck) return true;
    }
    return false;
}

// Вход: поток записи, массив строк, активный критерий, целевой критерий, ключ группы.
// Что делает: выводит только те адреса или руководителей, которые соответствуют ключу группы.
// Выход: нет.
void writeUniqueFilteredOrganizationInfo(ofstream& resFile, const vector<string>& lines, SortCriteria activeCriteria, SortCriteria targetCriteria, const string& currentKey) {
    vector<string> printedHistory;

    for (size_t i = 0; i < lines.size(); i++) {
        string line = lines[i];
        bool shouldPrint = false;

        if (activeCriteria != targetCriteria) {
            shouldPrint = true;
        }
        else {
            size_t p = line.find(":");
            string cleanStr = (p != string::npos && p + 2 <= line.length()) ? line.substr(p + 2) : "";
            cleanStr.erase(0, cleanStr.find_first_not_of(" \t"));
            cleanStr.erase(cleanStr.find_last_not_of(" \r\n\t") + 1);

            if (cleanStr == currentKey) shouldPrint = true;
        }

        if (shouldPrint && !isAlreadyPrinted(printedHistory, line)) {
            resFile << line << "\n";
            printedHistory.push_back(line);
        }
    }
}

// Вход: поток записи, организация, критерий, ключ группы, направление сортировки.
// Что делает: записывает строки документов, соответствующие выбранному ключу.
// Выход: нет.
void writeFilteredDocs(ofstream& resFile, const Organization& org, SortCriteria activeCriteria, const string& currentKey, bool isAscending) {
    if (org.docs.empty()) return;
    resFile << org.corrHeader << "\n";
    for (size_t i = 0; i < org.docs.size(); i++) {
        string docLine = org.docs[i];
        bool shouldPrint = true;
        if (activeCriteria == BY_TYPE) {
            size_t p = docLine.find("- Вид:");
            size_t end = docLine.find(',');
            string typeVal = (p != string::npos) ? docLine.substr(p + 6, (end == string::npos ? docLine.length() : end) - (p + 6)) : "";
            typeVal.erase(0, typeVal.find_first_not_of(" \t"));
            typeVal.erase(typeVal.find_last_not_of(" \r\n\t") + 1);
            if (typeVal != currentKey) shouldPrint = false;
        }
        else if (activeCriteria == BY_DATE) {
            size_t p = docLine.find("Дата:");
            string dateVal = (p != string::npos && p + 5 <= docLine.length()) ? parseToISO(docLine.substr(p + 5), isAscending) : (isAscending ? ISO_MAX : ISO_MIN);
            if (dateVal != currentKey) shouldPrint = false;
        }
        if (shouldPrint) {
            resFile << docLine << "\n";
        }
    }
}

// Вход: имя файла для чтения.
// Что делает: считывает все блоки организаций из файла в оперативную память.
// Выход: вектор структур Organization.
vector<Organization> loadAllOrganizations(const string& filename) {
    vector<Organization> orgs;
    ifstream file(filename.c_str());
    if (!file.is_open()) return orgs;

    while (file.peek() != EOF) {
        Organization org = readNext(file);
        if (!org.isEmpty) {
            orgs.push_back(org);
        }
    }
    file.close();
    return orgs;
}

// Вход: список организаций в памяти, выбранный критерий, направление сортировки.
// Что делает: извлекает все неповторяющиеся ключи для группировки данных.
// Выход: вектор уникальных строк-ключей.
vector<string> collectUniqueKeys(const vector<Organization>& orgs, SortCriteria criteria, bool isAscending) {
    vector<string> keys;
    for (size_t i = 0; i < orgs.size(); i++) {
        vector<string> blockKeys = extractValues(orgs[i], criteria, isAscending);
        for (size_t j = 0; j < blockKeys.size(); j++) {
            if (find(keys.begin(), keys.end(), blockKeys[j]) == keys.end()) {
                keys.push_back(blockKeys[j]);
            }
        }
    }
    return keys;
}

// Вход: массив ключей по ссылке, порядок сортировки.
// Что делает: сортирует массив ключей в оперативной памяти методом простого выбора (Selection Sort).
// Выход: нет, сортируется исходный массив .
void selectionSort(vector<string>& keys, bool isAscending) {
    cout << " Выполняется сортировка методом выбора" << endl;
    for (size_t i = 0; i < keys.size(); i++) {
        size_t targetIndex = i;
        for (size_t j = i + 1; j < keys.size(); j++) {
            if (isAscending) {
                if (keys[j] < keys[targetIndex]) {
                    targetIndex = j;
                }
            }
            else {
                if (keys[j] > keys[targetIndex]) {
                    targetIndex = j;
                }
            }
        }
        if (targetIndex != i) {
            swap(keys[i], keys[targetIndex]);
        }
        cout << ".";
    }
    cout << "\n";
}

// Вход: ключ группы, критерий сортировки.
// Что делает: преобразует технические заглушки и дату ISO обратно в понятный текст.
// Выход: отформатированная строка для заголовка блока.
string formatDisplayKey(const string& key, SortCriteria criteria) {
    if (criteria == BY_DATE) {
        if (key == ISO_MAX || key == ISO_MIN) {
            return "нет данных";
        }
        if (key.length() == 8) {
            return key.substr(6, 2) + "." + key.substr(4, 2) + "." + key.substr(0, 4);
        }
    }
    return key;
}

// Вход: вектор организаций из памяти, поток файла для записи, критерий, направление, текущий ключ.
// Что делает: находит в памяти и выгружает в файл блоки, соответствующие ключу группы.
// Выход: нет.
void writeGroup(const vector<Organization>& orgs, ofstream& resFile, SortCriteria criteria, bool isAscending, const string& currentKey) {
    for (size_t i = 0; i < orgs.size(); i++) {
        const Organization& org = orgs[i];
        vector<string> orgVals = extractValues(org, criteria, isAscending);

        if (find(orgVals.begin(), orgVals.end(), currentKey) != orgVals.end()) {
            resFile << org.nameLine << "\n";
            writeUniqueFilteredOrganizationInfo(resFile, org.addresses, criteria, BY_ADDR, currentKey);
            writeUniqueFilteredOrganizationInfo(resFile, org.directors, criteria, BY_DIR, currentKey);
            writeFilteredDocs(resFile, org, criteria, currentKey, isAscending);
            resFile << "\n\n";
        }
    }
}

// Вход: пути к входному и выходному файлам, критерий сортировки, порядок сортировки.
// Что делает: считывает данные в ОЗУ, сортирует методом выбора в памяти и сохраняет итоговый отчет.
// Выход: нет.
void performSelectionSort(const string& inputFile, const string& outputFile, SortCriteria criteria, bool isAscending) {
    vector<Organization> orgs = loadAllOrganizations(inputFile);
    if (orgs.empty()) {
        cout << "\nОшибка: входной файл пуст или не найден.\n";
        return;
    }

    vector<string> keys = collectUniqueKeys(orgs, criteria, isAscending);

    selectionSort(keys, isAscending);

    ofstream out(outputFile.c_str());
    if (!out.is_open()) {
        cout << "\nОшибка создания выходного файла.\n";
        return;
    }

    out << "ОТЧЕТ СОРТИРОВКИ ДАННЫХ\n";
    out << "Критерий: " << getCriteriaName(criteria) << "\n";
    out << "Порядок : " << (isAscending ? "По возрастанию" : "По убыванию") << "\n\n";

    for (size_t i = 0; i < keys.size(); i++) {
        string displayHeader = formatDisplayKey(keys[i], criteria);
        out << " ЗНАЧЕНИЕ ПОЛЯ: " << displayHeader << "\n";
        writeGroup(orgs, out, criteria, isAscending, keys[i]);
    }

    out.close();
    cout << " Запись отсортированных данных успешно завершена.\n";
}