#include "FileSorter.h"
#include <iostream>
#include <conio.h>
#include <windows.h>

using namespace std;

// Состояния для выхода из меню
enum FlowState { SUCCESS, BACK };

FlowState safeInput(string& buffer, string prompt) {
    cout << prompt << buffer;
    while (true) {
        int keycode = _getch();

        // Игнорирование служебных клавиш (стрелки, F1-F12)
        if (keycode == 0 || keycode == 224) {
            if (_kbhit()) { _getch(); continue; }
        }

        if (keycode == 27) return BACK; // Нажат ESC

        if (keycode == 13) { // Нажат Enter
            if (buffer.empty()) continue;
            cout << endl;
            return SUCCESS;
        }

        if (keycode == 8) { // Нажат Backspace
            if (!buffer.empty()) {
                buffer.pop_back();
                cout << "\b \b";
            }
        }
        else if (keycode >= 32 && keycode <= 255) {
            // Защита от системных символов Windows
            if (string("<>:\"/\\|?*").find((char)keycode) != string::npos) continue;
            buffer += (char)keycode;
            cout << (char)keycode;
        }
    }
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    while (true) {
        system("cls");
        cout << "============================================================\n";
        cout << "              ОБРАБОТКА И ГРУППИРОВКА ОТЧЕТОВ               \n";
        cout << "============================================================\n";
        cout << " Пример формата:\n";
        cout << " Название организации: ООО Вектор\n";
        cout << " Адрес: г. Москва, ул. Тверская, д. 10\n";
        cout << " Фамилия руководителя: Иванов И.И.\n";
        cout << " Корреспонденция:\n";
        cout << " \t- Вид: Письмо, Дата: 15.01.2024\n";
        cout << " --------------------\n";
        cout << "============================================================\n";
        cout << " 1. Начать выполнение\n 2. Инструкция\n ESC. Выход\n";
        cout << "------------------------------------------------------------\n";

        int menuKey = _getch();
        if (menuKey == 27) break;
        if (menuKey == '2') {
            system("cls");
            printInstructions("instructions.txt");
            cout << "\n Нажмите любую клавишу..."; _getch();
            continue;
        }

        if (menuKey == '1') {
            int currentStep = 1;
            string inputPath = "", outputPath = "";
            SortCriteria sortCriteria = BY_NAME;
            bool isAscending = true;

            // Машина состояний
            while (currentStep > 0 && currentStep <= 4) {
                system("cls");
                if (currentStep == 1) cout << " [ ESC: Назад в главное меню ]\n";
                else cout << "[ ESC: Назад на один шаг ]\n";
                cout << "------------------------------------------------------------\n";

                switch (currentStep) {
                case 1: { // ВЫБОР ФАЙЛА
                    string autoFoundFile = findSmartInputFile();
                    if (!autoFoundFile.empty()) {
                        cout << " Обнаружен файл: " << autoFoundFile << "\n Использовать его? (Y - да, любой другой ввод - нет): ";
                        string answer = "Y";
                        if (safeInput(answer, "") == BACK) { currentStep = 0; break; }

                        if (answer == "Y" || answer == "y" || answer == "Да" || answer == "да") {
                            inputPath = autoFoundFile;
                            currentStep = 2;
                            break;
                        }
                    }
                    cout << " ! Вводите только название (без .txt).\n";
                    string manualInput = "";
                    if (safeInput(manualInput, " Имя входного файла: ") == BACK) { currentStep = 0; break; }

                    inputPath = manualInput + ".txt";
                    if (!fileExists(inputPath)) {
                        cout << "\n Ошибка: файл не найден. [Enter]";
                        while (_getch() != 13);
                        break;
                    }
                    currentStep = 2;
                    break;
                }
                case 2: { // ВЫБОР КРИТЕРИЯ
                    cout << " Файл: " << inputPath << "\n Поле группировки:\n";
                    cout << " 1. Имя\n 2. Адрес\n 3. Руководитель\n 4. Вид корр.\n 5. Дата\n";

                    int key = _getch();
                    if (key == 27) { currentStep = 1; break; }
                    if (key < '1' || key > '5') break;

                    sortCriteria = (SortCriteria)(key - '1');
                    currentStep = 3;
                    break;
                }
                case 3: { // ВЫБОР НАПРАВЛЕНИЯ
                    cout << " Порядок:\n 1. А-Я (0-9)\n 2. Я-А (9-0)\n";

                    int key = _getch();
                    if (key == 27) { currentStep = 2; break; }
                    if (key != '1' && key != '2') break;

                    isAscending = (key == '1');
                    currentStep = 4;
                    break;
                }
                case 4: { // СОХРАНЕНИЕ
                    // Генерируем базовое имя (например Отчет_Имя_Возр)
                    string autoBaseName = generateOutputFilename(inputPath, sortCriteria, isAscending);
                    string suggestedName = autoBaseName;

                    // ПРОВЕРКА ДУБЛИКАТОВ И УВЕДОМЛЕНИЕ
                    if (fileExists(autoBaseName + ".txt")) {
                        suggestedName = getIndexedName(autoBaseName); // Получит Отчет_Имя_Возр(1)
                        cout << " ! Исходный файл " << autoBaseName << ".txt уже существует.\n";
                        cout << " По умолчанию будет предложено безопасное имя с индексом.\n\n";
                    }

                    cout << " ! Вводите только название (без .txt).\n";

                    // Предлагаем пользователю имя (с индексом или без)
                    string manualOutput = suggestedName;
                    if (safeInput(manualOutput, " Имя выходного файла: ") == BACK) { currentStep = 3; break; }

                    string finalPath = manualOutput + ".txt";

                    // ЛОГИКА ПЕРЕЗАПИСИ (Если пользователь стер индекс или ввел другое существующее имя)
                    if (fileExists(finalPath)) {
                        cout << "\n ВНИМАНИЕ Файл " << finalPath << " уже существует!\n";
                        cout << " Перезаписать его? (Y - да, любой другой ввод - нет): ";
                        string overwriteAns = "Y";
                        if (safeInput(overwriteAns, "") == BACK) { currentStep = 3; break; }

                        // Если пользователь передумал перезаписывать - возвращаем его на стадию 4
                        if (!(overwriteAns == "Y" || overwriteAns == "y" || overwriteAns == "Да" || overwriteAns == "да")) {
                            break;
                        }
                    }

                    performGroupingSort(inputPath, finalPath, sortCriteria, isAscending);
                    cout << "\n Нажмите любую клавишу...";
                    _getch();
                    currentStep = 5;
                    break;
                }
                }
            }
        }
    }
    return 0;
}