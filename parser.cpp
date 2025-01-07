#include "core.hpp"
#include "parser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <set>
#include <regex>

using namespace std;

// Create an address operand based on the parsed string
Operand* createAddrOperand(string s) {
    // Regular expressions for 64-bit addresses and registers
    regex addr1("0x[[:xdigit:]]+");
    regex addr2("rax|rbx|rcx|rdx|rsi|rdi|rsp|rbp|r8|r9|r10|r11|r12|r13|r14|r15");
    regex addr3("(rax|rbx|rcx|rdx|rsi|rdi|rsp|rbp|r8|r9|r10|r11|r12|r13|r14|r15)\\*([[:digit:]])");
    regex addr4("(rax|rbx|rcx|rdx|rsi|rdi|rsp|rbp|r8|r9|r10|r11|r12|r13|r14|r15)(\\+|-)(0x[[:xdigit:]]+)");
    regex addr5("(rax|rbx|rcx|rdx|rsi|rdi|rsp|rbp|r8|r9|r10|r11|r12|r13|r14|r15)\\+(rax|rbx|rcx|rdx|rsi|rdi|rsp|rbp|r8|r9|r10|r11|r12|r13|r14|r15)\\*([[:digit:]])");
    regex addr6("(rax|rbx|rcx|rdx|rsi|rdi|rsp|rbp|r8|r9|r10|r11|r12|r13|r14|r15)\\*([[:digit:]])(\\+|-)(0x[[:xdigit:]]+)");
    regex addr7("(rax|rbx|rcx|rdx|rsi|rdi|rsp|rbp|r8|r9|r10|r11|r12|r13|r14|r15)\\+(rax|rbx|rcx|rdx|rsi|rdi|rsp|rbp|r8|r9|r10|r11|r12|r13|r14|r15)\\*([[:digit:]])(\\+|-)(0x[[:xdigit:]]+)");

    Operand* opr = new Operand();
    smatch m;

    // Match the longest sequences first
    if (regex_search(s, m, addr7)) {
        opr->ty = Operand::MEM;
        opr->tag = 7;
        opr->field[0] = m[1]; // rax
        opr->field[1] = m[2]; // rbx
        opr->field[2] = m[3]; // 2
        opr->field[3] = m[4]; // + or -
        opr->field[4] = m[5]; // 0xfffff1
    } else if (regex_search(s, m, addr5)) {
        opr->ty = Operand::MEM;
        opr->tag = 5;
        opr->field[0] = m[1]; // rax
        opr->field[1] = m[2]; // rbx
        opr->field[2] = m[3]; // 2
    } else if (regex_search(s, m, addr6)) {
        opr->ty = Operand::MEM;
        opr->tag = 6;
        opr->field[0] = m[1]; // rax
        opr->field[1] = m[2]; // 2
        opr->field[2] = m[3]; // + or -
        opr->field[3] = m[4]; // 0xfffff1
    } else if (regex_search(s, m, addr4)) {
        opr->ty = Operand::MEM;
        opr->tag = 4;
        opr->field[0] = m[1]; // rax
        opr->field[1] = m[2]; // + or -
        opr->field[2] = m[3]; // 0xfffff1
    } else if (regex_search(s, m, addr3)) {
        opr->ty = Operand::MEM;
        opr->tag = 3;
        opr->field[0] = m[1]; // rax
        opr->field[1] = m[2]; // 2
    } else if (regex_search(s, m, addr1)) {
        opr->ty = Operand::MEM;
        opr->tag = 1;
        opr->field[0] = m[0]; // 0x123456
    } else if (regex_search(s, m, addr2)) {
        opr->ty = Operand::MEM;
        opr->tag = 2;
        opr->field[0] = m[0]; // rax
    } else {
        cout << "Unknown addr operand: " << s << endl;
    }

    return opr;
}

// Create a data operand (immediate or register)
Operand* createDataOperand(string s) {
    regex immvalue("0x[[:xdigit:]]+");
    regex reg8("al|ah|bl|bh|cl|ch|dl|dh|sil|dil|bpl|spl|r8b|r9b|r10b|r11b|r12b|r13b|r14b|r15b");
    regex reg16("ax|bx|cx|dx|si|di|bp|sp|r8w|r9w|r10w|r11w|r12w|r13w|r14w|r15w");
    regex reg32("eax|ebx|ecx|edx|esi|edi|esp|ebp|r8d|r9d|r10d|r11d|r12d|r13d|r14d|r15d");
    regex reg64("rax|rbx|rcx|rdx|rsi|rdi|rsp|rbp|r8|r9|r10|r11|r12|r13|r14|r15");

    Operand* opr = new Operand();
    smatch m;
    if (regex_search(s, m, reg64)) {
        opr->ty = Operand::REG;
        opr->bit = 64;
        opr->field[0] = m[0];
    } else if (regex_search(s, m, reg32)) {
        opr->ty = Operand::REG;
        opr->bit = 32;
        opr->field[0] = m[0];
    } else if (regex_search(s, m, reg16)) {
        opr->ty = Operand::REG;
        opr->bit = 16;
        opr->field[0] = m[0];
    } else if (regex_search(s, m, reg8)) {
        opr->ty = Operand::REG;
        opr->bit = 8;
        opr->field[0] = m[0];
    } else if (regex_search(s, m, immvalue)) {
        opr->ty = Operand::IMM;
        opr->bit = 64;
        opr->field[0] = m[0];
    } else {
        cout << "Unknown data operand: " << s << endl;
    }

    return opr;
}

// Create an operand (either memory or data)
Operand* createOperand(string s) {
    regex ptr("ptr \\[(.*)\\]");
    regex byteptr("byte ptr \\[(.*)\\]");
    regex wordptr("word ptr \\[(.*)\\]");
    regex dwordptr("dword ptr \\[(.*)\\]");
    regex qwordptr("qword ptr \\[(.*)\\]");
    smatch m;

    Operand* opr;
    if (regex_search(s, m, ptr)) {
        opr = createAddrOperand(m[1]);
        opr->bit = 64;
    } else {
        opr = createDataOperand(s);
    }

    return opr;
}

// Parse operands for each instruction
void parseOperand(list<Inst>::iterator begin, list<Inst>::iterator end) {
    for (auto it = begin; it != end; ++it) {
        for (int i = 0; i < it->oprnum; ++i) {
            it->oprd[i] = createOperand(it->oprs[i]);
        }
    }
}

// Parse the trace file into a list of instructions
void parseTrace(ifstream* infile, list<Inst>* L) {
    string line;
    int num = 1;

    while (getline(*infile, line)) {
        if (line.empty()) continue;

        istringstream strbuf(line);
        string temp, disasstr;

        Inst* ins = new Inst();
        ins->id = num++;

        getline(strbuf, ins->addr, ';');
        ins->addrn = stoull(ins->addr, nullptr, 16);

        getline(strbuf, disasstr, ';');
        ins->assembly = disasstr;

        istringstream disasbuf(disasstr);
        getline(disasbuf, ins->opcstr, ' ');

        while (getline(disasbuf, temp, ',')) {
            if (!temp.empty() && temp.find_first_not_of(' ') != string::npos) {
                ins->oprs.push_back(temp);
            }
        }
        ins->oprnum = ins->oprs.size();

        for (int i = 0; i < 16; ++i) {
            getline(strbuf, temp, ',');
            ins->ctxreg[i] = stoull(temp, nullptr, 16);
        }

        getline(strbuf, temp, ',');
        ins->raddr = stoull(temp, nullptr, 16);

        getline(strbuf, temp, ',');
        ins->waddr = stoull(temp, nullptr, 16);

        L->push_back(*ins);
    }
}

// Print the first 3 instructions for debugging
void printfirst3inst(list<Inst>* L) {
    int i = 0;
    for (auto it = L->begin(); it != L->end() && i < 3; ++it, ++i) {
        cout << it->opcstr << '\t';
        for (const auto& opr : it->oprs) {
            cout << opr << '\t';
        }
        for (int i = 0; i < 16; ++i) {
            printf("%llx, ", it->ctxreg[i]);
        }
        printf("%llx, %llx,\n", it->raddr, it->waddr);
    }
}

// Print the trace in a human-readable format
void printTraceHuman(list<Inst>& L, string fname) {
    FILE* ofp = fopen(fname.c_str(), "w");
    for (auto& it : L) {
        fprintf(ofp, "%s %s  \t", it.addr.c_str(), it.assembly.c_str());
        fprintf(ofp, "(%llx, %llx)\n", it.raddr, it.waddr);
    }
    fclose(ofp);
}

// Print the trace in LLSE format
void printTraceLLSE(list<Inst>& L, string fname) {
    FILE* ofp = fopen(fname.c_str(), "w");
    for (auto& it : L) {
        fprintf(ofp, "%s;%s;", it.addr.c_str(), it.assembly.c_str());
        for (int i = 0; i < 16; ++i) {
            fprintf(ofp, "%llx,", it.ctxreg[i]);
        }
        fprintf(ofp, "%llx,%llx,\n", it.raddr, it.waddr);
    }
    fclose(ofp);
}
