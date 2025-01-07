
#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <bitset>
#include <sstream>

using namespace std;

#include "core.hpp"
#include "mg-symengine.hpp"

enum ValueTy {SYMBOL, CONCRETE, HYBRID, UNKNOWN};
enum OperTy {ADD, MOV, SHL, XOR, SHR};
typedef pair<int,int> BitRange;

// A symbolic or concrete value in a formula
struct Value {
     int id;                             // a unique id for each value
     ValueTy valty;
     Operation *opr;
     string conval;                      // concrete value
     bitset<64> bsconval;                // concrete set stored as bitset
     BitRange brange;
     map<BitRange, Value*> childs;       // a list of child values in a hybrid value
     int len;                            // length of the value

     static int idseed;

     Value(ValueTy vty);
     Value(ValueTy vty, int l);
     Value(ValueTy vty, string con); // constructor for concrete value
     Value(ValueTy vty, string con, int l);
     Value(ValueTy vty, Operation *oper);
     Value(ValueTy vty, Operation *oper, int l);
     Value(ValueTy vty, bitset<64> bs);

     bool isSymbol();
     bool isConcrete();
     bool isHybrid();
};

int Value::idseed = 0;

Value::Value(ValueTy vty) : opr(NULL)
{
     id = ++idseed;
     valty = vty;
     len = 64;
}

Value::Value(ValueTy vty, int l) : opr(NULL)
{
     id = ++idseed;
     valty = vty;
     len = l;
}

Value::Value(ValueTy vty, string con) : opr(NULL), bsconval(stoull(con, 0, 16))
{
     id = ++idseed;
     valty = vty;
     conval = con;
     brange.first = 0;
     brange.second = 63;
     len = 64;
}

Value::Value(ValueTy vty, string con, int l) : opr(NULL)
{
     id = ++idseed;
     valty = vty;
     conval = con;
     len = l;
}

Value::Value(ValueTy vty, bitset<64> bs) : opr(NULL)
{
     id = ++idseed;
     valty = vty;
     bsconval = bs;
     len = 64;
}

Value::Value(ValueTy vty, Operation *oper)
{
     id = ++idseed;
     valty = vty;
     opr = oper;
     len = 64;
}

Value::Value(ValueTy vty, Operation *oper, int l)
{
     id = ++idseed;
     valty = vty;
     opr = oper;
     len = l;
}


bool Value::isSymbol()
{
     if (this->valty == SYMBOL)
          return true;
     else
          return false;
}

bool Value::isConcrete()
{
     if (this->valty == CONCRETE)
          return true;
     else
          return false;
}
bool Value::isHybrid()
{
     if (this->valty == HYBRID)
          return true;
     else
          return false;
}


string getValueName(Value *v)
{
     if (v->valty == SYMBOL)
          return "sym" + to_string(v->id);
     else
          return v->conval;
}

// An operation taking several values to calculate a result value
struct Operation {
     string opty;
     Value *val[3];

     Operation(string opt, Value *v1);
     Operation(string opt, Value *v1, Value *v2);
     Operation(string opt, Value *v1, Value *v2, Value *v3);
};

Operation::Operation(string opt, Value *v1)
{
     opty = opt;
     val[0] = v1;
     val[1] = NULL;
     val[2] = NULL;
}

Operation::Operation(string opt, Value *v1, Value *v2)
{
     opty = opt;
     val[0] = v1;
     val[1] = v2;
     val[2] = NULL;
}

Operation::Operation(string opt, Value *v1, Value *v2, Value *v3)
{
     opty = opt;
     val[0] = v1;
     val[1] = v2;
     val[2] = v3;
}

Value *buildop1(string opty, Value *v1)
{
     Operation *oper = new Operation(opty, v1);
     Value *result;

     if (v1->isSymbol())
          result = new Value(SYMBOL, oper);
     else
          result = new Value(CONCRETE, oper);

     return result;
}

Value *buildop2(string opty, Value *v1, Value *v2)
{
     Operation *oper = new Operation(opty, v1, v2);
     Value *result;
     if (v1->isSymbol() || v2->isSymbol())
          result = new Value(SYMBOL, oper);
     else
          result = new Value(CONCRETE, oper);

     return result;
}

// Currently there is no 3-operand operation,
// it is reserved for future.
Value *buildop3(string opty, Value *v1, Value *v2, Value *v3)
{
     Operation *oper = new Operation(opty, v1, v2, v3);
     Value *result;

     if (v1->isSymbol() || v2->isSymbol() || v3->isSymbol())
          result = new Value(SYMBOL, oper);
     else
          result = new Value(CONCRETE, oper);

     return result;
}


// ********************************
//  Class SEEngine Implementation
// ********************************

// return the concrete value in a register
ADDR64 SEEngine::getRegConVal(string reg)
{
     if (reg == "rax")
          return ip->ctxreg[0];
     else if (reg == "rbx")
          return ip->ctxreg[1];
     else if (reg == "rcx")
          return ip->ctxreg[2];
     else if (reg == "rdx")
          return ip->ctxreg[3];
     else if (reg == "rsi")
          return ip->ctxreg[4];
     else if (reg == "rdi")
          return ip->ctxreg[5];
     else if (reg == "rsp")
          return ip->ctxreg[6];
     else if (reg == "rbp")
          return ip->ctxreg[7];
     else {
          cerr << "now only get 64-bit register's concrete value." << endl;
          return 0;
     }
}

// Calculate the address in a memory operand
ADDR64 SEEngine::calcAddr(Operand *opr)
{
     ADDR64 r1, r2, c;        // addr = r1 + r2*n + c
     int64_t n;
     switch (opr->tag)
     {
     case 7:                    // addr7 = r1 + r2*n + c
          r1 = getRegConVal(opr->field[0]);
          r2 = getRegConVal(opr->field[1]);
          n  = stoi(opr->field[2]);
          c  = stoull(opr->field[4], 0, 16);
          if (opr->field[3] == "+")
               return r1 + r2*n + c;
          else if (opr->field[3] == "-")
               return r1 + r2*n - c;
          else {
               cerr << "unrecognized addr: tag 7" << endl;
               return 0;
          }
     case 4:                    // addr4 = r1 + c
          r1 = getRegConVal(opr->field[0]);
          c = stoull(opr->field[2], 0, 16);
          if (opr->field[1] == "+")
               return r1 + c;
          else if (opr->field[1] == "-")
               return r1 - c;
          else {
               cerr << "unrecognized addr: tag 4" << endl;
               return 0;
          }
     case 5:                    // addr5 = r1 + r2*n
          r1 = getRegConVal(opr->field[0]);
          r2 = getRegConVal(opr->field[1]);
          n  = stoi(opr->field[2]);
          return r1 + r2*n;
     case 6:                    // addr6 = r2*n + c
          r2 = getRegConVal(opr->field[0]);
          n = stoi(opr->field[1]);
          c = stoull(opr->field[3], 0, 16);
          if (opr->field[2] == "+")
               return r2*n + c;
          else if (opr->field[2] == "-")
               return r2*n - c;
          else {
               cerr << "unrecognized addr: tag 6" << endl;
               return 0;
          }
     case 3:                    // addr3 = r2*n
          r2 = getRegConVal(opr->field[0]);
          n = stoi(opr->field[1]);
          return r2*n;
     case 1:                    // addr1 = c
          c = stoull(opr->field[0], 0, 16);
          return c;
     case 2:                    // addr2 = r1
          r1 = getRegConVal(opr->field[0]);
          return r1;
     default:
          cerr << "unrecognized addr tag" << endl;
          return 0;
     }
}

bool hasVal(Value *v, int start, int end)
{
    BitRange br(start, end);
    map<BitRange, Value*>::iterator i = v->childs.find(br);
    if (i == v->childs.end())
        return false;
    else
        return true;
}

Value* readVal(Value *v, int start, int end)
{
    BitRange br(start, end);
    map<BitRange, Value*>::iterator i = v->childs.find(br);
    if (i == v->childs.end())
        return NULL;
    else
        return i->second;
}
string bs2str(bitset<63> bs, BitRange br){
     int start = br.first, end = br.second;
     unsinged long long int ui = 0, step = 1;
     for (int i = start; i <= end; ++i, step *= 2) {
          ui += bs[i] * step;
     }
     stringstream strs;
     strs << "0x" << hex << ui;   // the number of bits that should shift right
     string res(strs.str());

     return res;
}


Value *writeVal(Value *from, Value *to, int start, int end)
{
     Value *res = new Value(HYBRID);
     BitRange brfrom(start, end);
     if (to->isHybrid()) {
          map<BitRange, Value*>::iterator i = to->childs.find(brfrom);
          if (i != to->childs.end()) {
               i->second = from;
               return to;
          } else {
               cerr << "writeVal: no child in to match the from!" << endl;
               return NULL;
          }
     } else if (from->isSymbol() && to->isConcrete()) {
          int s1 = to->brange.first;
          int e1 = to->brange.second;
          Value *v1 = new Value(CONCRETE, to->bsconval);
          Value *v2 = new Value(CONCRETE, to->bsconval);
          v1->brange.first = s1;
          v1->brange.second = start-1;
          v1->conval = bs2str(v1->bsconval, v1->brange);

          v2->brange.first = end+1;
          v2->brange.second = e1;
          v2->conval = bs2str(v2->bsconval, v2->brange);

          res->childs.insert(pair<BitRange, Value*>(v1->brange, v1));
          res->childs.insert(pair<BitRange, Value*>(brfrom, from));
          res->childs.insert(pair<BitRange, Value*>(v2->brange, v2));


          return res;
     } else {
          cerr << "writeVal: the case is missing!" << endl;
          return NULL;
     }
}

Value* SEEngine::readReg(string &s)
{
    Value* res;

    if (s == "rax" || s == "rbx" || s == "rcx" || s == "rdx" ||
        s == "rsi" || s == "rdi" || s == "rsp" || s == "rbp" ||
        s == "r8"  || s == "r9"  || s == "r10" || s == "r11" ||
        s == "r12" || s == "r13" || s == "r14" || s == "r15") {
        return ctx[s];  // Return the full 64-bit register value
    }

    // Handle 32-bit sub-registers
    if (s == "eax" || s == "ebx" || s == "ecx" || s == "edx" ||
        s == "esi" || s == "edi" || s == "esp" || s == "ebp" ||
        s == "r8d" || s == "r9d" || s == "r10d" || s == "r11d" ||
        s == "r12d" || s == "r13d" || s == "r14d" || s == "r15d") {
        return buildop1("and", ctx["r" + s.substr(1)], new Value(CONCRETE, "0xFFFFFFFF"));  // Mask lower 32 bits
    }

    // Handle 16-bit sub-registers
    if (s == "ax" || s == "bx" || s == "cx" || s == "dx" ||
        s == "si" || s == "di" || s == "bp" || s == "sp" ||
        s == "r8w" || s == "r9w" || s == "r10w" || s == "r11w" ||
        s == "r12w" || s == "r13w" || s == "r14w" || s == "r15w") {
        return buildop1("and", ctx["r" + s.substr(1)], new Value(CONCRETE, "0xFFFF"));  // Mask lower 16 bits
    }

    // Handle 8-bit low sub-registers
    if (s == "al" || s == "bl" || s == "cl" || s == "dl" ||
        s == "sil" || s == "dil" || s == "bpl" || s == "spl" ||
        s == "r8b" || s == "r9b" || s == "r10b" || s == "r11b" ||
        s == "r12b" || s == "r13b" || s == "r14b" || s == "r15b") {
        return buildop1("and", ctx["r" + s.substr(1)], new Value(CONCRETE, "0xFF"));  // Mask lower 8 bits
    }

    // Handle 8-bit high sub-registers (ah, bh, ch, dh)
    if (s == "ah" || s == "bh" || s == "ch" || s == "dh") {
        string rname = "r" + s.substr(0, 1) + "x";  // Get corresponding 64-bit register
        Value* v0 = buildop1("and", ctx[rname], new Value(CONCRETE, "0xFF00"));  // Mask bits [8:15]
        return buildop2("shr", v0, new Value(CONCRETE, "8"));  // Shift right by 8 bits
    }

    cerr << "Unknown register name: " << s << endl;
    return nullptr;
}
void SEEngine::writeReg(string &s, Value *v)
{
    if (s == "rax" || s == "rbx" || s == "rcx" || s == "rdx" ||
        s == "rsi" || s == "rdi" || s == "rsp" || s == "rbp" ||
        s == "r8"  || s == "r9"  || s == "r10" || s == "r11" ||
        s == "r12" || s == "r13" || s == "r14" || s == "r15") {
        ctx[s] = v;  // Directly write the value for 64-bit registers
        return;
    }

    // Handle 32-bit sub-registers
    if (s == "eax" || s == "ebx" || s == "ecx" || s == "edx" ||
        s == "esi" || s == "edi" || s == "esp" || s == "ebp" ||
        s == "r8d" || s == "r9d" || s == "r10d" || s == "r11d" ||
        s == "r12d" || s == "r13d" || s == "r14d" || s == "r15d") {
        Value *mask = new Value(CONCRETE, "0xFFFFFFFF00000000");  // Mask upper 32 bits
        Value *regval = buildop2("and", ctx["r" + s.substr(1)], mask);
        ctx["r" + s.substr(1)] = buildop2("or", regval, v);  // Combine masked register with new value
        return;
    }

    // Handle 16-bit sub-registers
    if (s == "ax" || s == "bx" || s == "cx" || s == "dx" ||
        s == "si" || s == "di" || s == "bp" || s == "sp" ||
        s == "r8w" || s == "r9w" || s == "r10w" || s == "r11w" ||
        s == "r12w" || s == "r13w" || s == "r14w" || s == "r15w") {
        Value *mask = new Value(CONCRETE, "0xFFFFFFFFFFFF0000");  // Mask upper 48 bits
        Value *regval = buildop2("and", ctx["r" + s.substr(1)], mask);
        ctx["r" + s.substr(1)] = buildop2("or", regval, v);  // Combine masked register with new value
        return;
    }

    // Handle 8-bit low sub-registers
    if (s == "al" || s == "bl" || s == "cl" || s == "dl" ||
        s == "sil" || s == "dil" || s == "bpl" || s == "spl" ||
        s == "r8b" || s == "r9b" || s == "r10b" || s == "r11b" ||
        s == "r12b" || s == "r13b" || s == "r14b" || s == "r15b") {
        Value *mask = new Value(CONCRETE, "0xFFFFFFFFFFFFFF00");  // Mask upper 56 bits
        Value *regval = buildop2("and", ctx["r" + s.substr(1)], mask);
        ctx["r" + s.substr(1)] = buildop2("or", regval, v);  // Combine masked register with new value
        return;
    }

    // Handle 8-bit high sub-registers (ah, bh, ch, dh)
    if (s == "ah" || s == "bh" || s == "ch" || s == "dh") {
        string rname = "r" + s.substr(0, 1) + "x";  // Get corresponding 64-bit register
        Value *mask = new Value(CONCRETE, "0xFFFFFFFFFFFF00FF");  // Mask bits [8:15]
        Value *regval = buildop2("and", ctx[rname], mask);  // Clear bits [8:15]
        Value *shifted = buildop2("shl", v, new Value(CONCRETE, "8"));  // Shift new value to bits [8:15]
        ctx[rname] = buildop2("or", regval, shifted);  // Combine masked register with shifted value
        return;
    }

    cerr << "Unknown register name: " << s << endl;
}
bool SEEngine::issubset(AddrRange ar, AddrRange *superset)
{
    for (auto it = mem.begin(); it != mem.end(); ++it) {
        AddrRange curar = it->first;
        if (curar.first <= ar.first && curar.second >= ar.second) {
            superset->first = curar.first;
            superset->second = curar.second;
            return true;
        }
    }
    return false;
}
bool SEEngine::issuperset(AddrRange ar, AddrRange *subset)
{
    for (auto it = mem.begin(); it != mem.end(); ++it) {
        AddrRange curar = it->first;
        if (curar.first >= ar.first && curar.second <= ar.second) {
            subset->first = curar.first;
            subset->second = curar.second;
            return true;
        }
    }
    return false;
}
bool SEEngine::isnew(AddrRange ar)
{
    for (auto it = mem.begin(); it != mem.end(); ++it) {
        AddrRange curar = it->first;
        if ((curar.first <= ar.first && curar.second >= ar.first) ||  // Overlaps at the start
            (curar.first <= ar.second && curar.second >= ar.second)) { // Overlaps at the end
            return false;
        }
    }
    return true;
}
Value* SEEngine::readMem(ADDR64 addr)
{
    ADDR64 end = addr + 7;  // 64-bit value spans 8 bytes

    AddrRange ar(addr, end), res;

    if (memfind(ar)) return mem[ar];  // If the exact range exists, return the value

    if (isnew(ar)) {
        Value *v = new Value(SYMBOL, 8);  // Create a new symbolic value for 8 bytes
        mem[ar] = v;
        meminput[v] = ar;
        return v;
    } else if (issubset(ar, &res)) {
        ADDR64 b1 = ar.first, e1 = ar.second;
        ADDR64 b2 = res.first, e2 = res.second;
        string mask = "0x";

        for (ADDR64 i = e2; i >= b2; --i) {  // Create a mask for the desired range
            if (i >= b1 && i <= e1)
                mask += "ff";
            else
                mask += "00";
        }

        stringstream strs;
        strs << "0x" << hex << (b1 - b2) * 8;  // Compute shift amount
        string low0(strs.str());

        Value *v0 = mem[res];
        Value *v1 = new Value(CONCRETE, mask);
        Value *v2 = buildop2("and", v0, v1);
        Value *v3 = new Value(CONCRETE, low0);
        Value *v4 = buildop2("shr", v2, v3);  // Shift right to extract the value

        return v4;
    } else {
        cerr << "readMem64: Partial overlapping symbolic memory access is not implemented yet!" << endl;
        return NULL;
    }
}

void SEEngine::writeMem(ADDR64 addr, Value *v)
{
    ADDR64 end = addr + 7;  // 64-bit value spans 8 bytes
    AddrRange ar(addr, end), res;

    if (memfind(ar) || isnew(ar)) {  // If exact match or new range
        mem[ar] = v;
        return;
    } else if (issuperset(ar, &res)) {  // If the new range is a superset of an existing range
        mem.erase(res);
        mem[ar] = v;
        return;
    } else if (issubset(ar, &res)) {  // If the new range is a subset of an existing range
        ADDR64 b1 = ar.first, e1 = ar.second;
        ADDR64 b2 = res.first, e2 = res.second;

        string mask = "0x";  // Mask to clear bits in the old value
        for (ADDR64 i = e2; i >= b2; --i) {
            if (i >= b1 && i <= e1)
                mask += "00";
            else
                mask += "ff";
        }

        stringstream strs;
        strs << "0x" << hex << (b1 - b2) * 8;
        string low0(strs.str());  // Compute shift amount

        Value *v0 = mem[res];
        Value *v1 = new Value(CONCRETE, mask);
        Value *v2 = buildop2("and", v0, v1);
        Value *v3 = new Value(CONCRETE, low0);
        Value *v4 = buildop2("shl", v, v3);  // Shift left the new value to the correct position
        Value *v5 = buildop2("or", v2, v4);  // Combine old and new values
        mem[res] = v5;
        return;
    } else {
        cerr << "writeMem64: Partial overlapping symbolic memory access is not implemented yet!" << endl;
    }
}

void SEEngine::init(Value *rax, Value *rbx, Value *rcx, Value *rdx,
                    Value *rsi, Value *rdi, Value *rsp, Value *rbp,
                    Value *r8,  Value *r9,  Value *r10, Value *r11,
                    Value *r12, Value *r13, Value *r14, Value *r15,
                    list<Inst>::iterator it1,
                    list<Inst>::iterator it2)
{
    // Initialize 64-bit registers
    ctx["rax"] = rax;
    ctx["rbx"] = rbx;
    ctx["rcx"] = rcx;
    ctx["rdx"] = rdx;
    ctx["rsi"] = rsi;
    ctx["rdi"] = rdi;
    ctx["rsp"] = rsp;
    ctx["rbp"] = rbp;
    ctx["r8"]  = r8;
    ctx["r9"]  = r9;
    ctx["r10"] = r10;
    ctx["r11"] = r11;
    ctx["r12"] = r12;
    ctx["r13"] = r13;
    ctx["r14"] = r14;
    ctx["r15"] = r15;

    // Map register values for input tracking
    reginput[rax] = "rax";
    reginput[rbx] = "rbx";
    reginput[rcx] = "rcx";
    reginput[rdx] = "rdx";
    reginput[rsi] = "rsi";
    reginput[rdi] = "rdi";
    reginput[rsp] = "rsp";
    reginput[rbp] = "rbp";
    reginput[r8]  = "r8";
    reginput[r9]  = "r9";
    reginput[r10] = "r10";
    reginput[r11] = "r11";
    reginput[r12] = "r12";
    reginput[r13] = "r13";
    reginput[r14] = "r14";
    reginput[r15] = "r15";

    this->start = it1;
    this->end = it2;
}


void SEEngine::init(list<Inst>::iterator it1,
                    list<Inst>::iterator it2)
{
     this->start = it1;
     this->end = it2;
}

void SEEngine::initAllRegSymbol(list<Inst>::iterator it1,
                                list<Inst>::iterator it2)
{
    // Create symbolic values for all 64-bit registers
    Value *rax = new Value(SYMBOL);
    Value *rbx = new Value(SYMBOL);
    Value *rcx = new Value(SYMBOL);
    Value *rdx = new Value(SYMBOL);
    Value *rsi = new Value(SYMBOL);
    Value *rdi = new Value(SYMBOL);
    Value *rsp = new Value(SYMBOL);
    Value *rbp = new Value(SYMBOL);
    Value *r8  = new Value(SYMBOL);
    Value *r9  = new Value(SYMBOL);
    Value *r10 = new Value(SYMBOL);
    Value *r11 = new Value(SYMBOL);
    Value *r12 = new Value(SYMBOL);
    Value *r13 = new Value(SYMBOL);
    Value *r14 = new Value(SYMBOL);
    Value *r15 = new Value(SYMBOL);

    // Initialize context with symbolic values
    ctx["rax"] = rax;
    ctx["rbx"] = rbx;
    ctx["rcx"] = rcx;
    ctx["rdx"] = rdx;
    ctx["rsi"] = rsi;
    ctx["rdi"] = rdi;
    ctx["rsp"] = rsp;
    ctx["rbp"] = rbp;
    ctx["r8"]  = r8;
    ctx["r9"]  = r9;
    ctx["r10"] = r10;
    ctx["r11"] = r11;
    ctx["r12"] = r12;
    ctx["r13"] = r13;
    ctx["r14"] = r14;
    ctx["r15"] = r15;

    // Map symbolic values for input tracking
    reginput[rax] = "rax";
    reginput[rbx] = "rbx";
    reginput[rcx] = "rcx";
    reginput[rdx] = "rdx";
    reginput[rsi] = "rsi";
    reginput[rdi] = "rdi";
    reginput[rsp] = "rsp";
    reginput[rbp] = "rbp";
    reginput[r8]  = "r8";
    reginput[r9]  = "r9";
    reginput[r10] = "r10";
    reginput[r11] = "r11";
    reginput[r12] = "r12";
    reginput[r13] = "r13";
    reginput[r14] = "r14";
    reginput[r15] = "r15";

    start = it1;
    end = it2;
}
set<string> noeffectinst = {
    "test", "jmp", "jz", "jbe", "jo", "jno", "js", "jns", "je", "jne",
    "jnz", "jb", "jnae", "jc", "jnb", "jae", "jnc", "jna", "ja", "jnbe", "jl",
    "jnge", "jge", "jnl", "jle", "jng", "jg", "jnle", "jp", "jpe", "jnp", "jpo",
    "jcxz", "jecxz", "ret", "cmp", "call", 
    
    // 64-bit specific mnemonics
    "jmpq", "retq", "callq", "cmpq", 
    "jzq", "jeq", "jnzq", "jneq", 
    "jgeq", "jleq", "jgq", "jlq", 
    "jbeq", "jaq", "jnae", "jnaeq", "jaeq", "jbq"
};


int SEEngine::symexec()
{
    for (list<Inst>::iterator it = start; it != end; ++it) {
        ip = it;

        // Skip no-effect instructions (jumps, comparisons without side effects, etc.)
        if (noeffectinst.find(it->opcstr) != noeffectinst.end()) continue;

        switch (it->oprnum) {
        case 0: 
            // Handle zero-operand instructions (e.g., nop)
            break;

        case 1: { 
            // Handle one-operand instructions (e.g., push, pop, inc, dec, neg, not)
            Operand *op0 = it->oprd[0];
            Value *v0, *res, *temp;
            int nbyte;

            if (it->opcstr == "push") {
                if (op0->ty == Operand::IMM) {
                    v0 = new Value(CONCRETE, op0->field[0]);
                    writeMem(it->waddr, 4, v0);
                } else if (op0->ty == Operand::REG) {
                    nbyte = op0->bit / 8;
                    temp = readReg(op0->field[0]);
                    writeMem(it->waddr, nbyte, temp);
                } else if (op0->ty == Operand::MEM) {
                    nbyte = op0->bit / 8;
                    v0 = readMem(it->raddr, nbyte);
                    writeMem(it->waddr, nbyte, v0);
                } else {
                    cerr << "push error: unsupported operand type!" << endl;
                    return 1;
                }
            } else if (it->opcstr == "pop") {
                if (op0->ty == Operand::REG) {
                    nbyte = op0->bit / 8;
                    temp = readMem(it->raddr, nbyte);
                    writeReg(op0->field[0], temp);
                } else if (op0->ty == Operand::MEM) {
                    nbyte = op0->bit / 8;
                    temp = readMem(it->raddr, nbyte);
                    writeMem(it->waddr, nbyte, temp);
                } else {
                    cerr << "pop error: unsupported operand type!" << endl;
                    return 1;
                }
            } else if (it->opcstr == "inc" || it->opcstr == "dec" || it->opcstr == "neg" || it->opcstr == "not") {
                if (op0->ty == Operand::REG) {
                    v0 = readReg(op0->field[0]);
                    res = buildop1(it->opcstr, v0);
                    writeReg(op0->field[0], res);
                } else if (op0->ty == Operand::MEM) {
                    nbyte = op0->bit / 8;
                    v0 = readMem(it->raddr, nbyte);
                    res = buildop1(it->opcstr, v0);
                    writeMem(it->waddr, nbyte, res);
                } else {
                    cerr << "[Error] Line " << it->id << ": unknown 1-op instruction!" << endl;
                    return 1;
                }
            } else if (it->opcstr == "bswap") {
                if (op0->ty == Operand::REG) {
                    v0 = readReg(op0->field[0]);
                    res = buildop1("bswap", v0);
                    writeReg(op0->field[0], res);
                } else {
                    cerr << "bswap error: unsupported operand type!" << endl;
                    return 1;
                }
            } else {
                cerr << "Error: Unsupported one-operand instruction " << it->opcstr << endl;
                return 1;
            }
            break;
        }

        case 2: {
            // Handle two-operand instructions (e.g., mov, add, sub, and, or, xor)
            Operand *op0 = it->oprd[0];
            Operand *op1 = it->oprd[1];
            Value *v0, *v1, *res;
            int nbyte;

            if (it->opcstr == "mov") {
                if (op0->ty == Operand::REG) {
                    v1 = (op1->ty == Operand::IMM) ? new Value(CONCRETE, op1->field[0]) : readReg(op1->field[0]);
                    writeReg(op0->field[0], v1);
                } else if (op0->ty == Operand::MEM) {
                    v1 = (op1->ty == Operand::IMM) ? new Value(CONCRETE, op1->field[0]) : readReg(op1->field[0]);
                    writeMem(it->waddr, op0->bit / 8, v1);
                } else {
                    cerr << "mov error: unsupported operand type!" << endl;
                    return 1;
                }
            } else if (it->opcstr == "add" || it->opcstr == "sub" || it->opcstr == "and" || 
                       it->opcstr == "or" || it->opcstr == "xor" || it->opcstr == "shl" || it->opcstr == "shr") {
                if (op0->ty == Operand::REG) {
                    v0 = readReg(op0->field[0]);
                    v1 = (op1->ty == Operand::IMM) ? new Value(CONCRETE, op1->field[0]) : readReg(op1->field[0]);
                    res = buildop2(it->opcstr, v0, v1);
                    writeReg(op0->field[0], res);
                } else if (op0->ty == Operand::MEM) {
                    nbyte = op0->bit / 8;
                    v0 = readMem(it->raddr, nbyte);
                    v1 = (op1->ty == Operand::IMM) ? new Value(CONCRETE, op1->field[0]) : readReg(op1->field[0]);
                    res = buildop2(it->opcstr, v0, v1);
                    writeMem(it->waddr, nbyte, res);
                } else {
                    cerr << "Error: Unsupported operand type in " << it->opcstr << " instruction!" << endl;
                    return 1;
                }
            } else if (it->opcstr == "cmp") {
                // cmp: compare op0 and op1 (sets flags, no result stored)
                v0 = (op0->ty == Operand::REG) ? readReg(op0->field[0]) : readMem(it->raddr, op0->bit / 8);
                v1 = (op1->ty == Operand::REG) ? readReg(op1->field[0]) : readMem(it->raddr, op1->bit / 8);
                res = buildop2("sub", v0, v1); // Subtraction to set flags (dummy result)
            } else if (it->opcstr == "test") {
                // test: logical AND op0 and op1 (sets flags, no result stored)
                v0 = (op0->ty == Operand::REG) ? readReg(op0->field[0]) : readMem(it->raddr, op0->bit / 8);
                v1 = (op1->ty == Operand::REG) ? readReg(op1->field[0]) : readMem(it->raddr, op1->bit / 8);
                res = buildop2("and", v0, v1); // AND operation to set flags (dummy result)
            } else {
                cerr << "Error: Unsupported two-operand instruction " << it->opcstr << endl;
                return 1;
            }
            break;
        }

        case 3: {
            // Handle three-operand instructions (e.g., imul reg, reg, imm)
            Operand *op0 = it->oprd[0];
            Operand *op1 = it->oprd[1];
            Operand *op2 = it->oprd[2];
            Value *v1, *v2, *res;

            if (it->opcstr == "imul") {
                v1 = readReg(op1->field[0]);
                v2 = new Value(CONCRETE, op2->field[0]);
                res = buildop2("imul", v1, v2);
                writeReg(op0->field[0], res);
            } else {
                cerr << "Error: Unsupported three-operand instruction " << it->opcstr << endl;
                return 1;
            }
            break;
        }

        default:
            cerr << "Error: Instruction with unsupported operand count!" << endl;
            return 1;
        }
    }
    return 0;
}

// Recursively traverse each operand of v and print it as a parenthesized formula
void traverse(Value *v)
{
    if (v == NULL) return;

    Operation *op = v->opr;
    if (op == NULL) {
        if (v->valty == CONCRETE) {
            cout << v->conval;
        } else {
            cout << "sym" << v->id;
        }
    } else {
        cout << "(" << op->opty << " ";
        traverse(op->val[0]);
        cout << " ";
        traverse(op->val[1]);
        cout << ")";
    }
}

// Recursively traverse for showing SE results, including hybrid values
void traverse2(Value *v)
{
    if (v == NULL) return;

    Operation *op = v->opr;
    if (op == NULL) {
        if (v->valty == CONCRETE) {
            cout << v->conval;
        } else if (v->valty == SYMBOL) {
            cout << "sym" << v->id << " ";
        } else if (v->valty == HYBRID) {
            cout << "[" << "hyb" << v->id << " ";
            for (auto &child : v->childs) {
                cout << "[" << child.first.first << "," << child.first.second << "]:";
                traverse2(child.second);
            }
            cout << "]";
        } else {
            cout << "unknown type" << endl;
            return;
        }
    } else {
        cout << "(" << op->opty << " ";
        traverse2(op->val[0]);
        cout << " ";
        traverse2(op->val[1]);
        cout << ")";
    }
}
void SEEngine::outputFormula(string reg)
{
    if (ctx.find(reg) == ctx.end()) {
        cerr << "Error: Register " << reg << " not found!" << endl;
        return;
    }

    Value *v = ctx[reg];
    cout << reg << " = sym" << v->id << " =" << endl;
    traverse(v);
    cout << endl;
}
void SEEngine::dumpreg(string reg)
{
    if (ctx.find(reg) == ctx.end()) {
        cerr << "Error: Register " << reg << " not found!" << endl;
        return;
    }

    Value *v = ctx[reg];
    cout << "Register " << reg << " = " << endl;
    traverse2(v);
    cout << endl;
}
vector<Value*> SEEngine::getAllOutput()
{
    vector<Value*> outputs;
    Value *v;

    // Symbols in general-purpose registers
    vector<string> registers = {
        "eax", "ebx", "ecx", "edx",    // 32-bit registers
        "rax", "rbx", "rcx", "rdx",    // 64-bit registers
        "esi", "edi", "esp", "ebp",    // Other 32-bit registers
        "rsi", "rdi", "rsp", "rbp"     // Corresponding 64-bit registers
    };

    for (const auto &reg : registers) {
        if (ctx.find(reg) != ctx.end()) {
            v = ctx[reg];
            if (v->opr != NULL) {
                outputs.push_back(v);
            }
        }
    }

    // Symbols in memory
    for (auto const &x : mem) {
        v = x.second;
        if (v->opr != NULL) {
            outputs.push_back(v);
        }
    }

    return outputs;
}
void SEEngine::printAllRegFormulas()
{
    vector<string> registers = {
        "eax", "ebx", "ecx", "edx",    // 32-bit registers
        "esi", "edi", "esp", "ebp",    // Other 32-bit registers
        "rax", "rbx", "rcx", "rdx",    // 64-bit registers
        "rsi", "rdi", "rsp", "rbp"     // Other 64-bit registers
    };

    for (const auto &reg : registers) {
        if (ctx.find(reg) != ctx.end()) {
            cout << reg << ": ";
            outputFormula(reg);
            printInputSymbols(reg);
            cout << endl;
        }
    }
}
void SEEngine::printAllMemFormulas()
{
    if (mem.empty()) {
        cout << "No symbolic values in memory." << endl;
        return;
    }

    for (auto const &x : mem) {
        AddrRange ar = x.first;
        Value *v = x.second;
        printf("Memory [%x, %x]: ", ar.first, ar.second);
        cout << "sym" << v->id << " =" << endl;
        traverse(v);
        cout << endl;
    }
}
set<Value*> *getInputs(Value *output)
{
    if (output == NULL) {
        cerr << "Error: Output value is NULL." << endl;
        return new set<Value*>;
    }

    queue<Value*> que;
    set<Value*> *inputset = new set<Value*>;
    set<Value*> visited;  // To track visited nodes and avoid infinite loops

    que.push(output);
    visited.insert(output);

    while (!que.empty()) {
        Value *v = que.front();
        Operation *op = v->opr;
        que.pop();

        if (op == NULL) {
            if (v->valty == SYMBOL)
                inputset->insert(v);
        } else {
            for (int i = 0; i < 3; ++i) {
                if (op->val[i] != NULL && visited.find(op->val[i]) == visited.end()) {
                    que.push(op->val[i]);
                    visited.insert(op->val[i]);
                }
            }
        }
    }

    return inputset;
}
void SEEngine::printInputSymbols(string output)
{
    if (ctx.find(output) == ctx.end()) {
        cerr << "Error: Register '" << output << "' not found in context." << endl;
        return;
    }

    Value *v = ctx[output];
    set<Value*> *insyms = getInputs(v);

    if (insyms->empty()) {
        cout << "No input symbols for " << output << "." << endl;
    } else {
        cout << insyms->size() << " input symbols for " << output << ": ";
        for (set<Value*>::iterator it = insyms->begin(); it != insyms->end(); ++it) {
            cout << "sym" << (*it)->id << " ";
        }
        cout << endl;
    }

    delete insyms;  // Free the dynamically allocated set
}
void SEEngine::printformula(Value *v)
{
    if (v == NULL) {
        cerr << "Error: Value is NULL." << endl;
        return;
    }

    set<Value*> *insyms = getInputs(v);

    cout << insyms->size() << " input symbols: " << endl;
    for (set<Value*>::iterator it = insyms->begin(); it != insyms->end(); ++it) {
        cout << "sym" << (*it)->id << ": ";
        map<Value*, AddrRange>::iterator it1 = meminput.find(*it);
        if (it1 != meminput.end()) {
            printf("[%llx, %llx]\n", it1->second.first, it1->second.second);  // 64-bit addresses
        } else {
            map<Value*, string>::iterator it2 = reginput.find(*it);
            if (it2 != reginput.end()) {
                cout << it2->second << endl;
            } else {
                cout << "Unknown input source." << endl;
            }
        }
    }

    cout << endl;
    cout << "sym" << v->id << " =" << endl;
    traverse(v);
    cout << endl;

    delete insyms;
}
void SEEngine::printMemFormula(ADDR64 addr1, ADDR64 addr2)
{
    AddrRange ar(addr1, addr2);
    if (mem.find(ar) == mem.end()) {
        cerr << "Error: No value found for the memory range [" << hex << addr1 << ", " << addr2 << "]." << endl;
        return;
    }

    Value *v = mem[ar];
    printformula(v);
}
ADDR64 eval(Value *v, map<Value*, ADDR64> *inmap)
{
    if (v == NULL) {
        cerr << "Error: Value is NULL." << endl;
        return 0;
    }

    Operation *op = v->opr;
    if (op == NULL) {
        if (v->valty == CONCRETE) {
            return stoul(v->conval, 0, 16);
        } else {
            return (*inmap)[v];
        }
    } else {
        ADDR64 op0 = 0, op1 = 0;

        if (op->val[0] != NULL) op0 = eval(op->val[0], inmap);
        if (op->val[1] != NULL) op1 = eval(op->val[1], inmap);

        if (op->opty == "add") {
            return op0 + op1;
        } else if (op->opty == "sub") {
            return op0 - op1;
        } else if (op->opty == "imul") {
            return op0 * op1;
        } else if (op->opty == "div") {
            if (op1 == 0) {
                cerr << "Error: Division by zero." << endl;
                return 0;
            }
            return op0 / op1;
        } else if (op->opty == "mod") {
            if (op1 == 0) {
                cerr << "Error: Modulus by zero." << endl;
                return 0;
            }
            return op0 % op1;
        } else if (op->opty == "xor") {
            return op0 ^ op1;
        } else if (op->opty == "and") {
            return op0 & op1;
        } else if (op->opty == "or") {
            return op0 | op1;
        } else if (op->opty == "shl") {
            return op0 << op1;
        } else if (op->opty == "shr") {
            return op0 >> op1;
        } else if (op->opty == "neg") {
            return ~op0 + 1;
        } else if (op->opty == "inc") {
            return op0 + 1;
        } else if (op->opty == "dec") {
            return op0 - 1;
        } else {
            cerr << "Error: Unsupported instruction '" << op->opty << "' encountered during evaluation." << endl;
            return 0;
        }
    }
}
// Given inputs, concrete compute the output value of a formula
ADDR64 SEEngine::conexec(Value *f, map<Value*, ADDR64> *inmap)
{
    if (f == NULL) {
        cerr << "Error: Formula value is NULL." << endl;
        return 0;
    }

    set<Value*> *inputsym = getInputs(f);
    set<Value*> inmapkeys;
    for (map<Value*, ADDR64>::iterator it = inmap->begin(); it != inmap->end(); ++it) {
        inmapkeys.insert(it->first);
    }

    if (inmapkeys != *inputsym) {
        cerr << "Error: Mismatch in number of input symbols and parameters." << endl;
        delete inputsym;
        return 0;
    }

    ADDR64 result = eval(f, inmap);
    delete inputsym;
    return result;
}
// Build a map based on a variable vector and an input vector
map<Value*, ADDR64> buildinmap(vector<Value*> *vv, vector<ADDR64> *input)
{
    map<Value*, ADDR64> inmap;
    if (vv == NULL || input == NULL) {
        cerr << "Error: Null input vectors provided." << endl;
        return inmap;
    }

    if (vv->size() != input->size()) {
        cerr << "Error: Number of input symbols does not match number of inputs." << endl;
        return inmap;
    }

    for (size_t i = 0; i < vv->size(); ++i) {
        inmap.insert(pair<Value*, ADDR64>((*vv)[i], (*input)[i]));
    }

    return inmap;
}
// Get all inputs of a formula and push them into a vector
vector<Value*> getInputVector(Value *f)
{
    vector<Value*> vv;
    if (f == NULL) {
        cerr << "Error: Formula value is NULL." << endl;
        return vv;
    }

    set<Value*> *inset = getInputs(f);
    vv.assign(inset->begin(), inset->end());
    delete inset;

    return vv;
}
string sympostfix;
// Output the calculation of v as a formula in CVC format
void outputCVC(Value *v, FILE *fp)
{
    if (v == NULL) return;

    Operation *op = v->opr;
    if (op == NULL) {
        if (v->valty == CONCRETE) {
            ADDR64 i = stoull(v->conval, 0, 16);  // Use 64-bit address
            fprintf(fp, "0hex%016llx", i);        // 16 hex digits for 64-bit
        } else {
            fprintf(fp, "sym%d%s", v->id, sympostfix.c_str());
        }
    } else {
        if (op->opty == "add") {
            fprintf(fp, "BVPLUS(64, ");
            outputCVC(op->val[0], fp);
            fprintf(fp, ", ");
            outputCVC(op->val[1], fp);
            fprintf(fp, ")");
        } else if (op->opty == "sub") {
            fprintf(fp, "BVSUB(64, ");
            outputCVC(op->val[0], fp);
            fprintf(fp, ", ");
            outputCVC(op->val[1], fp);
            fprintf(fp, ")");
        } else if (op->opty == "imul") {
            fprintf(fp, "BVMULT(64, ");
            outputCVC(op->val[0], fp);
            fprintf(fp, ", ");
            outputCVC(op->val[1], fp);
            fprintf(fp, ")");
        } else if (op->opty == "xor") {
            fprintf(fp, "BVXOR(");
            outputCVC(op->val[0], fp);
            fprintf(fp, ", ");
            outputCVC(op->val[1], fp);
            fprintf(fp, ")");
        } else if (op->opty == "and") {
            fprintf(fp, "BVAND(");
            outputCVC(op->val[0], fp);
            fprintf(fp, ", ");
            outputCVC(op->val[1], fp);
            fprintf(fp, ")");
        } else if (op->opty == "or") {
            fprintf(fp, "BVOR(");
            outputCVC(op->val[0], fp);
            fprintf(fp, ", ");
            outputCVC(op->val[1], fp);
            fprintf(fp, ")");
        } else if (op->opty == "neg") {
            fprintf(fp, "BVNEG(");
            outputCVC(op->val[0], fp);
            fprintf(fp, ")");
        } else if (op->opty == "shl") {
            fprintf(fp, "BVSHL(");
            outputCVC(op->val[0], fp);
            fprintf(fp, ", ");
            outputCVC(op->val[1], fp);
            fprintf(fp, ")");
        } else if (op->opty == "shr") {
            fprintf(fp, "BVLSHR(");  // Logical shift right
            outputCVC(op->val[0], fp);
            fprintf(fp, ", ");
            outputCVC(op->val[1], fp);
            fprintf(fp, ")");
        } else if (op->opty == "sar") {  // Support for arithmetic shift right
            fprintf(fp, "BVASHR(");
            outputCVC(op->val[0], fp);
            fprintf(fp, ", ");
            outputCVC(op->val[1], fp);
            fprintf(fp, ")");
        } else if (op->opty == "inc") {
            fprintf(fp, "BVPLUS(64, ");
            outputCVC(op->val[0], fp);
            fprintf(fp, ", 0hex0000000000000001)");
        } else if (op->opty == "dec") {
            fprintf(fp, "BVSUB(64, ");
            outputCVC(op->val[0], fp);
            fprintf(fp, ", 0hex0000000000000001)");
        } else if (op->opty == "not") {
            fprintf(fp, "BVNOT(");
            outputCVC(op->val[0], fp);
            fprintf(fp, ")");
        } else {
            cerr << "Error: Instruction " << op->opty << " is not interpreted in CVC!" << endl;
        }
    }
}
// Output the calculation of the formula 'f' as a CVC formula
void outputCVCFormula(Value *f)
{
    const char *cvcfile = "formula.cvc";
    FILE *fp = fopen(cvcfile, "w");

    outputCVC(f, fp);

    fclose(fp);
}

// Output a CVC formula to check the equivalence of f1 and f2 using the variable mapping in 'm'
void outputChkEqCVC(Value *f1, Value *f2, map<int,int> *m)
{
    const char *cvcfile = "ChkEq.cvc";
    FILE *fp = fopen(cvcfile, "w");

    // Declare the variables as 64-bit BV (bit vectors)
    for (map<int,int>::iterator it = m->begin(); it != m->end(); ++it) {
        fprintf(fp, "sym%da: BV(64);\n", it->first);  // Use 64-bit bit vector
        fprintf(fp, "sym%db: BV(64);\n", it->second); // Use 64-bit bit vector
    }
    fprintf(fp, "\n");

    // Assert the equivalence between sym%da and sym%db
    for (map<int,int>::iterator it = m->begin(); it != m->end(); ++it) {
        fprintf(fp, "ASSERT(sym%da = sym%db);\n", it->first, it->second);
    }

    fprintf(fp, "\nQUERY(\n");
    sympostfix = "a";  // Add postfix to differentiate variables
    outputCVC(f1, fp); // Output formula for f1
    fprintf(fp, "\n=\n");
    sympostfix = "b";  // Add postfix to differentiate variables
    outputCVC(f2, fp); // Output formula for f2
    fprintf(fp, ");\n");

    // Request a counterexample for the equivalence check
    fprintf(fp, "COUNTEREXAMPLE;\n");

    fclose(fp);
}
// Output all bit formulas based on the result of variable mapping
void outputBitCVC(Value *f1, Value *f2, vector<Value*> *inv1, vector<Value*> *inv2,
                            list<FullMap> *result)
{
    int n = 1;
    for (list<FullMap>::iterator it = result->begin(); it != result->end(); ++it) {
        // Create a new formula for each mapping result
        string cvcfile = "formula" + to_string(n++) + ".cvc";
        FILE *fp = fopen(cvcfile.c_str(), "w");

        map<int, int> *inmap = &(it->first);  // Input variables mapping
        map<int, int> *outmap = &(it->second); // Output variables mapping

        // Output bit values of the inputs (64-bit now)
        for (int i = 0, max = 64 * inv1->size(); i < max; ++i) {
            fprintf(fp, "bit%da: BV(1);\n", i);
        }
        for (int i = 0, max = 64 * inv2->size(); i < max; ++i) {
            fprintf(fp, "bit%db: BV(1);\n", i);
        }

        // Output inputs mapping
        for (map<int, int>::iterator it = inmap->begin(); it != inmap->end(); ++it) {
            fprintf(fp, "ASSERT(bit%da = bit%db);\n", it->first, it->second);
        }
        fprintf(fp, "\n");

        fprintf(fp, "\nQUERY(\n");

        // Concatenate bits into a variable in the formula for input vectors
        for (int i = 0, max = inv1->size(); i < max; ++i) {
            fprintf(fp, "LET %sa = ", getValueName((*inv1)[i]).c_str());
            for (int j = 0; j < 63; ++j) { // 64-bit concatenation
                fprintf(fp, "bit%da@", i*64 + j);
            }
            fprintf(fp, "bit%da IN (\n", i + 63);
        }
        for (int i = 0, max = inv2->size(); i < max; ++i) {
            fprintf(fp, "LET %sb = ", getValueName((*inv2)[i]).c_str());
            for (int j = 0; j < 63; ++j) { // 64-bit concatenation
                fprintf(fp, "bit%db@", i*64 + j);
            }
            fprintf(fp, "bit%db IN (\n", i + 63);
        }

        // Output the calculation formulas for f1 and f2
        fprintf(fp, "LET out1 = ");
        sympostfix = "a";
        outputCVC(f1, fp);
        fprintf(fp, " IN (\n");

        fprintf(fp, "LET out2 = ");
        sympostfix = "b";
        outputCVC(f2, fp);
        fprintf(fp, " IN (\n");

        // Output the final equivalence check
        for (map<int, int>::iterator it = outmap->begin(); it != outmap->end(); ++it) {
            if (next(it, 1) != outmap->end()) {
                fprintf(fp, "out1[%d:%d] = out2[%d:%d] AND\n", it->first, it->first, it->second, it->second);
            } else {
                fprintf(fp, "out1[%d:%d] = out2[%d:%d]\n", it->first, it->first, it->second, it->second);
            }
        }

        // Closing brackets for LET expressions
        for (int i = 0, n = inv1->size() + inv2->size(); i < n; ++i) {
            fprintf(fp, ")");
        }
        fprintf(fp, ")));\n");

        // Request a counterexample to check for equivalence
        fprintf(fp, "COUNTEREXAMPLE;");

        fclose(fp);
    }
}
// Show inputs in memory
void SEEngine::showMemInput()
{
    cout << "Inputs in memory:" << endl;
    for (map<Value*, AddrRange>::iterator it = meminput.begin(); it != meminput.end(); ++it) {
        printf("sym%d: ", it->first->id);
        printf("[%lx, %lx]\n", it->second.first, it->second.second);  // Updated to print 64-bit addresses
    }
    cout << endl;
}
