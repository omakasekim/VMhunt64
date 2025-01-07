
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
