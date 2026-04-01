#include "FileSorter.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <windows.h> 

// Отсекаем символы, которые файловая система ОС не принимает в именах файлов.
bool isValidFilename(const std::string& filename) {
    if (filename.empty()) return false;
    return filename.find_first_of("<>:\"/\\|?*") == std::string::npos;
}


// Пытаемся открыть поток. Если открылся (is_open) - файл физически существует и доступен.
bool fileExists(const std::string& filename) {
    std::ifstream file(filename.c_str());
    return file.is_open();
}

// Сканируем папку с программой через WinAPI.
// Собираем все .txt файлы, попутно игнорируя файл инструкции, чтобы выдать пользователю чистый список.
std::vector<std::string> getAvailableFiles() {
    std::vector<std::string> files;
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA("*.txt", &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string name = findData.cFileName;
            // Прячем инструкцию из списка для обработки
            if (name.find("instr") == std::string::npos) {
                files.push_back(name);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    return files;
}

// Автоматическая генерация уникального имени для защиты от перезаписи.
// Крутим бесконечный цикл, подставляя (1), (2) и т.д., пока не найдем свободное имя на диске.
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

// Формируем имя результата на основе входного имени, поля сортировки и направления.
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

// Переворачиваем строку 15.01.2024 в 20240115. Теперь их можно сравнивать лексикографически (как обычные строки).
// Пустые/битые даты жестко заменяем на 99999999 (ISO_MAX), чтобы они всегда падали в самый конец отчета.
std::string parseToISO(std::string dateStr) {
    dateStr.erase(0, dateStr.find_first_not_of(" \t"));

    if (dateStr.find("нет данных") != std::string::npos || dateStr.length() < 10) {
        return ISO_MAX;
    }
    return dateStr.substr(6, 4) + dateStr.substr(3, 2) + dateStr.substr(0, 2);
}

// Читаем из потока ровно один блок данных до линии из 20 дефисов.
// Сразу распихиваем строки по массивам (адреса, директора, письма).
// Это нужно для фильтрации при выводе чтобы отсекать лишние письма, не попадающие в целевую группу.
Organization readNext(std::ifstream& file) {
    Organization org;
    org.isEmpty = true;
    std::string line;

    while (std::getline(file, line)) {
        if (line.find(BLOCK_SEP) != std::string::npos) break; // Разделитель найден - блок считан

        org.isEmpty = false;
        if (line.find("Название") != std::string::npos) org.nameLine = line;
        else if (line.find("Адрес:") != std::string::npos) org.addresses.push_back(line);
        else if (line.find("Фамилия") != std::string::npos) org.directors.push_back(line);
        else if (line.find("Корреспонденция") != std::string::npos) org.corrHeader = line;
        else if (line.find("- Вид:") != std::string::npos) org.docs.push_back(line);
    }
    return org;
}

// Вытаскивает "чистые" значения нужного поля из структуры организации.
// Возвращает массив, так как в одном блоке может быть 5 разных дат или 2 разных адреса.
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
                // Если запятой нет, берем строку до конца. Иначе - строго до запятой.
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

    // Зачистка от лишних пробелов и символов переноса
    for (auto& str : results) {
        str.erase(0, str.find_first_not_of(" \t"));
        str.erase(str.find_last_not_of(" \r\n\t") + 1);
    }

    // Удаляем дубликаты (std::unique работает только на отсортированном контейнере).
    // Если этого не сделать, блок с двумя письмами от одной даты выведется два раза подряд.
    std::sort(results.begin(), results.end());
    results.erase(std::unique(results.begin(), results.end()), results.end());

    // Если искомых данных в блоке вообще не оказалось - ставим техническую заглушку
    if (results.empty()) {
        if (criteria == BY_DATE) results.push_back(ISO_MAX);
        else results.push_back("нет данных");
    }

    return results;
}

// Перечисление доступных полей для группировки и сортировки.
std::string getCriteriaName(SortCriteria c) {
    if (c == BY_NAME) return "Название организации";
    if (c == BY_ADDR) return "Адрес";
    if (c == BY_DIR) return "Руководитель";
    if (c == BY_TYPE) return "Вид корреспонденции";
    return "Дата";
}

// ==========================================================================================
// Внешняя сортировка выбором
// Работает в 2 прохода по файлу на каждое уникальное значение ключа.
// ==========================================================================================
void performSelectionSort(const std::string& inputFile, const std::string& outputFile, SortCriteria criteria, bool isAscending) {
    // Инициализация выходного файла и запись технической шапки
    std::ofstream out(outputFile);
    out << "============================================================\n";
    out << " ОТЧЕТ СОРТИРОВКИ ДАННЫХ\n";
    out << " Критерий: " << getCriteriaName(criteria) << "\n";
    out << " Порядок : " << (isAscending ? "По возрастанию" : "По убыванию") << "\n";
    out << "============================================================\n\n";
    out.close();

    // lastProcessedValue хранит экстремум с предыдущей итерации.
    // От него мы отталкиваемся, чтобы найти следующее значение по порядку.
    std::string lastProcessedValue = "";
    bool isFirstPass = true;

    std::cout << "Выполняется сортировка" << std::endl;

    while (true) {
        std::ifstream fileIn(inputFile);
        std::string currentMinMax = "";
        bool isCandidateFound = false;

        // ----------------------------------------------------------------------------------
        // ПРОХОД 1 (ПОИСК): Сканируем весь файл от начала до конца.
        // Задача: найти минимальный (или максимальный) ключ, который строго больше/меньше
        // того, что мы уже обработали на прошлом круге (lastProcessedValue).
        // ----------------------------------------------------------------------------------
        while (fileIn.peek() != EOF) {
            Organization org = readNext(fileIn);
            if (org.isEmpty) continue;

            for (const auto& value : extractValues(org, criteria)) {
                // Тернарный оператор: 
                // При сортировке по возрастанию кандидат должен быть СТРОГО БОЛЬШЕ прошлого ключа.
                // При убывании - СТРОГО МЕНЬШЕ. Для первого круга берем вообще все.
                bool isNew = isFirstPass || (isAscending ? value > lastProcessedValue : value < lastProcessedValue);

                if (isNew) {
                    if (!isCandidateFound) {
                        currentMinMax = value;
                        isCandidateFound = true;
                    }
                    else {
                        // Поиск экстремума из оставшихся.
                        if (isAscending && value < currentMinMax) currentMinMax = value;
                        if (!isAscending && value > currentMinMax) currentMinMax = value;
                    }
                }
            }
        }
        fileIn.close();

        // Если новых кандидатов не найдено, значит мы прошли весь файл.
        if (!isCandidateFound) break;

        // ----------------------------------------------------------------------------------
        // ПРОХОД 2 (ЗАПИСЬ): Повторное сканирование файла с самого начала.
        // Задача: выбрать все блоки, в которых присутствует найденный на 1-м проходе ключ (currentMinMax).
        // Дополнительно: отфильтровать лишние данные внутри самого блока и подавить дубликаты.
        // ----------------------------------------------------------------------------------
        std::ofstream resFile(outputFile, std::ios::app); // Открываем на дозапись (append)

        std::string displayHeader = currentMinMax;
        // Возврат системного ISO_MAX и дат ISO формата обратно в человекочитаемый вид для вывода
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

            // Проверка принадлежности организации к текущей группе (найденному ключу)
            if (std::find(orgVals.begin(), orgVals.end(), currentMinMax) != orgVals.end()) {

                resFile << org.nameLine << "\n";

                // Входной файл может содержать дублирующиеся строки адресов (при нескольких руководителях).
                // Вектор printedAddrs выполняет роль кэша выведенных строк для предотвращения повторов.
                std::vector<std::string> printedAddrs;
                for (const auto& a : org.addresses) {
                    // Выводим только если фильтр не по адресу, ИЛИ адрес совпадает с ключом группы
                    if (criteria != BY_ADDR || extractValues({ "", {a}, {}, "", {}, false }, BY_ADDR)[0] == currentMinMax) {
                        // Проверка на наличие дубликата в кэше вывода
                        if (std::find(printedAddrs.begin(), printedAddrs.end(), a) == printedAddrs.end()) {
                            resFile << a << "\n";
                            printedAddrs.push_back(a);
                        }
                    }
                }

                // Селективный вывод директоров с подавлением дубликатов по аналогии с адресами.
                std::vector<std::string> printedDirs;
                for (const auto& d : org.directors) {
                    if (criteria != BY_DIR || extractValues({ "", {}, {d}, "", {}, false }, BY_DIR)[0] == currentMinMax) {
                        if (std::find(printedDirs.begin(), printedDirs.end(), d) == printedDirs.end()) {
                            resFile << d << "\n";
                            printedDirs.push_back(d);
                        }
                    }
                }

                // Селективный вывод писем. Оставляем только те, которые подошли по Дате или Типу.
                if (!org.docs.empty()) {
                    resFile << org.corrHeader << "\n";
                    for (const auto& doc : org.docs) {
                        // Изоляция строки документа в отдельную структуру для прогона через парсер
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

        // Сохранение ключа для следующей итерации поиска
        lastProcessedValue = currentMinMax;
        isFirstPass = false;
        std::cout << "."; // Индикация выполнения процесса для больших файлов
    }
    std::cout << "\nЗапись сортировки успешно завершена.\n";
}

void printInstructions(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cout << "\n ! Инструкция не найдена. Положите файл instructions.txt в папку с программой\n";
        return;
    }
    std::string line;
    std::cout << "\n------------------------------------------------------------\n";
    while (std::getline(file, line)) std::cout << " " << line << "\n";
    std::cout << "------------------------------------------------------------\n";
}