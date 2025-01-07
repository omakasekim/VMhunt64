#ifndef CORE_HPP
#define CORE_HPP

#include <cstdint>
#include <bitset>
#include <string>
#include <vector>
#include <utility>
#include <map>

typedef uint64_t ADDR64;
typedef std::pair<ADDR64, ADDR64> AddrRange;

enum Register {
     // General-purpose 64-bit registers
     RAX, RBX, RCX, RDX,
     RSI, RDI, RSP, RBP,
     R8,  R9,  R10, R11,
     R12, R13, R14, R15,

     // General-purpose 32-bit registers
     EAX, EBX, ECX, EDX,
     ESI, EDI, ESP, EBP,
     R8D, R9D, R10D, R11D,
     R12D, R13D, R14D, R15D,

     // General-purpose 16-bit registers
     AX, BX, CX, DX,
     SI, DI, BP, SP,
     R8W, R9W, R10W, R11W,
     R12W, R13W, R14W, R15W,

     // General-purpose 8-bit registers (low)
     AL, BL, CL, DL,
     SIL, DIL, BPL, SPL,
     R8B, R9B, R10B, R11B,
     R12B, R13B, R14B, R15B,

     // General-purpose 8-bit registers (high)
     AH, BH, CH, DH,

     // FPU registers
     ST0, ST1, ST2, ST3,
     ST4, ST5,

     // Segment registers
     CS, DS, ES, FS, GS, SS,

     // Unknown register
     UNK
};


struct Operand {
     enum Type { IMM, REG, MEM };
     Type ty;
     int tag;
     int bit;
     bool issegaddr;
     std::string segreg;             // For seg mem access like fs:[0x1]
     std::string field[5];

     Operand() : bit(0), issegaddr(false) {}
};

struct Parameter {
     enum Type { IMM, REG, MEM };
     Type ty;
     Register reg;
     ADDR64 idx;

     bool operator==(const Parameter &other);
     bool operator<(const Parameter &other) const;
     bool isIMM();
     void show() const;
};

struct Inst {
     int id;                    // Unique instruction ID
     std::string addr;               // Instruction address: string
     ADDR64 addrn;        // Instruction address: unsigned number
     std::string assembly;           // Assembly code, including opcode and operands: string
     int opc;                   // Opcode: number
     std::string opcstr;             // Opcode: string
     std::vector<std::string> oprs;       // Operands: string
     int oprnum;                // Number of operands
     Operand *oprd[3];          // Parsed operands
     ADDR64 ctxreg[16];         // Context registers (support for 64-bit and 32-bit)
     ADDR64 raddr;              // Read memory address
     ADDR64 waddr;              // Write memory address

     std::vector<Parameter> src;     // Source parameters
     std::vector<Parameter> dst;     // Destination parameters
     std::vector<Parameter> src2;    // src and dst for extra dependency such as in xchg
     std::vector<Parameter> dst2;

     void addsrc(Parameter::Type t, std::string s);
     void addsrc(Parameter::Type t, AddrRange a);
     void adddst(Parameter::Type t, std::string s);
     void adddst(Parameter::Type t, AddrRange a);
     void addsrc2(Parameter::Type t, std::string s);
     void addsrc2(Parameter::Type t, AddrRange a);
     void adddst2(Parameter::Type t, std::string s);
     void adddst2(Parameter::Type t, AddrRange a);
};

typedef std::pair<std::map<int, int>, std::map<int, int>> FullMap;

std::string reg2string(Register reg);

#endif