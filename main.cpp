#include "FileSorter.h"
#include <iostream>
#include <conio.h>
#include <windows.h>

using namespace std;

enum Flow { SUCCESS, BACK };

Flow safeInput(string& b, string p) {
    cout << p << b;
    while (true) {
        int c = _getch();
        if (c == 0 || c == 224) { if (_kbhit()) { _getch(); continue; } }
        if (c == 27) return BACK;
        if (c == 13) { if (b.empty()) continue; cout << endl; return SUCCESS; }
        if (c == 8) { if (!b.empty()) { b.pop_back(); cout << "\b \b"; } }
        else if (c >= 32 && c <= 255) {
            if (string("<>:\"/\\|?*").find((char)c) != string::npos) continue;
            b += (char)c; cout << (char)c;
        }
    }
}

int main() {
    SetConsoleCP(1251); SetConsoleOutputCP(1251);
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

        int m = _getch();
        if (m == 27) break;
        if (m == '2') { system("cls"); printInstructions("instructions.txt"); cout << "\n Нажмите любую клавишу..."; _getch(); continue; }
        if (m == '1') {
            int step = 1;
            string inP = "", outP = "";
            SortCriteria cr = BY_NAME;
            bool asc = true;

            while (step > 0 && step <= 4) {
                system("cls");
                if (step == 1) cout << " [ ESC: Назад в главное меню ]\n";
                else cout << " [ ESC: Назад на один шаг ]\n";
                cout << "------------------------------------------------------------\n";

                switch (step) {
                case 1: {
                    string d = findSmartInputFile();
                    if (!d.empty()) {
                        cout << " Обнаружен файл: " << d << "\n Использовать его? (Y - да, любой другой ввод - нет): ";
                        string a = "Y"; if (safeInput(a, "") == BACK) { step = 0; break; }
                        if (a == "Y" || a == "y" || a == "Да" || a == "да") { inP = d; step = 2; break; }
                    }
                    cout << " ! Вводите только название (без .txt).\n";
                    string man = "";
                    if (safeInput(man, " Имя входного файла: ") == BACK) { step = 0; break; }
                    inP = man + ".txt";
                    if (!fileExists(inP)) { cout << "\n Ошибка: файл не найден. [Enter]"; while (_getch() != 13); break; }
                    step = 2; break;
                }
                case 2: {
                    cout << " Файл: " << inP << "\n Поле группировки:\n";
                    cout << " 1. Имя\n 2. Адрес\n 3. Руководитель\n 4. Вид корреспонденции\n 5. Дата\n";
                    int s = _getch();
                    if (s == 27) { step = 1; break; }
                    if (s < '1' || s > '5') break;
                    cr = (SortCriteria)(s - '1'); step = 3; break;
                }
                case 3: {
                    cout << " Порядок:\n 1. А-Я (0-9)\n 2. Я-А (9-0)\n";
                    int d = _getch();
                    if (d == 27) { step = 2; break; }
                    if (d != '1' && d != '2') break;
                    asc = (d == '1'); step = 4; break;
                }
                case 4: {
                    string oB = generateOutputFilename(inP, cr, asc);
                    string oF = getIndexedName(oB);
                    cout << " ! Вводите только название (без .txt).\n";
                    size_t dot = oF.find_last_of('.');
                    string mO = (dot == string::npos) ? oF : oF.substr(0, dot);
                    if (safeInput(mO, " Имя выходного файла: ") == BACK) { step = 3; break; }
                    performGroupingSort(inP, mO + ".txt", cr, asc);
                    cout << "\n Нажмите любую клавишу..."; _getch(); step = 5; break;
                }
                }
            }
        }
    }
    return 0;
}