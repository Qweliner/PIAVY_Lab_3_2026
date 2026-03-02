#include "FileSorter.h"
#include <iostream>
#include <conio.h>
#include <windows.h>

using namespace std;

// Сигнальные коды возврата для машины состояний
enum Flow { SUCCESS, BACK, EXIT };

Flow safeInput(string& b, string p) {
    cout << p << b;
    while (true) {
        int c = _getch();
        if (c == 0 || c == 224) { if (_kbhit()) { _getch(); continue; } }
        if (c == 27) return BACK;
        if (c == 13) { if (b.empty()) continue; cout << endl; return SUCCESS; }
        if (c == 8) { if (!b.empty()) { b.pop_back(); cout << "\b \b"; } }
        else if (c >= 32 && c <= 255) {
            string s(1, (char)c); if (isValidFilename(s)) { b += (char)c; cout << (char)c; }
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
        cout << " 1. Начать выполнение\n";
        cout << " 2. Инструкция\n";
        cout << " ESC. Выход\n";
        cout << "------------------------------------------------------------\n";
        int m = _getch();
        if (m == 27) break;
        if (m == '2') { system("cls"); printInstructions("instructions.txt"); cout << "\n Нажмите любую клавишу..."; _getch(); continue; }
        if (m == '1') {
            int step = 1;
            string inPath = "", outPath = "";
            SortCriteria crit = BY_NAME;
            bool asc = true;

            while (step > 0 && step <= 4) {
                switch (step) {
                case 1: { // ШАГ 1: Входной файл
                    system("cls"); string det = findSmartInputFile();
                    if (!det.empty()) {
                        cout << " Чтобы вернуться назад, нажмите ESC\n\n Обнаружен файл: " << det << "\n Использовать его? (Y - да, любой другой ввод - нет): ";
                        string a = "Y"; Flow res = safeInput(a, "");
                        if (res == BACK) { step = 0; break; }
                        if (a == "Y" || a == "y" || a == "Да" || a == "да") { inPath = det; step = 2; break; }
                    }
                    string manual = ""; cout << " Вводите только название. ";
                    Flow res = safeInput(manual, " Имя входного файла: ");
                    if (res == BACK) { step = 0; break; }
                    inPath = manual + ".txt";
                    if (!fileExists(inPath)) { cout << "\n Ошибка: файл не найден. [Enter]"; while (_getch() != 13); break; }
                    step = 2; break;
                }
                case 2: { // ШАГ 2: Критерий
                    system("cls"); cout << " Чтобы вернуться назад, нажмите ESC\n\n Файл: " << inPath << "\n Выберите поле :\n";
                    cout << " 1. Имя\n 2. Адрес\n 3. Руководитель\n 4. Тип\n 5. Дата\n";
                    int s = _getch();
                    if (s == 27) { step = 1; break; }
                    if (s < '1' || s > '5') break;
                    crit = (SortCriteria)(s - '1'); step = 3; break;
                }
                case 3: { // ШАГ 3: Направление
                    system("cls"); cout << " Чтобы вернуться назад, нажмите ESC\n\n Порядок :\n 1. А-Я\n 2. Я-А\n";
                    int d = _getch();
                    if (d == 27) { step = 2; break; }
                    if (d != '1' && d != '2') break;
                    asc = (d == '1'); step = 4; break;
                }
                case 4: { // ШАГ 4: Выходной файл и старт
                    system("cls");
                    string outBase = generateOutputFilename(inPath, crit, asc);
                    string outFull = getIndexedName(outBase);
                    cout << " Чтобы вернуться назад, нажмите ESC\n\n Вводите только название.\n";
                    size_t dot = outFull.find_last_of('.');
                    string manualOut = (dot == string::npos) ? outFull : outFull.substr(0, dot);
                    Flow res = safeInput(manualOut, " Имя выходного файла: ");
                    if (res == BACK) { step = 3; break; }
                    performGroupingSort(inPath, manualOut + ".txt", crit, asc);
                    _getch(); step = 5; break;
                }
                }
            }
        }
    }
    return 0;
}