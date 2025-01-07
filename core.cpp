#include "core.hpp"
#include <iostream>

// Operator for comparison: equality
bool Parameter::operator==(const Parameter& other)
{
    if (ty == other.ty) {
        switch (ty) {
        case IMM:
            return idx == other.idx;
        case REG:
            return reg == other.reg && idx == other.idx;
        case MEM:
            return idx == other.idx;
        default:
            return false;
        }
    }
    return false;
}

// Operator for comparison: less-than
bool Parameter::operator<(const Parameter& other) const
{
    if (ty < other.ty)
        return true;
    else if (ty > other.ty)
        return false;
    else {
        switch (ty) {
        case IMM:
            return idx < other.idx;
        case REG:
            return (reg < other.reg) || (reg == other.reg && idx < other.idx);
        case MEM:
            return idx < other.idx;
        default:
            return false;
        }
    }
}

bool Parameter::isIMM()
{
    return ty == IMM;
}

std::string reg2string(Register reg) {
    switch (reg) {
    // 64-bit registers
    case RAX:  return "rax";   case RBX:  return "rbx";   case RCX:  return "rcx";   case RDX:  return "rdx";
    case RSI:  return "rsi";   case RDI:  return "rdi";   case RSP:  return "rsp";   case RBP:  return "rbp";
    case R8:   return "r8";    case R9:   return "r9";    case R10:  return "r10";   case R11:  return "r11";
    case R12:  return "r12";   case R13:  return "r13";   case R14:  return "r14";   case R15:  return "r15";

    // 32-bit registers
    case EAX:  return "eax";   case EBX:  return "ebx";   case ECX:  return "ecx";   case EDX:  return "edx";
    case ESI:  return "esi";   case EDI:  return "edi";   case ESP:  return "esp";   case EBP:  return "ebp";
    case R8D:  return "r8d";   case R9D:  return "r9d";   case R10D: return "r10d";  case R11D: return "r11d";
    case R12D: return "r12d";  case R13D: return "r13d";  case R14D: return "r14d";  case R15D: return "r15d";

    // 16-bit registers
    case AX:   return "ax";    case BX:   return "bx";    case CX:   return "cx";    case DX:   return "dx";
    case SI:   return "si";    case DI:   return "di";    case BP:   return "bp";    case SP:   return "sp";
    case R8W:  return "r8w";   case R9W:  return "r9w";   case R10W: return "r10w";  case R11W: return "r11w";
    case R12W: return "r12w";  case R13W: return "r13w";  case R14W: return "r14w";  case R15W: return "r15w";

    // 8-bit registers
    case AL:   return "al";    case BL:   return "bl";    case CL:   return "cl";    case DL:   return "dl";
    case AH:   return "ah";    case BH:   return "bh";    case CH:   return "ch";    case DH:   return "dh";
    case SIL:  return "sil";   case DIL:  return "dil";   case BPL:  return "bpl";   case SPL:  return "spl";
    case R8B:  return "r8b";   case R9B:  return "r9b";   case R10B: return "r10b";  case R11B: return "r11b";
    case R12B: return "r12b";  case R13B: return "r13b";  case R14B: return "r14b";  case R15B: return "r15b";

    default:   return "unknown";
    }
}

void Parameter::show() const
{
    if (ty == IMM) {
        printf("(IMM 0x%x) ", idx);
    } else if (ty == REG) {
        std::cout << "(REG " << reg2string(reg) << ") ";
    } else if (ty == MEM) {
        printf("(MEM 0x%x) ", idx);
    } else {
        std::cout << "Parameter show() error: unknown src type." << std::endl;
    }
}

bool isReg64(std::string reg) {
    return (reg == "rax" || reg == "rbx" || reg == "rcx" || reg == "rdx" ||
            reg == "rsi" || reg == "rdi" || reg == "rsp" || reg == "rbp" ||
            reg == "r8" || reg == "r9" || reg == "r10" || reg == "r11" ||
            reg == "r12" || reg == "r13" || reg == "r14" || reg == "r15");
}

bool isReg32(std::string reg) {
    return (reg == "eax" || reg == "ebx" || reg == "ecx" || reg == "edx" ||
            reg == "esi" || reg == "edi" || reg == "esp" || reg == "ebp" ||
            reg == "r8d" || reg == "r9d" || reg == "r10d" || reg == "r11d" ||
            reg == "r12d" || reg == "r13d" || reg == "r14d" || reg == "r15d");
}

bool isReg16(std::string reg) {
    return (reg == "ax" || reg == "bx" || reg == "cx" || reg == "dx" ||
            reg == "si" || reg == "di" || reg == "bp" || reg == "sp" ||
            reg == "r8w" || reg == "r9w" || reg == "r10w" || reg == "r11w" ||
            reg == "r12w" || reg == "r13w" || reg == "r14w" || reg == "r15w");
}

bool isReg8(std::string reg) {
    return (reg == "al" || reg == "bl" || reg == "cl" || reg == "dl" ||
            reg == "ah" || reg == "bh" || reg == "ch" || reg == "dh" ||
            reg == "sil" || reg == "dil" || reg == "bpl" || reg == "spl" ||
            reg == "r8b" || reg == "r9b" || reg == "r10b" || reg == "r11b" ||
            reg == "r12b" || reg == "r13b" || reg == "r14b" || reg == "r15b");
}

Register getRegParameter(std::string regname, std::vector<int> &idx)
{
    if (isReg64(regname)) {
        idx.push_back(0); idx.push_back(1); idx.push_back(2); idx.push_back(3);
        idx.push_back(4); idx.push_back(5); idx.push_back(6); idx.push_back(7);

        if (regname == "rax") return RAX;
        if (regname == "rbx") return RBX;
        if (regname == "rcx") return RCX;
        if (regname == "rdx") return RDX;
        if (regname == "rsi") return RSI;
        if (regname == "rdi") return RDI;
        if (regname == "rsp") return RSP;
        if (regname == "rbp") return RBP;
        if (regname == "r8")  return R8;
        if (regname == "r9")  return R9;
        if (regname == "r10") return R10;
        if (regname == "r11") return R11;
        if (regname == "r12") return R12;
        if (regname == "r13") return R13;
        if (regname == "r14") return R14;
        if (regname == "r15") return R15;
    }

    if (isReg32(regname)) {
        idx.push_back(0); idx.push_back(1); idx.push_back(2); idx.push_back(3);
        // Handle 32-bit registers
        if (regname == "eax") return EAX;
        if (regname == "ebx") return EBX;
        if (regname == "ecx") return ECX;
        if (regname == "edx") return EDX;
        if (regname == "esi") return ESI;
        if (regname == "edi") return EDI;
        if (regname == "esp") return ESP;
        if (regname == "ebp") return EBP;
        if (regname == "r8d") return R8D;
        if (regname == "r9d") return R9D;
        if (regname == "r10d") return R10D;
        if (regname == "r11d") return R11D;
        if (regname == "r12d") return R12D;
        if (regname == "r13d") return R13D;
        if (regname == "r14d") return R14D;
        if (regname == "r15d") return R15D;
    }

    if (isReg16(regname)) {
        idx.push_back(0); idx.push_back(1);
        // Handle 16-bit registers
        if (regname == "ax") return AX;
        if (regname == "bx") return BX;
        if (regname == "cx") return CX;
        if (regname == "dx") return DX;
        if (regname == "si") return SI;
        if (regname == "di") return DI;
        if (regname == "bp") return BP;
        if (regname == "sp") return SP;
        if (regname == "r8w") return R8W;
        if (regname == "r9w") return R9W;
        if (regname == "r10w") return R10W;
        if (regname == "r11w") return R11W;
        if (regname == "r12w") return R12W;
        if (regname == "r13w") return R13W;
        if (regname == "r14w") return R14W;
        if (regname == "r15w") return R15W;
    }

    if (isReg8(regname)) {
        idx.push_back(0);
        // Handle 8-bit registers
        if (regname == "al") return AL;
        if (regname == "bl") return BL;
        if (regname == "cl") return CL;
        if (regname == "dl") return DL;
        if (regname == "ah") return AH;
        if (regname == "bh") return BH;
        if (regname == "ch") return CH;
        if (regname == "dh") return DH;
        if (regname == "sil") return SIL;
        if (regname == "dil") return DIL;
        if (regname == "bpl") return BPL;
        if (regname == "spl") return SPL;
        if (regname == "r8b") return R8B;
        if (regname == "r9b") return R9B;
        if (regname == "r10b") return R10B;
        if (regname == "r11b") return R11B;
        if (regname == "r12b") return R12B;
        if (regname == "r13b") return R13B;
        if (regname == "r14b") return R14B;
        if (regname == "r15b") return R15B;
    }

    std::cout << "Unknown register: " << regname << std::endl;
    return UNK;
}

// Add source parameter: immediate or register
void Inst::addsrc(Parameter::Type t, std::string s)
{
    if (t == Parameter::IMM) {
        Parameter p;
        p.ty = t;
        p.idx = stoul(s, 0, 16);
        src.push_back(p);
    } else if (t == Parameter::REG) {
        std::vector<int> v;
        Register r = getRegParameter(s, v);
        for (int i = 0, max = v.size(); i < max; ++i) {
            Parameter p;
            p.ty = t;
            p.reg = r;
            p.idx = v[i];
            src.push_back(p);
        }
    } else {
        std::cout << "addsrc error!" << std::endl;
    }
}

// Add source parameter: memory
void Inst::addsrc(Parameter::Type t, AddrRange a)
{
    for (ADDR64 i = a.first; i <= a.second; ++i) {
        Parameter p;
        p.ty = t;
        p.idx = i;
        src.push_back(p);
    }
}

// Add destination parameter: register
void Inst::adddst(Parameter::Type t, std::string s)
{
    if (t == Parameter::REG) {
        std::vector<int> v;
        Register r = getRegParameter(s, v);
        for (int i = 0, max = v.size(); i < max; ++i) {
            Parameter p;
            p.ty = t;
            p.reg = r;
            p.idx = v[i];
            dst.push_back(p);
        }
    } else {
        std::cout << "adddst error!" << std::endl;
    }
}

// Add destination parameter: memory
void Inst::adddst(Parameter::Type t, AddrRange a)
{
    for (ADDR64 i = a.first; i <= a.second; ++i) {
        Parameter p;
        p.ty = t;
        p.idx = i;
        dst.push_back(p);
    }
}

// Add secondary source parameter
void Inst::addsrc2(Parameter::Type t, std::string s)
{
    if (t == Parameter::IMM) {
        Parameter p;
        p.ty = t;
        p.idx = stoul(s, 0, 16);
        src2.push_back(p);
    } else if (t == Parameter::REG) {
        std::vector<int> v;
        Register r = getRegParameter(s, v);
        for (int i = 0, max = v.size(); i < max; ++i) {
            Parameter p;
            p.ty = t;
            p.reg = r;
            p.idx = v[i];
            src2.push_back(p);
        }
    } else {
        std::cout << "addsrc2 error!" << std::endl;
    }
}

// Add secondary source parameter: memory
void Inst::addsrc2(Parameter::Type t, AddrRange a)
{
    for (ADDR64 i = a.first; i <= a.second; ++i) {
        Parameter p;
        p.ty = t;
        p.idx = i;
        src2.push_back(p);
    }
}

// Add secondary destination parameter
void Inst::adddst2(Parameter::Type t, std::string s)
{
    if (t == Parameter::REG) {
        std::vector<int> v;
        Register r = getRegParameter(s, v);
        for (int i = 0, max = v.size(); i < max; ++i) {
            Parameter p;
            p.ty = t;
            p.reg = r;
            p.idx = v[i];
            dst2.push_back(p);
        }
    } else {
        std::cout << "adddst2 error!" << std::endl;
    }
}

// Add secondary destination parameter: memory
void Inst::adddst2(Parameter::Type t, AddrRange a)
{
    for (ADDR64 i = a.first; i <= a.second; ++i) {
        Parameter p;
        p.ty = t;
        p.idx = i;
        dst2.push_back(p);
    }
}
