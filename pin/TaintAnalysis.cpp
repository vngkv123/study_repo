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

int syscallInputList[] = {0, 19, 45, 47, 267};

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

static VOID RecordMem(VOID * ip, CHAR r, VOID * addr, INT32 size, BOOL isPrefetch)
{
	TaintObject *cur;
	if(gHeadTaintObject){
		for(cur = gHeadTaintObject; cur; cur = cur->next){
			if((char *)addr >= (char *)cur->start && (char *)addr <= (char *)cur->end)
				//printf("ip(%p) perm(%c) addr(%p) size(%d)\n", ip, r, addr, size);
			if (!isPrefetch){
				/* empty */
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

/*	stack variables layer

   0x0000000000400626 <+0>:	push   rbp
   0x0000000000400627 <+1>:	mov    rbp,rsp
   0x000000000040062a <+4>:	sub    rsp,0x130
   0x0000000000400631 <+11>:	mov    DWORD PTR [rbp-0x124],edi
   0x0000000000400637 <+17>:	mov    QWORD PTR [rbp-0x130],rsi
   0x000000000040063e <+24>:	mov    rax,QWORD PTR fs:0x28
   0x0000000000400647 <+33>:	mov    QWORD PTR [rbp-0x8],rax
   0x000000000040064b <+37>:	xor    eax,eax
   0x000000000040064d <+39>:	mov    DWORD PTR [rbp-0x11c],0xdeadbeef
   0x0000000000400657 <+49>:	mov    DWORD PTR [rbp-0x118],0xcafebabe
   0x0000000000400661 <+59>:	mov    DWORD PTR [rbp-0x114],0xc0dec0de
   0x000000000040066b <+69>:	lea    rax,[rbp-0x110]
   0x0000000000400672 <+76>:	mov    edx,0x100
   0x0000000000400677 <+81>:	mov    esi,0x41
   0x000000000040067c <+86>:	mov    rdi,rax
   0x000000000040067f <+89>:	call   0x400500 <memset@plt>

	Create Parent stack frame

*/

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
*/
void getSyscallArgs(CONTEXT *ctxt, SYSCALL_STANDARD std)
{
	for (int i = 0; i < 5; i++) {
		ADDRINT scargs = PIN_GetSyscallArgument(ctxt, std, i);
		fprintf(stdout, "arg%d: 0x%lx\n", i, scargs);
	}
}

/*
Retrieves the arguments of the sendto and recvfrom system calls. Dereferences then increments
the bufptr pointer to grab the value at each byte in the buffer.
*/
static bool headerChecker = false;

void getSyscallArgsVal(CONTEXT *ctxt, SYSCALL_STANDARD std)
{
	ADDRINT buf = PIN_GetSyscallArgument(ctxt, std, 1);
	ADDRINT len = PIN_GetSyscallArgument(ctxt, std, 2);
//	int buflen = (int)len;
//	char *bufptr = (char *)buf;
//	fprintf(stdout, "buffer start: 0x%lx\n", buf);
//	fprintf(stdout, "length: %d\n", buflen);

/* check boundry */

	for(TaintObject *tmp = gHeadTaintObject; tmp; tmp = tmp->next){
		if( (char *)buf >= tmp->start && (char *)buf <= tmp->end ){
			if( ((char *)buf + len) >= tmp->end ){
				printf("\n[-] Overflow Detection at (%p ~ %p)\n", tmp->start, tmp->end);
				exit(0);
			}
		}
	}

/* make linked list */
	TaintObject *tmpTaint;

	if( !gHeadTaintObject ){
		if(!headerChecker){
			headerChecker = true;
			return;
		}
		gHeadTaintObject = (TaintObject *)malloc(sizeof(TaintObject));
		tmpTaint = gHeadTaintObject;
	}
	else{
		TaintObject *cur = gHeadTaintObject;
		for(tmpTaint = cur; tmpTaint->next; tmpTaint = tmpTaint->next);
		tmpTaint->next = (TaintObject *)malloc(sizeof(TaintObject));
		tmpTaint = tmpTaint->next;
	}

	tmpTaint->start = (void *)buf;
	tmpTaint->end = (void *)((char *)buf + len);

	printf("[-] Taint start : %p\n", tmpTaint->start);
	printf("[-] Taint end : %p\n", tmpTaint->end);
/*
	for (int i = 0; i < buflen; i++, bufptr++) {
		fprintf(stdout, "%c", *bufptr);
	}
	fprintf(stdout, "\n");
*/
}

void _SYSCALL_ENTRY_CALLBACK(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
	ADDRINT scnum = PIN_GetSyscallNumber(ctxt, std);

	for(int i = 0; i < (int)sizeof(syscallInputList)/(int)4; i++){
		if((int)scnum == syscallInputList[i]){
//			fprintf(stdout, "systemcall read: %lu\n", scnum);
			getSyscallArgsVal(ctxt, std);
		}
	}
}

void _SYSCALL_EXIT_CALLBACK(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
	//ADDRINT retval = PIN_GetSyscallReturn(ctxt, std);
	//fprintf(stdout, "retval: %lu\n", retval);
}

VOID Fini(INT32 code, VOID *v)
{
	TaintObject *cur = gHeadTaintObject;
	for(TaintObject *i = cur; i; i = i->next){
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

VOID Arg1Before(CHAR * name, ADDRINT size)
{
	heapSize = (unsigned long)size;
//	std::cout << "enter(" << heapSize << ")" << std::endl;
}

VOID MallocAfter(ADDRINT ret)
{
	heapRet = (void *)ret;
//	std::cout << "ret(" << ret << ")" << std::endl;
	HeapVariable *cur = 0;
	if( !gHeapVariable ){
		gHeapVariable = (HeapVariable *)malloc(sizeof(HeapVariable));
		gHeapVariable->start = heapRet;
		heapSize = *(unsigned long *)((char *)gHeapVariable->start - 0x8) & ~1;
		printf("malloc size : %#lx(%p)\n", heapSize, (void *)heapRet);
		gHeapVariable->end = (void *)((char *)heapRet + heapSize - 0x10);
	}
	else{
		for( cur = gHeapVariable; cur->next; cur = cur->next );
		cur->next = (HeapVariable *)malloc(sizeof(HeapVariable));
		cur = cur->next;
		cur->start = heapRet;
		heapSize = *(unsigned long *)((char *)cur->start - 0x8) & ~1;
		printf("malloc size : %#lx(%p)\n", heapSize, (void *)heapRet);
		cur->end = (void *)((char *)heapRet + heapSize - 0x10);
	}
}

VOID Image(IMG img, VOID *v)
{
    RTN mallocRtn = RTN_FindByName(img, MALLOC);
    if (RTN_Valid(mallocRtn))
    {
        RTN_Open(mallocRtn);
        RTN_InsertCall(mallocRtn, IPOINT_BEFORE, (AFUNPTR)Arg1Before, IARG_ADDRINT, MALLOC, IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_END);
        RTN_InsertCall(mallocRtn, IPOINT_AFTER, (AFUNPTR)MallocAfter, IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);
        RTN_Close(mallocRtn);
    }

    RTN freeRtn = RTN_FindByName(img, FREE);
    if (RTN_Valid(freeRtn))
    {
        RTN_Open(freeRtn);
        RTN_InsertCall(freeRtn, IPOINT_BEFORE, (AFUNPTR)Arg1Before, IARG_ADDRINT, FREE, IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_END);
        RTN_Close(freeRtn);
    }
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

	IMG_AddInstrumentFunction(Image, 0);	/* for trace *alloc() [TODO]*/
	INS_AddInstrumentFunction(Instruction, 0);		/* set watcher */
	PIN_AddFiniFunction(Fini, 0);
	RTN_AddInstrumentFunction(Rtn, 0);		/* for construct stackframe [TODO]*/
    PIN_StartProgram();

    return(0);
}
