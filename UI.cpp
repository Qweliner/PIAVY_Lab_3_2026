//UI.cpp
#include "UI.h"
#include <iostream>
#include <conio.h>
#include <windows.h>
#include <fstream>
#include <vector>

using namespace std;

// Безопасный ввод: перехватывает нажатия клавиш, блокирует запрещенные символы 
// например, для имен файлов и обрабатывает выход через ESC.
// prompt - текст перед вводом пользователя, buffer - введенное пользователем
FlowState safeInput(string& buffer, const string& prompt, bool onlyDigits) {
    cout << prompt << buffer;
    while (true) {
        int keycode = _getch();
        if (keycode == 0 || keycode == 224) { if (_kbhit()) { _getch(); continue; } }
        if (keycode == 27) return BACK;
        if (keycode == 13) { if (buffer.empty()) continue; cout << endl; return SUCCESS; }
        if (keycode == 8) { if (!buffer.empty()) { buffer.pop_back(); cout << "\b \b"; } }
        else if (keycode >= 32 && keycode <= 255) {
            if (onlyDigits && !isdigit(keycode)) continue;
            if (!onlyDigits && string("<>:\"/\\|?*").find((char)keycode) != string::npos) continue;
            buffer += (char)keycode;
            cout << (char)keycode;
        }
    }
}

// Шаг 1: Выбор входного файла из найденных или ввод вручную.
int SelectFileStep(string& inputPath) {
    system("cls");
    cout << " [ ESC: Возврат в главное меню ]\n";
    cout << "------------------------------------------------------------\n";

    vector<string> files = getAvailableFiles();
    if (!files.empty()) {
        cout << " Найденные файлы:\n";
        for (size_t i = 0; i < files.size(); ++i) cout << " " << i + 1 << ". " << files[i] << "\n";
        cout << " 0. Ввод вручную\n\n";

        string choiceStr = "";
        if (safeInput(choiceStr, " Выберите: ", true) == BACK) return 0;

        int idx = -1;
        try { idx = stoi(choiceStr); }
        catch (...) { idx = -1; }

        if (idx > 0 && idx <= files.size()) {
            inputPath = files[idx - 1];
            if (!fileExists(inputPath)) {
                cout << "\n Ошибка: файл не найден. [Enter]";
                while (_getch() != 13);
                return 1;
            }
            return 2;
        }
        if (idx != 0) {
            cout << "\n Ошибка: неверный пункт. [Enter]";
            while (_getch() != 13);
            return 1;
        }
    }

    cout << "\n ! Вводите только название (без .txt).\n";
    string manual = "";
    if (safeInput(manual, " Имя входного файла: ") == BACK) return 0;

    inputPath = manual + ".txt";
    if (!fileExists(inputPath)) {
        cout << "\n Ошибка: файл не найден. [Enter]";
        while (_getch() != 13);
        return 1;
    }
    return 2;
}

// Шаг 2: Выбор поля, по которому будем группировать данные.
int SelectCriteriaStep(const string& inputPath, SortCriteria& crit) {
    system("cls");
    cout << "[ ESC: Вернуться на предыдущий шаг ]\n";
    cout << "------------------------------------------------------------\n";
    cout << " Файл: " << inputPath << "\n\n";
    cout << " Поле сортировки:\n 1. Имя\n 2. Адрес\n 3. Руководитель\n 4. Вид корреспонденции\n 5. Дата\n";

    int key = _getch();
    if (key == 27) return 1;
    if (key < '1' || key > '5') return 2;

    crit = (SortCriteria)(key - '1');
    return 3;
}

// Шаг 3: Выбор направления сортировки (возрастание или убывание).
int SelectOrderStep(bool& asc) {
    system("cls");
    cout << "[ ESC: Вернуться на предыдущий шаг ]\n";
    cout << "------------------------------------------------------------\n";
    cout << " Порядок сортировки:\n 1. По возрастанию (А-Я, 0-9)\n 2. По убыванию (Я-А, 9-0)\n";

    int key = _getch();
    if (key == 27) return 2;
    if (key != '1' && key != '2') return 3;

    asc = (key == '1');
    return 4;
}

// Шаг 4: Генерация имени выходного файла, проверка перезаписи и запуск алгоритма.
int FinalizeOutputStep(const string& in, SortCriteria cr, bool asc) {
    system("cls");
    cout << " [ ESC: Вернуться на предыдущий шаг ]\n";
    cout << "------------------------------------------------------------\n";

    string autoBase = generateOutputFilename(in, cr, asc);
    string suggested = autoBase;

    if (fileExists(autoBase + ".txt")) {
        suggested = getIndexedName(autoBase);
        cout << " [!] Файл " << autoBase << ".txt уже существует.\n";
        cout << " Предложено безопасное имя с индексом.\n\n";
    }

    cout << " ! Вводите только название (без .txt).\n";

    size_t dot = suggested.find_last_of('.');
    string manualOut = (dot == string::npos) ? suggested : suggested.substr(0, dot);

    if (safeInput(manualOut, " Имя выходного файла: ") == BACK) return 3;

    string finalPath = manualOut + ".txt";

    if (fileExists(finalPath)) {
        cout << "\n\n [ВНИМАНИЕ] Файл " << finalPath << " уже существует!\n";
        cout << " Перезаписать? (Y - да, любой другой ввод - нет): ";
        string ans = "no";
        if (safeInput(ans, "") == BACK) return 3;
        if (!(ans == "Y" || ans == "y" || ans == "Да" || ans == "да" || ans == "1")) return 4;
    }

    cout << "\n\n";
    performSelectionSort(in, finalPath, cr, asc);
    cout << "\n Нажмите любую клавишу...";
    _getch();
    return 5;
}

// Вывод приветственного меню в консоль.
void Greeting() {
    cout << "============================================================\n";
    cout << "              СИСТЕМА СОРТИРОВКИ ОТЧЕТНОСТИ                 \n";
    cout << "============================================================\n";
    cout << " 1. Начать выполнение сортировки\n";
    cout << " 2. Инструкция пользователя\n";
    cout << " ESC. Выход из программы\n";
    cout << "------------------------------------------------------------\n";
}