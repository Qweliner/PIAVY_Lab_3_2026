//main.cpp
#include "FileSorter.h"
#include "UI.h"
#include <iostream>
#include <conio.h>
#include <windows.h>

using namespace std;

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    while (true) {
        system("cls");
        Greeting();

        int m = _getch();
        if (m == 27) break;

        switch (m) {
        case '2':
            system("cls");
            printInstructions("instructions.txt");
            cout << "\n Нажмите любую клавишу...";
            _getch();
            break;

        case '1': {
            int step = 1;
            string inP, outP; SortCriteria cr; bool asc;

            while (step > 0 && step < 5) {
                switch (step) {
                case 1: step = SelectFileStep(inP); break;
                case 2: step = SelectCriteriaStep(inP, cr); break;
                case 3: step = SelectOrderStep(asc); break;
                case 4: step = FinalizeOutputStep(inP, cr, asc); break;
                }
            }
            break;
        }
        }
    }
    return 0;
}