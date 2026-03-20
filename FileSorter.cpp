#include "FileSorter.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <windows.h> 

bool isValidFilename(const std::string& filename) {
    if (filename.empty()) return false;
    return filename.find_first_of("<>:\"/\\|?*") == std::string::npos;
}

// Надежная проверка: если поток открылся, значит файл 100% есть и доступен
bool fileExists(const std::string& filename) {
    std::ifstream file(filename.c_str());
    return file.is_open();
}

std::vector<std::string> getAvailableFiles() {
    std::vector<std::string> files;
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA("*.txt", &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string name = findData.cFileName;
            // Скрываем инструкцию из списка файлов для обработки
            if (name.find("instr") == std::string::npos) {
                files.push_back(name);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    return files;
}

std::string getIndexedName(const std::string& baseName) {
    std::string finalName = baseName + ".txt";
    if (!fileExists(finalName)) return finalName;
    int index = 1;
    // Крутим цикл, пока не найдем свободную цифру в скобках
    while (true) {
        std::string testName = baseName + "(" + std::to_string(index) + ").txt";
        if (!fileExists(testName)) return testName;
        index++;
    }
}

std::string generateOutputFilename(const std::string& inputName, SortCriteria criteria, bool isAscending) {
    std::string suffix;
    switch (criteria) {
    case BY_NAME: suffix = "_Имя"; break;
    case BY_ADDR: suffix = "_Адрес"; break;
    case BY_DIR:  suffix = "_Директор";  break;
    case BY_TYPE: suffix = "_ТипПисьма"; break;
    case BY_DATE: suffix = "_Дата"; break;
    }

    // Добавляем маркер направления через тернарник
    suffix += (isAscending ? "_Возр" : "_Убыв");
    size_t dotPos = inputName.find_last_of('.');

    // Если точки нет (ввели без .txt) лепим суффикс в конец. Если есть - врезаемся перед точкой.
    return (dotPos == std::string::npos ? inputName + suffix : inputName.substr(0, dotPos) + suffix);
}

std::string parseToISO(std::string dateStr) {
    dateStr.erase(0, dateStr.find_first_not_of(" \t")); // Чистим ведущие пробелы

    // Защита от кривых дат. Если данных нет или строка короче 10 символов - даем максимум.
    // Зачем? Чтобы при сортировке по возрастанию битые даты всегда падали в самый конец отчета.
    if (dateStr.find("нет данных") != std::string::npos || dateStr.length() < 10) {
        return ISO_MAX;
    }
    // Пересобираем ДД.ММ.ГГГГ -> ГГГГММДД
    return dateStr.substr(6, 4) + dateStr.substr(3, 2) + dateStr.substr(0, 2);
}

Organization readNext(std::ifstream& file) {
    Organization org;
    org.isEmpty = true;
    std::string line;

    while (std::getline(file, line)) {
        if (line.find(BLOCK_SEP) != std::string::npos) break; // Уперлись в дефисы - отдаем готовый блок

        org.isEmpty = false;
        // Распихиваем строки по массивам, чтобы потом было легко фильтровать
        if (line.find("Название") != std::string::npos) org.nameLine = line;
        else if (line.find("Адрес:") != std::string::npos) org.addresses.push_back(line);
        else if (line.find("Фамилия") != std::string::npos) org.directors.push_back(line);
        else if (line.find("Корреспонденция") != std::string::npos) org.corrHeader = line;
        else if (line.find("- Вид:") != std::string::npos) org.docs.push_back(line);
    }
    return org;
}

std::vector<std::string> extractValues(const Organization& org, SortCriteria criteria) {
    std::vector<std::string> results;

    // Тут просто дергаем нужные куски строк (сдвигаем индекс на длину слова, типа "Адрес: ")
    if (criteria == BY_NAME) {
        size_t p = org.nameLine.find(":");
        if (p != std::string::npos) results.push_back(org.nameLine.substr(p + 2));
    }
    else if (criteria == BY_ADDR) {
        for (const auto& a : org.addresses) {
            size_t p = a.find(":");
            if (p != std::string::npos) results.push_back(a.substr(p + 2));
        }
    }
    else if (criteria == BY_DIR) {
        for (const auto& d : org.directors) {
            size_t p = d.find(":");
            if (p != std::string::npos) results.push_back(d.substr(p + 2));
        }
    }
    else if (criteria == BY_TYPE) {
        for (const auto& doc : org.docs) {
            size_t p = doc.find("- Вид:");
            size_t end = doc.find(',');
            if (p != std::string::npos) {
                // Жесткий тернарник: если запятой нет (end == npos), берем подстроку до самого конца (doc.length()).
                // Иначе отрезаем кусок строго до запятой.
                results.push_back(doc.substr(p + 6, (end == std::string::npos ? doc.length() : end) - (p + 6)));
            }
        }
    }
    else if (criteria == BY_DATE) {
        for (const auto& doc : org.docs) {
            size_t p = doc.find("Дата:");
            if (p != std::string::npos) results.push_back(parseToISO(doc.substr(p + 5)));
        }
    }

    // Тримминг (обрезка пробелов) по краям
    for (auto& str : results) {
        str.erase(0, str.find_first_not_of(" \t"));
        str.erase(str.find_last_not_of(" \r\n\t") + 1);
    }

    // Схлопываем дубликаты. Зачем? Если в организации 2 письма от 15.01.2024, 
    // нам нужно вернуть эту дату только 1 раз, чтобы не выводить всю организацию дважды под одной датой.
    std::sort(results.begin(), results.end());
    results.erase(std::unique(results.begin(), results.end()), results.end());

    // Если организация пустая по этому критерию (вообще нет писем, например)
    if (results.empty()) {
        if (criteria == BY_DATE) results.push_back(ISO_MAX);
        else results.push_back("нет данных");
    }

    return results;
}

std::string getCriteriaName(SortCriteria c) {
    if (c == BY_NAME) return "Название организации";
    if (c == BY_ADDR) return "Адрес";
    if (c == BY_DIR) return "Руководитель";
    if (c == BY_TYPE) return "Вид корреспонденции";
    return "Дата";
}

void performSelectionSort(const std::string& inputFile, const std::string& outputFile, SortCriteria criteria, bool isAscending) {
    std::ofstream out(outputFile);
    out << "============================================================\n";
    out << " ОТЧЕТ СОРТИРОВКИ ДАННЫХ\n";
    out << " Критерий: " << getCriteriaName(criteria) << "\n";
    // Тернарник: просто подставляем нужное слово в шапку файла
    out << " Порядок : " << (isAscending ? "По возрастанию" : "По убыванию") << "\n";
    out << "============================================================\n\n";
    out.close();

    std::string lastProcessedValue = "";
    bool isFirstPass = true;

    std::cout << "Выполняется сортировка" << std::endl;

    while (true) {
        std::ifstream fileIn(inputFile);
        std::string currentMinMax = "";
        bool isCandidateFound = false;

        // --- ПРОХОД 1: Ищем следующий ключ для группы ---
        while (fileIn.peek() != EOF) {
            Organization org = readNext(fileIn);
            if (org.isEmpty) continue;

            for (const auto& value : extractValues(org, criteria)) {
                // Хитрый тернарник для проверки "подходит ли нам это значение".
                // Если мы сортируем по возрастанию, ищем всё, что строго больше прошлого значения.
                // Если по убыванию - строго меньше.
                // Для первого прохода берем всё (isFirstPass == true).
                bool isNew = isFirstPass || (isAscending ? value > lastProcessedValue : value < lastProcessedValue);

                if (isNew) {
                    if (!isCandidateFound) {
                        currentMinMax = value;
                        isCandidateFound = true;
                    }
                    else {
                        // Обновляем текущий экстремум (ищем самый минимум из оставшихся)
                        if (isAscending && value < currentMinMax) currentMinMax = value;
                        if (!isAscending && value > currentMinMax) currentMinMax = value;
                    }
                }
            }
        }
        fileIn.close();

        // Не нашли новых кандидатов - сворачиваем лавочку
        if (!isCandidateFound) break;

        // --- ПРОХОД 2: Пишем в файл все блоки, где встретился этот ключ ---
        std::ofstream resFile(outputFile, std::ios::app);
        std::string displayHeader = currentMinMax;

        // Костыль для дат: переводим системные "99999999" обратно в нормальный вид
        if (criteria == BY_DATE && displayHeader == ISO_MAX) displayHeader = "нет данных";
        else if (criteria == BY_DATE && displayHeader.length() == 8) {
            displayHeader = displayHeader.substr(6, 2) + "." + displayHeader.substr(4, 2) + "." + displayHeader.substr(0, 4);
        }

        resFile << "============================================================\n";
        resFile << " ЗНАЧЕНИЕ ПОЛЯ: " << displayHeader << "\n";
        resFile << "============================================================\n";

        fileIn.open(inputFile);
        while (fileIn.peek() != EOF) {
            Organization org = readNext(fileIn);
            if (org.isEmpty) continue;

            std::vector<std::string> orgVals = extractValues(org, criteria);

            // Если в списке ключей текущей организации есть наш искомый ключ
            if (std::find(orgVals.begin(), orgVals.end(), currentMinMax) != orgVals.end()) {

                resFile << org.nameLine << "\n";

                // Выписываем адреса. Если сортировали не по адресу - выводим все. 
                // Если по адресу - выводим только тот, который совпал с заголовком группы.
                for (const auto& a : org.addresses) {
                    if (criteria != BY_ADDR || extractValues({ "", {a}, {}, "", {}, false }, BY_ADDR)[0] == currentMinMax) {
                        resFile << a << "\n";
                    }
                }

                for (const auto& d : org.directors) {
                    if (criteria != BY_DIR || extractValues({ "", {}, {d}, "", {}, false }, BY_DIR)[0] == currentMinMax) {
                        resFile << d << "\n";
                    }
                }

                if (!org.docs.empty()) {
                    resFile << org.corrHeader << "\n";
                    for (const auto& doc : org.docs) {
                        // Собираем фиктивную организацию из 1 письма, чтобы прогнать через наш же парсер
                        Organization tempDocOrg; tempDocOrg.docs.push_back(doc);

                        bool passType = (criteria != BY_TYPE || extractValues(tempDocOrg, BY_TYPE)[0] == currentMinMax);
                        bool passDate = (criteria != BY_DATE || extractValues(tempDocOrg, BY_DATE)[0] == currentMinMax);

                        if (passType && passDate) {
                            resFile << doc << "\n";
                        }
                    }
                }
                resFile << BLOCK_SEP << "\n\n";
            }
        }
        resFile.close();
        fileIn.close();

        lastProcessedValue = currentMinMax;
        isFirstPass = false;
        std::cout << ".";
    }
    std::cout << "\n[УСПЕХ] Запись сортировки успешно завершена.\n";
}

void printInstructions(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cout << "\n ! Инструкция не найдена.\n";
        return;
    }
    std::string line;
    std::cout << "\n------------------------------------------------------------\n";
    while (std::getline(file, line)) std::cout << " " << line << "\n";
    std::cout << "------------------------------------------------------------\n";
}