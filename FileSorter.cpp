#include "FileSorter.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <windows.h>

bool isValidFilename(const std::string& f) {
    if (f.empty()) return false;
    return f.find_first_of("<>:\"/\\|?*") == std::string::npos;
}

bool fileExists(const std::string& f) {
    DWORD a = GetFileAttributesA(f.c_str());
    return (a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY));
}

std::string findSmartInputFile() {
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA("*.txt", &fd);
    std::string fallback = "";
    if (h != INVALID_HANDLE_VALUE) {
        do {
            std::string n = fd.cFileName;
            if (n.find("instr") != std::string::npos) continue;
            if (n.find("тчет") != std::string::npos || n.find("Тчет") != std::string::npos) {
                FindClose(h); return n;
            }
            if (fallback.empty()) fallback = n;
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    return fallback;
}

std::string getIndexedName(const std::string& base) {
    std::string finalName = base + ".txt";
    if (!fileExists(finalName)) return finalName;
    int i = 1;
    while (true) {
        std::string test = base + "(" + std::to_string(i) + ").txt";
        if (!fileExists(test)) return test;
        i++;
    }
}

std::string generateOutputFilename(const std::string& in, SortCriteria c, bool a) {
    std::string s;
    switch (c) {
    case BY_NAME: s = "_Имя"; break;
    case BY_ADDR: s = "_Адрес"; break;
    case BY_DIR:  s = "_Дир";  break;
    case BY_TYPE: s = "_Вид"; break;
    case BY_DATE: s = "_Дата"; break;
    }
    s += (a ? "_Возр" : "_Убыв");
    size_t d = in.find_last_of('.');
    return (d == std::string::npos ? in + s : in.substr(0, d) + s);
}

std::string parseToISO(std::string d) {
    d.erase(0, d.find_first_not_of(" \t"));
    if (d.length() < 10) return ISO_MAX;
    return d.substr(6, 4) + d.substr(3, 2) + d.substr(0, 2);
}

Organization readNext(std::ifstream& f) {
    Organization o; o.isEmpty = true;
    std::string l; std::stringstream ss;
    while (std::getline(f, l)) {
        ss << l << "\n"; o.isEmpty = false;
        if (l.find(BLOCK_SEP) != std::string::npos) break;
    }
    o.rawBlock = ss.str();
    return o;
}

std::vector<std::string> extractValues(const std::string& b, SortCriteria c) {
    std::vector<std::string> r;
    std::stringstream ss(b); std::string l;
    while (std::getline(ss, l)) {
        size_t p = std::string::npos;
        if (c == BY_NAME && (p = l.find("Название организации:")) != std::string::npos) r.push_back(l.substr(p + 22));
        else if (c == BY_ADDR && (p = l.find("Адрес:")) != std::string::npos) r.push_back(l.substr(p + 7));
        else if (c == BY_DIR && (p = l.find("Фамилия руководителя:")) != std::string::npos) r.push_back(l.substr(p + 22));
        else if (c == BY_TYPE && (p = l.find("- Вид:")) != std::string::npos) {
            size_t e = l.find(',', p);
            r.push_back(l.substr(p + 7, (e == std::string::npos ? l.length() : e) - (p + 7)));
        }
        else if (c == BY_DATE && (p = l.find("Дата:")) != std::string::npos) r.push_back(parseToISO(l.substr(p + 6)));
    }
    for (auto& s : r) {
        s.erase(0, s.find_first_not_of(" \t"));
        s.erase(s.find_last_not_of(" \r\n\t") + 1);
    }
    std::sort(r.begin(), r.end());
    r.erase(std::unique(r.begin(), r.end()), r.end());
    return r;
}

void performGroupingSort(const std::string& in, const std::string& out, SortCriteria crit, bool asc) {
    std::ofstream(out).close();
    std::string last = ""; bool first = true;
    while (true) {
        std::ifstream f(in); std::string best = ""; bool found = false;
        while (f.peek() != EOF) {
            Organization o = readNext(f); if (o.isEmpty) continue;
            for (auto& v : extractValues(o.rawBlock, crit)) {
                bool isNew = first || (asc ? v > last : v < last);
                if (isNew && (!found || (asc ? v < best : v > best))) { best = v; found = true; }
            }
        }
        f.close();
        if (!found) break;
        std::ofstream res(out, std::ios::app);
        std::string h = best;
        if (crit == BY_DATE && h.length() == 8) h = h.substr(6, 2) + "." + h.substr(4, 2) + "." + h.substr(0, 4);
        res << "============================================================\n";
        res << " ГРУППА: " << h << "\n";
        res << "============================================================\n";
        f.open(in);
        while (f.peek() != EOF) {
            Organization o = readNext(f); if (o.isEmpty) continue;
            auto vs = extractValues(o.rawBlock, crit);
            if (std::find(vs.begin(), vs.end(), best) != vs.end()) res << o.rawBlock << "\n";
        }
        res.close(); f.close();
        last = best; first = false; std::cout << ".";
    }
    std::cout << "\nЗапись успешно завершена.\n";
}

void printInstructions(const std::string& p) {
    std::ifstream f(p);
    if (!f) 
    { 
        std::cout << "\n ! Инструкция (instructions.txt) не найдена.\n";
        std::cout << " Пожалуйста, положите текстовый файл инструкции в папку с приложением\n";
        return;
    }
    std::string l;
    std::cout << "\n------------------------------------------------------------\n";
    while (std::getline(f, l)) std::cout << " " << l << "\n";
    std::cout << "------------------------------------------------------------\n";
}