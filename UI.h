//UI.h
#pragma once
#include <string>
#include "FileSorter.h"

// Сигналы для управления навигацией по меню
enum FlowState { SUCCESS, BACK };

// Безопасный ввод: перехватывает нажатия клавиш, блокирует запрещенные символы 
// например, для имен файлов и обрабатывает выход через ESC.
// prompt - текст перед вводом пользователя, buffer - введенное пользователем
FlowState safeInput(std::string& buffer, const std::string& prompt, bool onlyDigits = false);

// Шаг 1: Выбор входного файла из найденных или ввод вручную.
int SelectFileStep(std::string& inputPath);

// Шаг 2: Выбор поля, по которому будем группировать данные.
int SelectCriteriaStep(const std::string& inputPath, SortCriteria& crit);

// Шаг 3: Выбор направления сортировки (возрастание или убывание).
int SelectOrderStep(bool& asc);

// Шаг 4: Генерация имени выходного файла, проверка перезаписи и запуск алгоритма.
int FinalizeOutputStep(const std::string& inputPath, SortCriteria crit, bool asc);

// Вывод приветственного меню в консоль.
void Greeting();