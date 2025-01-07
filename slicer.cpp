#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <map>
#include <stack>
#include <vector>
#include <set>

using namespace std;

#include "core.hpp"
#include "parser.hpp"

list<Inst> instlist;

// Instructions which have no data dependency effect
set<string> skipinst = {"test", "jmp", "jz", "jbe", "jo", "jno", "js", "jns", "je", "jne",
                        "jnz", "jb", "jnae", "jc", "jnb", "jae", "jnc", "jna", "ja", "jnbe", "jl",
                        "jnge", "jge", "jnl", "jle", "jng", "jg", "jnle", "jp", "jpe", "jnp", "jpo", 
                        "jcxz", "jecxz", "ret", "cmp", "call"};

int buildParameter(list<Inst> &L)
{
    for (list<Inst>::iterator it = L.begin(); it != L.end(); ++it) {
        if (skipinst.find(it->opcstr) != skipinst.end()) continue;

        switch (it->oprnum) {
            case 0:
                break;
            case 1: {
                Operand *op0 = it->oprd[0];
                int nbyte;

                if (it->opcstr == "push") {
                    if (op0->ty == Operand::IMM) {
                        it->addsrc(Parameter::IMM, op0->field[0]);
                        AddrRange ar(it->waddr, it->waddr + 7);
                        it->adddst(Parameter::MEM, ar);
                    } else if (op0->ty == Operand::REG) {
                        it->addsrc(Parameter::REG, op0->field[0]);
                        nbyte = op0->bit / 8;
                        AddrRange ar(it->waddr, it->waddr + nbyte - 1);
                        it->adddst(Parameter::MEM, ar);
                    } else if (op0->ty == Operand::MEM) {
                        nbyte = op0->bit / 8;
                        AddrRange rar(it->raddr, it->raddr + nbyte - 1);
                        it->addsrc(Parameter::MEM, rar);
                        AddrRange war(it->waddr, it->waddr + nbyte - 1);
                        it->adddst(Parameter::MEM, war);
                    } else {
                        cout << "push error: the operand is not Imm, Reg, or Mem!" << endl;
                        return 1;
                    }
                } else if (it->opcstr == "pop") {
                    if (op0->ty == Operand::REG) {
                        nbyte = op0->bit / 8;
                        AddrRange rar(it->raddr, it->raddr + nbyte - 1);
                        it->addsrc(Parameter::MEM, rar);
                        it->adddst(Parameter::REG, op0->field[0]);
                    } else if (op0->ty == Operand::MEM) {
                        nbyte = op0->bit / 8;
                        AddrRange rar(it->raddr, it->raddr + nbyte - 1);
                        it->addsrc(Parameter::MEM, rar);
                        AddrRange war(it->waddr, it->waddr + nbyte - 1);
                        it->adddst(Parameter::MEM, war);
                    } else {
                        cout << "pop error: the operand is not Reg or Mem!" << endl;
                        return 1;
                    }
                }
                break;
            }
            case 2: {
                Operand *op0 = it->oprd[0];
                Operand *op1 = it->oprd[1];
                int nbyte;

                if (it->opcstr == "mov" || it->opcstr == "movzx") {
                    if (op0->ty == Operand::REG) {
                        if (op1->ty == Operand::IMM) {
                            it->addsrc(Parameter::IMM, op1->field[0]);
                            it->adddst(Parameter::REG, op0->field[0]);
                        } else if (op1->ty == Operand::REG) {
                            it->addsrc(Parameter::REG, op1->field[0]);
                            it->adddst(Parameter::REG, op0->field[0]);
                        } else if (op1->ty == Operand::MEM) {
                            nbyte = op1->bit / 8;
                            AddrRange rar(it->raddr, it->raddr + nbyte - 1);
                            it->addsrc(Parameter::MEM, rar);
                            it->adddst(Parameter::REG, op0->field[0]);
                        } else {
                            cout << "mov error: op0 is Reg, but op1 is not Imm, Reg, or Mem" << endl;
                            return 1;
                        }
                    } else if (op0->ty == Operand::MEM) {
                        if (op1->ty == Operand::IMM) {
                            it->addsrc(Parameter::IMM, op1->field[0]);
                            nbyte = op0->bit / 8;
                            AddrRange war(it->waddr, it->waddr + nbyte - 1);
                            it->adddst(Parameter::MEM, war);
                        } else if (op1->ty == Operand::REG) {
                            it->addsrc(Parameter::REG, op1->field[0]);
                            nbyte = op0->bit / 8;
                            AddrRange war(it->waddr, it->waddr + nbyte - 1);
                            it->adddst(Parameter::MEM, war);
                        } else {
                            cout << "mov error: op0 is Mem, but op1 is not Imm, Reg, or Mem" << endl;
                            return 1;
                        }
                    } else {
                        cout << "mov error: op0 is not Mem or Reg." << endl;
                        return 1;
                    }
                }
                break;
            }
            case 3: {
                Operand *op0 = it->oprd[0];
                Operand *op1 = it->oprd[1];
                Operand *op2 = it->oprd[2];

                if (it->opcstr == "imul" && op0->ty == Operand::REG &&
                    op1->ty == Operand::REG && op2->ty == Operand::IMM) { // imul reg, reg, imm
                    it->addsrc(Parameter::IMM, op2->field[0]);
                    it->addsrc(Parameter::REG, op1->field[0]);
                    it->addsrc(Parameter::REG, op0->field[0]);
                } else {
                    cout << "other 3-op instruction error: ";
                    cout << "Not imul reg, reg, imm." << endl;
                    return 1;
                }
                break;
            }
            default:
                cout << "error: instruction has more than 4 operands." << endl;
                return 1;
                break;
        }
    }
    return 0;
}

void printInstParameter(list<Inst> &L)
{
    for (list<Inst>::iterator it = L.begin(); it != L.end(); ++it) {
        cout << it->id << " " << it->addr << " " << it->assembly << "\t";
        cout << "src: ";

        for (int i = 0, max = it->src.size(); i < max; ++i) {
            Parameter p = it->src[i];
            if (p.ty == Parameter::IMM) {
                cout << "(IMM ";
                printf("0x%llx) ", p.idx);
            } else if (p.ty == Parameter::REG) {
                cout << "(REG ";
                cout << reg2string(p.reg) << p.idx << ") ";
            } else if (p.ty == Parameter::MEM) {
                cout << "(MEM ";
                printf("%llx) ", p.idx);
            } else {
                cout << "printInstParameter error: unkonwn src type." << endl;
            }
        }
        cout << ", dst: ";

        for (int i = 0, max = it->dst.size(); i < max; ++i) {
            Parameter p = it->dst[i];
            if (p.ty == Parameter::IMM) {
                cout << "(IMM ";
                printf("0x%llx) ", p.idx);
            } else if (p.ty == Parameter::REG) {
                cout << "(REG ";
                cout << reg2string(p.reg) << p.idx << ") ";
            } else if (p.ty == Parameter::MEM) {
                cout << "(MEM ";
                printf("%llx) ", p.idx);
            } else {
                cout << "printInstParameter error: unkonwn dst type." << endl;
            }
        }
        cout << endl;
    }
}
int backslice(list<Inst> &L)
{
    set<Parameter> wl;        // a working list containing current src parameters
    list<Inst> sl;            // the sliced result

    list<Inst>::reverse_iterator rit = L.rbegin();
    for (int i = 0, max = rit->src.size(); i < max; ++i) {
        wl.insert(rit->src[i]);
    }
    sl.push_front(*rit);
    ++rit;

    while (rit != L.rend()) {
        bool isdep1 = false, isdep2 = false; // Flags to check if the current instruction is dependent

        if (rit->dst.size() == 0) {
            // Skip instructions without destination parameters
        } else if (rit->opcstr == "xchg") { // Handle xchg separately (has two destinations)
            for (int i = 0, max = rit->dst.size(); i < max; ++i) {
                set<Parameter>::iterator sit1 = wl.find(rit->dst[i]);
                if (sit1 != wl.end()) {
                    isdep1 = true;
                    wl.erase(sit1);
                }
            }
            for (int i = 0, max = rit->dst2.size(); i < max; ++i) {
                set<Parameter>::iterator sit2 = wl.find(rit->dst2[i]);
                if (sit2 != wl.end()) {
                    isdep2 = true;
                    wl.erase(sit2);
                }
            }
            if (isdep1) {
                for (int i = 0, max = rit->src2.size(); i < max; ++i) {
                    wl.insert(rit->src2[i]);
                }
                sl.push_front(*rit);
            }
            if (isdep2) {
                for (int i = 0, max = rit->src.size(); i < max; ++i) {
                    wl.insert(rit->src[i]);
                }
                sl.push_front(*rit);
            }
        } else {
            for (int i = 0, max = rit->dst.size(); i < max; ++i) {
                set<Parameter>::iterator sit = wl.find(rit->dst[i]);
                if (sit != wl.end()) {
                    isdep1 = true;
                    wl.erase(sit);
                }
            }
            if (isdep1) {
                for (int i = 0, max = rit->src.size(); i < max; ++i) {
                    if (!rit->src[i].isIMM())
                        wl.insert(rit->src[i]);
                }
                sl.push_front(*rit);
            }
        }
        ++rit;
    }

    for (set<Parameter>::iterator it = wl.begin(); it != wl.end(); ++it) {
        it->show();
    }
    cout << endl;
    printInstParameter(sl);
    printTraceHuman(sl, "slice.human.trace");
    printTraceLLSE(sl, "slice.llse.trace");

    return 0;
}
int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <tracefile>\n", argv[0]);
        return 1;
    }

    ifstream infile(argv[1]);
    if (!infile.is_open()) {
        fprintf(stderr, "Open file error!\n");
        return 1;
    }

    parseTrace(&infile, &instlist);
    infile.close();

    parseOperand(instlist.begin(), instlist.end());

    if (buildParameter(instlist) != 0) {
        cerr << "Error building parameters!" << endl;
        return 1;
    }

    if (backslice(instlist) != 0) {
        cerr << "Error in backslice!" << endl;
        return 1;
    }

    return 0;
}

