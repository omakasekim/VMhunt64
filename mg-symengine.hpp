#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <set>
#include <iterator>

using namespace std;

struct Operation;
struct Value;
struct FullMap;

// Alias for 64-bit address ranges
typedef pair<ADDR64, ADDR64> AddrRange;

// Symbolic execution engine class
class SEEngine {
private:
    // Context for registers (mapping register names to Value pointers)
    map<string, Value*> ctx;
    
    // Instruction iterators for execution range
    list<Inst>::iterator start;
    list<Inst>::iterator end;
    list<Inst>::iterator ip;

    // Memory model
    map<AddrRange, Value*> mem;              // Memory values
    map<Value*, AddrRange> meminput;         // Memory input values
    map<Value*, string> reginput;            // Register input values

    // Helper functions for memory operations
    bool memfind(AddrRange ar);
    bool memfind(ADDR64 b, ADDR64 e);
    bool isnew(AddrRange ar);
    bool issubset(AddrRange ar, AddrRange *superset);
    bool issuperset(AddrRange ar, AddrRange *subset);

    // Read/write operations for registers and memory
    Value* readReg(string &s);
    void writeReg(string &s, Value *v);
    Value* readMem(ADDR64 addr, int nbyte);
    void writeMem(ADDR64 addr, int nbyte, Value *v);

    // Register/concrete value utilities
    ADDR64 getRegConVal(string reg);
    ADDR64 calcAddr(Operand *opr);

    // Debugging and output helpers
    void printformula(Value* v);
    
public:
    // Constructor initializing register context
    SEEngine() {
        ctx = { {"rax", NULL}, {"rbx", NULL}, {"rcx", NULL}, {"rdx", NULL},
                {"rsi", NULL}, {"rdi", NULL}, {"rsp", NULL}, {"rbp", NULL},
                {"r8", NULL}, {"r9", NULL}, {"r10", NULL}, {"r11", NULL},
                {"r12", NULL}, {"r13", NULL}, {"r14", NULL}, {"r15", NULL} };
    };

    // Initialization functions
    void init(Value *v1, Value *v2, Value *v3, Value *v4,
              Value *v5, Value *v6, Value *v7, Value *v8,
              list<Inst>::iterator it1,
              list<Inst>::iterator it2);
    void init(list<Inst>::iterator it1,
              list<Inst>::iterator it2);
    void initAllRegSymol(list<Inst>::iterator it1,
                         list<Inst>::iterator it2);

    // Core symbolic execution function
    int symexec();

    // Concrete execution with given input mapping
    ADDR64 conexec(Value *f, map<Value*, ADDR64> *input);

    // Output and debugging functions
    void outputFormula(string reg);
    void dumpreg(string reg);
    void printAllRegFormulas();
    void printAllMemFormulas();
    void printInputSymbols(string output);
    Value* getValue(string s) { return ctx[s]; }
    vector<Value*> getAllOutput();
    void showMemInput();
    void printMemFormula(ADDR64 addr1, ADDR64 addr2);
};

// External functions for CVC and bit-vector handling
void outputCVCFormula(Value *f);
void outputChkEqCVC(Value *f1, Value *f2, map<int, int> *m);
void outputBitCVC(Value *f1, Value *f2, vector<Value*> *inv1, vector<Value*> *inv2,
                  list<FullMap> *result);
map<Value*, ADDR64> buildinmap(vector<Value*> *vv, vector<ADDR64> *input);
vector<Value*> getInputVector(Value *f); // Get formula f's inputs as a vector
string getValueName(Value *v);

