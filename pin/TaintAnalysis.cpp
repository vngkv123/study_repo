#include <iostream>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "pin.H"

/* For Follow Taint Object -> Untrusted input */

#define _CK_read    0
#define _CK_write   1

unsigned long heapSize = 0;
void *heapRet = 0;

int syscallInputList[] = {0, 45, 267};
int vectorIO[] = {19, 47};

typedef struct TaintObject
{
    unsigned int id;
    void *start;
    void *end;
    TaintObject *next;
} TaintObject;

typedef struct Stack
{
    /* rbp base address */
    void *start;
    /* current top stack */
    void *end;
    Stack *next;
} Stack;

typedef struct StackLocalVariable
{
    bool isTainted;
    unsigned int size;
    void *start;
    void *end;
    StackLocalVariable *next;
} StackLocalVariable;

typedef struct HeapVariable
{
    bool isTainted;
    unsigned int size;
    void *start;
    void *end;
    HeapVariable *next;
} HeapVariable;

TaintObject *gHeadTaintObject;
Stack *gHeadStack;
StackLocalVariable *gHeadStackLocalVariable;
HeapVariable *gHeapVariable;

/* R/W watcher */
/* 1byte read (gets, fgets, etc)... */
unsigned int IOcount = 0;
void *prevBuffer = 0;
bool byteWatcher = false;
bool libFunc = false;
HeapVariable *libcur;
bool check = false;

static VOID RecordMem(VOID * ip, CHAR r, VOID * addr, INT32 size, BOOL isPrefetch)
{
        TaintObject *cur;
        if(gHeadTaintObject){
                for(cur = gHeadTaintObject; cur; cur = cur->next){
                        if((char *)addr >= (char *)cur->start && (char *)addr <= (char *)cur->end){
                                //printf("ip(%p) perm(%c) addr(%p) size(%d)\n", ip, r, addr, size);
                        }
                        if (!isPrefetch){

                        }
                }
        }

        HeapVariable *cur2;
        if(gHeapVariable){
                for(cur2 = gHeapVariable; cur2; cur2 = cur2->next){
                        if((char *)addr >= (char *)cur2->start && (char *)addr <= (char *)cur2->end)
                                //printf("ip(%p) perm(%c) addr(%p) size(%d)\n", ip, r, addr, size);
                        if (!isPrefetch){

                        }
                }
        }

        if(byteWatcher){
                if( libFunc && libcur ){
                        if( (char *)addr == (char *)libcur->start + 1 )
                                check = true;
                        if( (char *)addr >= (char *)libcur->start && (char *)addr <= (char *)libcur->end + 1 && check ){
                                if( (char *)libcur->end + 1 == (char *)addr){
                                        printf("%p ~ %p\n", libcur->start, libcur->end);
                                        printf("[-] Overflow detection at %p %d\n\n", (void *)addr, (int)size);
                                        exit(0);
                                }
                        }
                }
                else{
                        for(cur2 = gHeapVariable; cur2; cur2 = cur2->next){
                if((char *)addr == (char *)cur2->start && (char *)addr <= (char *)cur2->end){
                                        libcur = cur2;
                                        libFunc = true;
                                        break;
                                }
                        }
                }
        }

}

static VOID * WriteAddr;
static INT32 WriteSize;

static VOID RecordWriteAddrSize(VOID * addr, INT32 size)
{
    WriteAddr = addr;
    WriteSize = size;
}


static VOID RecordMemWrite(VOID * ip)
{
    RecordMem(ip, 'W', WriteAddr, WriteSize, false);
}

VOID Instruction(INS ins, VOID *v)
{

    // instruments loads using a predicated call, i.e.
    // the call happens iff the load will be actually executed

    if (INS_IsMemoryRead(ins) && INS_IsStandardMemop(ins))
    {
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, (AFUNPTR)RecordMem,
            IARG_INST_PTR,
            IARG_UINT32, 'R',
            IARG_MEMORYREAD_EA,
            IARG_MEMORYREAD_SIZE,
            IARG_BOOL, INS_IsPrefetch(ins),
            IARG_END);
    }

    if (INS_HasMemoryRead2(ins) && INS_IsStandardMemop(ins))
    {
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, (AFUNPTR)RecordMem,
            IARG_INST_PTR,
            IARG_UINT32, 'R',
            IARG_MEMORYREAD2_EA,
            IARG_MEMORYREAD_SIZE,
            IARG_BOOL, INS_IsPrefetch(ins),
            IARG_END);
    }

    // instruments stores using a predicated call, i.e.
    // the call happens iff the store will be actually executed
    if (INS_IsMemoryWrite(ins) && INS_IsStandardMemop(ins))
    {
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, (AFUNPTR)RecordWriteAddrSize,
            IARG_MEMORYWRITE_EA,
            IARG_MEMORYWRITE_SIZE,
            IARG_END);

        if (INS_HasFallThrough(ins))
        {
            INS_InsertCall(
                ins, IPOINT_AFTER, (AFUNPTR)RecordMemWrite,
                IARG_INST_PTR,
                IARG_END);
        }
        if (INS_IsBranchOrCall(ins))
        {
            INS_InsertCall(
                ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)RecordMemWrite,
                IARG_INST_PTR,
                IARG_END);
        }

    }
}

/* watcher end */

/* RTN -> Make Local stack structure */

static VOID Rtn(RTN rtn, VOID *v)
{
/*
    if (!RTN_Valid(rtn) || RTN_Name(rtn) != watch_rtn)
    {
        return;
    }
*/
    //printf("Rtn Instrumenting %s\n", RTN_Name(rtn).c_str());
    RTN_Open(rtn);
    INS ins = RTN_InsHeadOnly(rtn);
    ASSERTX (INS_Valid(ins));
/*
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(Emit),
                             IARG_PTR, "RTN instrumentation", IARG_CALL_ORDER, CALL_ORDER_FIRST+1, IARG_END);
*/
    RTN_Close(rtn);
}

/* RTN end */


/*
Retrieves the value of registers with the current context.
*/
void getContext(CONTEXT *ctxt)
{
        fprintf(stdout, "rax: 0x%lx\nrbx: 0x%lx\nrcx: 0x%lx\nrdx: 0x%lx\nrsp: 0x%lx\nrbp: 0x%lx\nrsi: 0x%lx\nrdi: 0x%lx\nr8: 0x%lx\nr9: 0x%lx\n",
        PIN_GetContextReg(ctxt, REG_RAX),
        PIN_GetContextReg(ctxt, REG_RBX),
        PIN_GetContextReg(ctxt, REG_RCX),
        PIN_GetContextReg(ctxt, REG_RDX),
        PIN_GetContextReg(ctxt, REG_RSP),
        PIN_GetContextReg(ctxt, REG_RBP),
        PIN_GetContextReg(ctxt, REG_RSI),
        PIN_GetContextReg(ctxt, REG_RDI),
        PIN_GetContextReg(ctxt, REG_R8),
        PIN_GetContextReg(ctxt, REG_R9));
}

/*
Retrieves the arguments of a system call.
During input syscall, set lock mechanism
*/

ADDRINT buf;
ADDRINT len;

void getSyscallArgs(CONTEXT *ctxt, SYSCALL_STANDARD std)
{
        for (int i = 0; i < 5; i++) {
                ADDRINT scargs = PIN_GetSyscallArgument(ctxt, std, i);
                fprintf(stdout, "arg%d: 0x%lx\n", i, scargs);
        }
}

void syscallEntryBuffer(CONTEXT *ctxt, SYSCALL_STANDARD std)
{
        buf = PIN_GetSyscallArgument(ctxt, std, 1);
        len = PIN_GetSyscallArgument(ctxt, std, 2);

/* check boundry */

        HeapVariable *cur;
        if( gHeapVariable ){
                for( cur = gHeapVariable; cur; cur = cur->next ){
                        if( (char *)buf == (char *)cur->start && (char *)buf + len > (char *)cur->end ){
                                printf("\n\n[-] %p ~ %p\n", (void *)cur->start, (void *)cur->end);
                                printf("[-] Maybe vulnerability exist (%p(%d))\n\n", (void *)buf, (int)len);
                                //exit(0);
                        }
                }
        }
}

void syscallSanityCheck(ADDRINT retval)
{
    HeapVariable *cur;
    if( gHeapVariable ){
        for( cur = gHeapVariable; cur; cur = cur->next ){
            if( (char *)buf == (char *)cur->start && (char *)buf + retval > (char *)cur->end ){
                printf("\n\n[-] %p ~ %p\n", (void *)cur->start, (void *)cur->end);
                printf("[-] Overflow detection on heap(%p(%d))\n\n", (void *)buf, (int)retval);
                exit(0);
            }
        }
    }
}

void _SYSCALL_ENTRY_CALLBACK(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
        ADDRINT scnum = PIN_GetSyscallNumber(ctxt, std);

        for(int i = 0; i < (int)sizeof(syscallInputList)/(int)4; i++){
                if((int)scnum == syscallInputList[i]){
                        IOcount++;
                        syscallEntryBuffer(ctxt, std);
                }
        }
}

void _SYSCALL_EXIT_CALLBACK(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
        ADDRINT retval = PIN_GetSyscallReturn(ctxt, std);
        syscallSanityCheck(retval);
}

VOID Fini(INT32 code, VOID *v)
{
/*
        TaintObject *cur = gHeadTaintObject;
        for(TaintObject *i = cur; i; i = i->next){
                printf("[-] start : %p\n", i->start);
                printf("[-] end : %p\n", i->end);
                printf("\n");
        }
*/
        HeapVariable *cur2 = gHeapVariable;

/* logging for Heap struct */

        for(HeapVariable *i = cur2; i; i = i->next){
                printf("[-] start : %p\n", i->start);
                printf("[-] end : %p\n", i->end);
                printf("\n");
        }
}

/* Image for trace *alloc */
#if defined(TARGET_MAC)
#define MALLOC "_malloc"
#define FREE "_free"
#else
#define MALLOC "malloc"
#define FREE "free"
#endif
#define GETS "gets"
#define CALLOC "calloc"

unsigned int hc = 0;
bool heapCheck = true;
unsigned int heapSizeList[100];

VOID MallocBefore(CHAR * name, ADDRINT size)
{
        if(heapCheck){
                heapSizeList[hc] = (unsigned int)size;
                hc++;
        }
        heapSize = (unsigned long)size;
}

VOID CallocBefore(ADDRINT size, ADDRINT size2)
{
    if(heapCheck){
        heapSizeList[hc] = (unsigned int)size * (unsigned int)size2;
        hc++;
    }
    heapSize = (unsigned long)size * (unsigned int)size2;
}


void heapUnlink(HeapVariable *unlinkDst)
{
        HeapVariable *cur;
        for( cur = gHeapVariable; cur; cur = cur->next ){
                if(     unlinkDst == gHeapVariable ){
                        if( gHeapVariable->next )
                                gHeapVariable = cur->next;
                        else
                                gHeapVariable = 0;
                }
                if( unlinkDst == cur->next ){
                        if( cur->next->next )
                                cur->next = cur->next->next;
                        else
                                cur->next = 0;
                }
        }
}

VOID FreeBefore(CHAR * name, ADDRINT fptr)
{
        HeapVariable *cur;
        for( cur = gHeapVariable; cur; cur = cur->next ){
                if(     (char *)fptr == cur->start ){
                        printf("Free %p\n", (void *)fptr);
                        heapUnlink(cur);
                }
        }
}

VOID MallocAfter(ADDRINT ret)
{
        heapCheck = false;
        heapRet = (void *)ret;
        HeapVariable *cur = 0;
        if( !gHeapVariable ){
                gHeapVariable = (HeapVariable *)malloc(sizeof(HeapVariable));
                gHeapVariable->start = heapRet;
                heapSize = *(unsigned long *)((char *)gHeapVariable->start - 0x8) & ~1;
                gHeapVariable->end = (void *)((char *)heapRet + heapSize - 0x10);

/* First heap size alignment */
                heapSize = heapSizeList[hc-2];
                if( heapSize <= 0x18 )
                        heapSize = 0x20;
                else{
                        if((heapSize & 15) > 8){
                                heapSize = (heapSize / 0x10) * (heapSize / 0x10 + 2);
                        }
                        else{
                                heapSize = heapSize & ~7;
                        }
                }
                cur = (HeapVariable *)malloc(sizeof(HeapVariable));
                cur->start = (HeapVariable *)((char *)gHeapVariable->start - heapSize - 0x10);
                cur->end = (HeapVariable *)((char *)gHeapVariable->start - 0x10);
                cur->next = gHeapVariable;
                gHeapVariable = cur;
        }
        else{
                for( cur = gHeapVariable; cur->next; cur = cur->next );
                cur->next = (HeapVariable *)malloc(sizeof(HeapVariable));
                cur = cur->next;
                cur->start = heapRet;
                heapSize = *(unsigned long *)((char *)cur->start - 0x8) & ~7;
                cur->end = (void *)((char *)heapRet + heapSize - 0x10);
        }
}

VOID onebyteCallback(ADDRINT ret){
        byteWatcher = true;
}

VOID Image(IMG img, VOID *v)
{
        byteWatcher = false;
        libFunc = 0;
        libcur = 0;
        check = false;
    RTN mallocRtn = RTN_FindByName(img, MALLOC);
    if (RTN_Valid(mallocRtn))
    {
        RTN_Open(mallocRtn);
        RTN_InsertCall(mallocRtn, IPOINT_BEFORE, (AFUNPTR)MallocBefore, IARG_ADDRINT, MALLOC, IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_END);
        RTN_InsertCall(mallocRtn, IPOINT_AFTER, (AFUNPTR)MallocAfter, IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);
        RTN_Close(mallocRtn);
    }

    RTN callocRtn = RTN_FindByName(img, CALLOC);
    if (RTN_Valid(callocRtn))
    {
        RTN_Open(callocRtn);

        // Instrument callocRtn to print the input argument value and the return value.
        RTN_InsertCall(callocRtn, IPOINT_BEFORE, (AFUNPTR)CallocBefore,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                       IARG_END);
        RTN_InsertCall(callocRtn, IPOINT_AFTER, (AFUNPTR)MallocAfter,
                       IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);

        RTN_Close(callocRtn);
    }

    RTN freeRtn = RTN_FindByName(img, FREE);
    if (RTN_Valid(freeRtn))
    {
        RTN_Open(freeRtn);
        RTN_InsertCall(freeRtn, IPOINT_BEFORE, (AFUNPTR)FreeBefore, IARG_ADDRINT, FREE, IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_END);
        RTN_Close(freeRtn);
    }

        RTN getsRtn = RTN_FindByName(img, GETS);
    if (RTN_Valid(getsRtn))
    {
        RTN_Open(getsRtn);
        RTN_InsertCall(getsRtn, IPOINT_BEFORE, (AFUNPTR)onebyteCallback, IARG_ADDRINT, GETS, IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_END);
        RTN_Close(getsRtn);
    }

/*
    RTN strcpyRtn = RTN_FindByName(img, "strcpy");
    if (RTN_Valid(strcpyRtn))
    {
        RTN_Open(strcpyRtn);
        RTN_InsertCall(strcpyRtn, IPOINT_BEFORE, (AFUNPTR)copyCallback, IARG_ADDRINT, "strcpy", IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_END);
        RTN_Close(strcpyRtn);
    }
*/
}

/* Image end */


int Usage()
{
        fprintf(stdout, "../../../pin -t obj-intel64/syscalltest.so -- sample program");
        return -1;
}

int32_t main(int32_t argc, char *argv[])
{
        PIN_InitSymbols();

    if (PIN_Init(argc, argv))
    {
        return Usage();
    }

    PIN_AddSyscallEntryFunction(&_SYSCALL_ENTRY_CALLBACK, NULL);
    PIN_AddSyscallExitFunction(&_SYSCALL_EXIT_CALLBACK, NULL);

        IMG_AddInstrumentFunction(Image, 0);    /* for trace *alloc() [TODO]*/
        INS_AddInstrumentFunction(Instruction, 0);              /* set watcher */
        PIN_AddFiniFunction(Fini, 0);
        RTN_AddInstrumentFunction(Rtn, 0);              /* for construct stackframe [TODO]*/
    PIN_StartProgram();

    return(0);
}
