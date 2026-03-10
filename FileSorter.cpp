#include "FileSorter.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <windows.h> // Оставлен только для поиска файлов в папке (требование отсутствия путей)

// Проверка на запрещенные символы ОС (защита от краша)
bool isValidFilename(const std::string& filename) {
    if (filename.empty()) return false;
    return filename.find_first_of("<>:\"/\\|?*") == std::string::npos;
}

// Стандартная C++ проверка существования файла (открываем и смотрим, открылся ли)
bool fileExists(const std::string& filename) {
    std::ifstream file(filename.c_str());
    return file.is_open();
}

// Собирает все .txt файлы в текущей папке для вывода списком
std::vector<std::string> getAvailableFiles() {
    std::vector<std::string> files;
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA("*.txt", &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string name = findData.cFileName;
            // Инструкцию прячем из списка, чтобы не мешалась
            if (name.find("instr") == std::string::npos) {
                files.push_back(name);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    return files;
}

// Защита от перезаписи: если файл есть, делаем Имя(1).txt, Имя(2).txt
std::string getIndexedName(const std::string& baseName) {
    std::string finalName = baseName + ".txt";
    if (!fileExists(finalName)) return finalName;
    int index = 1;
    while (true) {
        std::string testName = baseName + "(" + std::to_string(index) + ").txt";
        if (!fileExists(testName)) return testName;
        index++;
    }
}

// Автогенерация имени файла
std::string generateOutputFilename(const std::string& inputName, SortCriteria criteria, bool isAscending) {
    std::string suffix;
    switch (criteria) {
    case BY_NAME: suffix = "_Имя"; break;
    case BY_ADDR: suffix = "_Адрес"; break;
    case BY_DIR:  suffix = "_Директор";  break;
    case BY_TYPE: suffix = "_ТипПисьма"; break;
    case BY_DATE: suffix = "_Дата"; break;
    }
    suffix += (isAscending ? "_Возр" : "_Убыв");
    size_t dotPos = inputName.find_last_of('.');
    return (dotPos == std::string::npos ? inputName + suffix : inputName.substr(0, dotPos) + suffix);
}

// Исправленный баг с датой!
// Переводим "15.01.2024" -> "20240115", чтобы строки сравнивались как числа
std::string parseToISO(std::string dateStr) {
    dateStr.erase(0, dateStr.find_first_not_of(" \t"));

    // Если дата отсутствует, заменяем на искусственный якорь
    if (dateStr.find("нет данных") != std::string::npos || dateStr.length() < 10) {
        return ISO_MAX; // Уйдет в конец списка
    }
    return dateStr.substr(6, 4) + dateStr.substr(3, 2) + dateStr.substr(0, 2);
}

// Чтение файла по блокам. Блок разбивается на векторы, чтобы при сортировке
// можно было выкинуть лишние письма или адреса (правка препода №6).
Organization readNext(std::ifstream& file) {
    Organization org;
    org.isEmpty = true;
    std::string line;

    while (std::getline(file, line)) {
        if (line.find(BLOCK_SEP) != std::string::npos) break; // Конец блока

        org.isEmpty = false;
        if (line.find("Название") != std::string::npos) org.nameLine = line;
        else if (line.find("Адрес:") != std::string::npos) org.addresses.push_back(line);
        else if (line.find("Фамилия") != std::string::npos) org.directors.push_back(line);
        else if (line.find("Корреспонденция") != std::string::npos) org.corrHeader = line;
        else if (line.find("- Вид:") != std::string::npos) org.docs.push_back(line);
    }
    return org;
}

// Вытаскивает "чистые" значения из массива строк для компаратора сортировки
std::vector<std::string> extractValues(const Organization& org, SortCriteria criteria) {
    std::vector<std::string> results;

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

    // Чистим пробелы по краям
    for (auto& str : results) {
        str.erase(0, str.find_first_not_of(" \t"));
        str.erase(str.find_last_not_of(" \r\n\t") + 1);
    }
    // Оставляем только уникальные значения внутри одного блока
    std::sort(results.begin(), results.end());
    results.erase(std::unique(results.begin(), results.end()), results.end());

    // Если по критерию у организации ничего нет (массив пуст)
    if (results.empty()) {
        if (criteria == BY_DATE) results.push_back(ISO_MAX);
        else results.push_back("нет данных");
    }

    return results;
}

// Красивое форматирование шапки выходного файла
std::string getCriteriaName(SortCriteria c) {
    if (c == BY_NAME) return "Название организации";
    if (c == BY_ADDR) return "Адрес";
    if (c == BY_DIR) return "Руководитель";
    if (c == BY_TYPE) return "Вид корреспонденции";
    return "Дата";
}

// ---------------------------------------------------------
// ГЛАВНЫЙ УЗЕЛ ЛОГИКИ: ВНЕШНЯЯ СОРТИРОВКА ВЫБОРОМ
// Сложность ОЗУ: O(1). Ограничения на размер файла: отсутствуют.
// ---------------------------------------------------------
void performSelectionSort(const std::string& inputFile, const std::string& outputFile, SortCriteria criteria, bool isAscending) {
    std::ofstream out(outputFile);

    // Пишем шапку файла (правка препода №4)
    out << "============================================================\n";
    out << " ОТЧЕТ СОРТИРОВКИ ДАННЫХ\n";
    out << " Критерий: " << getCriteriaName(criteria) << "\n";
    out << " Порядок : " << (isAscending ? "По возрастанию (А-Я, Старые-Новые)" : "По убыванию (Я-А, Новые-Старые)") << "\n";
    out << "============================================================\n\n";
    out.close();

    std::string lastProcessedValue = "";
    bool isFirstPass = true;

    std::cout << "Выполняется сортировка (без загрузки в ОЗУ)..." << std::endl;

    while (true) {
        std::ifstream fileIn(inputFile);
        std::string currentMinMax = "";
        bool isCandidateFound = false;

        // ПРОХОД 1 (SELECTION): Читаем файл от и до. 
        // Ищем наименьший/наибольший ключ, который мы ЕЩЕ НЕ обрабатывали на прошлых кругах.
        while (fileIn.peek() != EOF) {
            Organization org = readNext(fileIn);
            if (org.isEmpty) continue;

            for (const auto& value : extractValues(org, criteria)) {
                // Если мы сортируем по возрастанию, новое значение должно быть строго БОЛЬШЕ обработанного ранее.
                bool isNew = isFirstPass || (isAscending ? value > lastProcessedValue : value < lastProcessedValue);

                if (isNew) {
                    if (!isCandidateFound) {
                        currentMinMax = value;
                        isCandidateFound = true;
                    }
                    else {
                        // Классический поиск экстремума из оставшихся
                        if (isAscending && value < currentMinMax) currentMinMax = value;
                        if (!isAscending && value > currentMinMax) currentMinMax = value;
                    }
                }
            }
        }
        fileIn.close();

        // Если новых значений больше нет - сортировка завершена
        if (!isCandidateFound) break;

        // ПРОХОД 2 (WRITE): Снова открываем файл.
        // Записываем все блоки, в которых есть найденный на первом шаге ключ.
        std::ofstream resFile(outputFile, std::ios::app);

        std::string displayHeader = currentMinMax;
        if (criteria == BY_DATE && displayHeader == ISO_MAX) displayHeader = "нет данных";
        else if (criteria == BY_DATE && displayHeader.length() == 8) {
            // Переводим 20240115 обратно в 15.01.2024 для заголовка
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

            // Проверяем, есть ли текущий ключ в этой организации
            if (std::find(orgVals.begin(), orgVals.end(), currentMinMax) != orgVals.end()) {

                // ПРАВКА ПРЕПОДА №6: Фильтрация лишних данных перед печатью!
                // Печатаем название
                resFile << org.nameLine << "\n";

                // Печатаем адреса (если фильтр не по адресу - выводим все. Иначе - только совпавший)
                for (const auto& a : org.addresses) {
                    if (criteria != BY_ADDR || extractValues({ "", {a}, {}, "", {}, false }, BY_ADDR)[0] == currentMinMax) {
                        resFile << a << "\n";
                    }
                }

                // Печатаем директоров
                for (const auto& d : org.directors) {
                    if (criteria != BY_DIR || extractValues({ "", {}, {d}, "", {}, false }, BY_DIR)[0] == currentMinMax) {
                        resFile << d << "\n";
                    }
                }

                // Печатаем письма
                if (!org.docs.empty()) {
                    resFile << org.corrHeader << "\n";
                    for (const auto& doc : org.docs) {
                        // Создаем мини-организацию из одного письма, чтобы проверить его ключ
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
        std::cout << "."; // Индикатор, что цикл крутится
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