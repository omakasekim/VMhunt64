/*
 * A Pin tool to record all instructions in a binary execution (64-bit version).
 */

#include <stdio.h>
#include <pin.H>
#include <map>
#include <iostream>

// Output trace file
const char *tracefile = "instrace64.txt";
std::map<ADDRINT, string> opcmap;
FILE *fp;

// Capture and log context information for each instruction
void getctx(ADDRINT addr, CONTEXT *fromctx, ADDRINT raddr, ADDRINT waddr)
{
    fprintf(fp, "%llx;%s;", (unsigned long long)addr, opcmap[addr].c_str());
    
    // General-purpose registers (64-bit)
    fprintf(fp, "%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx,",
            (unsigned long long)PIN_GetContextReg(fromctx, REG_RAX),
            (unsigned long long)PIN_GetContextReg(fromctx, REG_RBX),
            (unsigned long long)PIN_GetContextReg(fromctx, REG_RCX),
            (unsigned long long)PIN_GetContextReg(fromctx, REG_RDX),
            (unsigned long long)PIN_GetContextReg(fromctx, REG_RSI),
            (unsigned long long)PIN_GetContextReg(fromctx, REG_RDI),
            (unsigned long long)PIN_GetContextReg(fromctx, REG_RSP),
            (unsigned long long)PIN_GetContextReg(fromctx, REG_RBP),
            (unsigned long long)PIN_GetContextReg(fromctx, REG_R8),
            (unsigned long long)PIN_GetContextReg(fromctx, REG_R9),
            (unsigned long long)PIN_GetContextReg(fromctx, REG_R10),
            (unsigned long long)PIN_GetContextReg(fromctx, REG_R11),
            (unsigned long long)PIN_GetContextReg(fromctx, REG_R12),
            (unsigned long long)PIN_GetContextReg(fromctx, REG_R13),
            (unsigned long long)PIN_GetContextReg(fromctx, REG_R14),
            (unsigned long long)PIN_GetContextReg(fromctx, REG_R15));

    // Memory read and write addresses (if applicable)
    fprintf(fp, "%llx,%llx,\n", (unsigned long long)raddr, (unsigned long long)waddr);
}

// Instrument instructions
static void instruction(INS ins, void *v)
{
    ADDRINT addr = INS_Address(ins);
    if (opcmap.find(addr) == opcmap.end()) {
        opcmap.insert(std::pair<ADDRINT, string>(addr, INS_Disassemble(ins)));
    }

    if (INS_IsMemoryRead(ins) && INS_IsMemoryWrite(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)getctx, IARG_INST_PTR, IARG_CONST_CONTEXT, IARG_MEMORYREAD_EA, IARG_MEMORYWRITE_EA, IARG_END);
    } else if (INS_IsMemoryRead(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)getctx, IARG_INST_PTR, IARG_CONST_CONTEXT, IARG_MEMORYREAD_EA, IARG_ADDRINT, 0, IARG_END);
    } else if (INS_IsMemoryWrite(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)getctx, IARG_INST_PTR, IARG_CONST_CONTEXT, IARG_ADDRINT, 0, IARG_MEMORYWRITE_EA, IARG_END);
    } else {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)getctx, IARG_INST_PTR, IARG_CONST_CONTEXT, IARG_ADDRINT, 0, IARG_ADDRINT, 0, IARG_END);
    }
}

// Finalization function
static void on_fini(int code, void *v)
{
    fclose(fp);
}

// Main function
int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv)) {
        fprintf(stderr, "Command line error\n");
        return 1;
    }

    fp = fopen(tracefile, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open trace file\n");
        return 1;
    }

    PIN_InitSymbols();

    PIN_AddFiniFunction(on_fini, 0);
    INS_AddInstrumentFunction(instruction, NULL);

    PIN_StartProgram(); // Never returns
    return 0;
}
