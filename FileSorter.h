//FileSorter.h
#pragma once
#include <string>
#include <vector>
#include <fstream>

// Разделитель для блоков данных в файле
const std::string BLOCK_SEP = "--------------------";

// Заглушки для пустых или битых дат. 
// Помогают выкидывать пустые записи в самый конец отчета.
const std::string ISO_MIN = "00000000";
const std::string ISO_MAX = "99999999";

// Поля, по которым мы умеем искать группы
enum SortCriteria { BY_NAME, BY_ADDR, BY_DIR, BY_TYPE, BY_DATE };

// Структура для удобного хранения данных внутри одного блока.
// Мы сразу бьем текст на массивы, чтобы потом было легко выкидывать из них лишнее.
struct Organization {
    std::string nameLine;
    std::vector<std::string> addresses;
    std::vector<std::string> directors;
    std::string corrHeader;
    std::vector<std::string> docs;
    bool isEmpty;
};


// ФУНКЦИИ ДЛЯ РАБОТЫ С ФАЙЛАМИ И ИМЕНАМИ

// Вход: строка с предполагаемым именем файла.
// Что делает: проверяет, нет ли внутри запрещенных для Windows символов (< > : " / \ | ? *).
// Выход: true, если имя безопасное, false если содержит мусор.
bool isValidFilename(const std::string& filename);

// Вход: строка с именем файла.
// Что делает: пытается открыть файл для чтения. Это надежный способ проверить его наличие.
// Выход: true, если файл существует и доступен, иначе false.
bool fileExists(const std::string& filename);

// Вход: нет.
// Что делает: сканирует папку с программой, ищет текстовые файлы (кроме инструкции).
// Выход: массив (вектор) с именами найденных .txt файлов.
std::vector<std::string> getAvailableFiles();

// Вход: базовое имя файла (например, "Отчет").
// Что делает: подбирает свободное имя, добавляя цифры в скобках, если файл уже есть (Отчет(1).txt).
// Выход: уникальное имя файла, которое точно ничего не перезапишет.
std::string getIndexedName(const std::string& baseName);

// Вход: исходное имя файла, выбранный критерий и порядок сортировки.
// Что делает: добавляет к имени файла красивые суффиксы (например, "_Дата_Возр").
// Выход: готовая строка с новым именем.
std::string generateOutputFilename(const std::string& input, SortCriteria crit, bool asc);

// Вход: путь к файлу инструкции.
// Что делает: читает файл инструкции и выводит его текст в консоль с рамками.
// Выход: нет (печать в консоль инструкции).
void printInstructions(const std::string& helpFile);

// Вход: критерий сортировки (enum).
// Что делает: переводит техническое название критерия в русский текст.
// Выход: строка (например, "Вид корреспонденции").
std::string getCriteriaName(SortCriteria c);


// ПАРСИНГ И ОБРАБОТКА ДАННЫХ

// Вход: сырая строка с датой (например, " 15.01.2024"), направление сортировки.
// Что делает: отрезает пробелы и переставляет куски местами в формат ГГГГММДД (20240115).
// Выход: строка, которую можно сравнивать как обычное число.
std::string parseToISO(std::string dateStr, bool isAscending = true);

// Вход: открытый поток чтения файла (ifstream).
// Что делает: читает строки до тех пор, пока не встретит 20 дефисов. Игнорирует системный мусор.
// Выход: заполненная структура Organization.
Organization readNext(std::ifstream& file);

// Вход: структура одной организации, критерий сортировки, направление сортировки.
// Что делает: вытаскивает из структуры только нужные чистые значения (без слов "Адрес: " и т.д.).
// Выход: массив строк с чистыми значениями для поиска ключа.
std::vector<std::string> extractValues(const Organization& org, SortCriteria criteria, bool isAscending = true);


// ЛОГИКА ВНУТРЕННЕЙ СОРТИРОВКИ ВЫБОРОМ

// Вход: история уже напечатанных строк и строка-кандидат.
// Что делает: ищет кандидата в истории. 
// Выход: true, если строка уже выводилась (дубликат), иначе false.
bool isAlreadyPrinted(const std::vector<std::string>& printedHistory, const std::string& stringToCheck);

// Вход: поток записи, список адресов/директоров, критерии и текущий ключ группы.
// Что делает: решает, выводить строку или нет. Отсекает чужие адреса/директоров и не дает печатать дубликаты.
// Выход: нет (запись напрямую в файл).
void writeUniqueFilteredOrganizationInfo(std::ofstream& resFile, const std::vector<std::string>& lines, SortCriteria activeCriteria, SortCriteria targetCriteria, const std::string& currentKey);

// Вход: поток записи, организация, критерий, текущий ключ группы, направление сортировки.
// Что делает: печатает только те письма, которые подходят под текущую выбранную дату или тип.
// Выход: нет (запись напрямую в файл).
void writeFilteredDocs(std::ofstream& resFile, const Organization& org, SortCriteria activeCriteria, const std::string& currentKey, bool isAscending = true);

// Вход: имя исходного файла базы данных.
// Что делает: читает все блоки организаций из файла и сохраняет их в оперативную память.
// Выход: вектор прочитанных структур Organization.
std::vector<Organization> loadAllOrganizations(const std::string& filename);

// Вход: список организаций в памяти, выбранный критерий и порядок сортировки.
// Что делает: собирает все уникальные ключи для группировки данных без повторений.
// Выход: массив уникальных строковых ключей.
std::vector<std::string> collectUniqueKeys(const std::vector<Organization>& orgs, SortCriteria criteria, bool isAscending);

// Вход: массив ключей (по ссылке) и направление сортировки.
// Что делает: сортирует массив ключей в памяти методом прямого выбора (Selection Sort).
// Выход: нет (сортирует переданный массив на месте).
void selectionSort(std::vector<std::string>& keys, bool isAscending);

// Вход: ключ группы и критерий сортировки.
// Что делает: переводит техническую дату ISO и заглушки обратно в понятный вид для шапки блока.
// Выход: читаемая строка для заголовка блока в файле.
std::string formatDisplayKey(const std::string& key, SortCriteria criteria);

// Вход: массив организаций из памяти, поток записи, критерий, направление и ключ текущей группы.
// Что делает: находит в памяти все организации с данным ключом и записывает их в файл отчета.
// Выход: нет (запись напрямую в файл).
void writeGroup(const std::vector<Organization>& orgs, std::ofstream& resFile, SortCriteria criteria, bool isAscending, const std::string& currentKey);

// Вход: пути к файлам, критерий и направление.
// Что делает: считывает файл в ОЗУ, запускает внутреннюю сортировку выбором и сохраняет отчет.
// Выход: нет (формирует итоговый отсортированный файл).
void performSelectionSort(const std::string& inputFile, const std::string& outputFile, SortCriteria criteria, bool isAscending);