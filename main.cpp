#include "FileSorter.h"
#include <iostream>
#include <conio.h>
#include <windows.h>

using namespace std;

// Перечисление состояний для безопасного ввода
enum FlowState { SUCCESS, BACK };

FlowState safeInput(string& buffer, string prompt) {
    cout << prompt << buffer;
    while (true) {
        int keycode = _getch();

        // Отсекаем стрелки (начинаются с 0 или 224). _kbhit() проверяет, есть ли хвост у стрелки.
        if (keycode == 0 || keycode == 224) {
            if (_kbhit()) { _getch(); continue; }
        }

        if (keycode == 27) return BACK; // Нажат ESC

        if (keycode == 13) {
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
        cout << "              СИСТЕМА СОРТИРОВКИ ОТЧЕТНОСТИ                 \n";
        cout << "============================================================\n";
        cout << " 1. Начать выполнение сортировки\n";
        cout << " 2. Инструкция пользователя\n";
        cout << " ESC. Выход из программы\n";
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

            // Конечный автомат для удобной отмены действий по ESC
            while (currentStep > 0 && currentStep <= 4) {
                system("cls");
                if (currentStep == 1) cout << " [ ESC: Возврат в главное меню ]\n";
                else cout << "[ ESC: Вернуться на предыдущий шаг ]\n";
                cout << "------------------------------------------------------------\n";

                switch (currentStep) {
                case 1: {
                    // ПРАВКА ПРЕПОДА №7: Меню выбора файлов
                    vector<string> files = getAvailableFiles();
                    if (!files.empty()) {
                        cout << " Найденные текстовые файлы в папке:\n";
                        for (size_t i = 0; i < files.size(); ++i) {
                            cout << " " << i + 1 << ". " << files[i] << "\n";
                        }
                        cout << " 0. Ввести имя файла вручную\n\n";
                        cout << " Выберите пункт: ";

                        int fileChoice = _getch();
                        if (fileChoice == 27) { currentStep = 0; break; }

                        int idx = fileChoice - '0';
                        if (idx > 0 && idx <= files.size()) {
                            inputPath = files[idx - 1];
                            currentStep = 2;
                            break;
                        }
                        else if (idx != 0) {
                            break; // Нажата левая кнопка, обновляем экран
                        }
                    }

                    cout << "\n ! Вводите только название (без .txt).\n";
                    string manualInput = "";
                    if (safeInput(manualInput, " Имя входного файла: ") == BACK) { currentStep = 0; break; }

                    inputPath = manualInput + ".txt";
                    if (!fileExists(inputPath)) {
                        cout << "\n Ошибка: файл не найден. [Enter для повтора]";
                        while (_getch() != 13);
                        break;
                    }
                    currentStep = 2;
                    break;
                }
                case 2: {
                    cout << " Файл: " << inputPath << "\n\n Выберите поле сортировки:\n";
                    cout << " 1. Название организации\n 2. Адрес\n 3. Руководитель\n 4. Вид корреспонденции\n 5. Дата\n";

                    int key = _getch();
                    if (key == 27) { currentStep = 1; break; }
                    if (key < '1' || key > '5') break;

                    sortCriteria = (SortCriteria)(key - '1');
                    currentStep = 3;
                    break;
                }
                case 3: {
                    cout << " Направление сортировки:\n";
                    cout << " 1. По возрастанию (А -> Я, Старые -> Новые)\n";
                    cout << " 2. По убыванию (Я -> А, Новые -> Старые)\n";

                    int key = _getch();
                    if (key == 27) { currentStep = 2; break; }
                    if (key != '1' && key != '2') break;

                    isAscending = (key == '1');
                    currentStep = 4;
                    break;
                }
                case 4: {
                    string autoBaseName = generateOutputFilename(inputPath, sortCriteria, isAscending);
                    string suggestedName = autoBaseName;

                    if (fileExists(autoBaseName + ".txt")) {
                        suggestedName = getIndexedName(autoBaseName);
                        cout << "[!] Файл " << autoBaseName << ".txt уже существует.\n";
                        cout << " Будет предложено безопасное имя с индексом.\n\n";
                    }

                    cout << " ! Вводите только название (без .txt).\n";

                    string manualOutput = suggestedName;
                    if (safeInput(manualOutput, " Имя выходного файла: ") == BACK) { currentStep = 3; break; }

                    string finalPath = manualOutput + ".txt";

                    if (fileExists(finalPath)) {
                        cout << "\n [ВНИМАНИЕ] Файл " << finalPath << " уже существует!\n";
                        cout << " Перезаписать его? (Y - да, любой другой ввод - нет): ";
                        string overwriteAns = "Y";
                        if (safeInput(overwriteAns, "") == BACK) { currentStep = 3; break; }

                        if (!(overwriteAns == "Y" || overwriteAns == "y" || overwriteAns == "Да" || overwriteAns == "да")) {
                            break;
                        }
                    }

                    cout << "\n\n";
                    performSelectionSort(inputPath, finalPath, sortCriteria, isAscending);
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