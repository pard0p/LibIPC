/*
 * Copyright 2025 Raphael Mudge, Adversary Fan Fiction Writers Guild
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <windows.h>
#include "tcg.h"
#include "IPC.h"

WINBASEAPI LPVOID WINAPI KERNEL32$VirtualAlloc (LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);

/*
 * This is our opt-in Dynamic Function Resolution resolver. It turns MODULE$Function into pointers.
 * See dfr "resolve" in loader.spec
 */
char * resolve(DWORD modHash, DWORD funcHash) {
	char * hModule = (char *)findModuleByHash(modHash);
	return findFunctionByHash(hModule, funcHash);
}

/*
 * This is our opt-in function to help fix ptrs in x86 PIC. See fixptrs _caller" in loader.spec
 */
#ifdef WIN_X86
__declspec(noinline) ULONG_PTR caller( VOID ) { return (ULONG_PTR)WIN_GET_CALLER(); }
#endif

/*
 * This is the Crystal Palace convention for getting ahold of data linked with this loader.
 */
char __DLLDATA__[0] __attribute__((section("my_data")));
char __BOFDATA__[0] __attribute__((section("my_bof")));
char __HOKDATA__[0] __attribute__((section("my_hooks")));

char * findAppendedDLL() {
	return (char *)&__DLLDATA__;
}

char * findAppendedPICO() {
	return (char *)&__BOFDATA__;
}

char * findAppendedHOOKS() {
	return (char *)&__HOKDATA__;
}

char * SetupCOFF(IMPORTFUNCS * funcs, char * dstCode, char * srcData) {
	char * dstData;

	/* allocate memory for our PICO's data */
	dstData = KERNEL32$VirtualAlloc( NULL, PicoDataSize(srcData), MEM_RESERVE|MEM_COMMIT|MEM_TOP_DOWN, PAGE_READWRITE );

	/* In this project, we're pre-allocating 4096b for the .text section of each of our PICOs in-lin with our DLL.
	 * But, if that's not enough, well, we'll allocate more data then and stick everything there. */
	if (PicoCodeSize(srcData) > 4096)
		dstCode = KERNEL32$VirtualAlloc( NULL, PicoCodeSize(srcData), MEM_RESERVE|MEM_COMMIT|MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE );

	/* load our pico into our destination address, thanks! */
	PicoLoad(funcs, srcData, dstCode, dstData);

	return dstCode;
}

/*
 * A quick COFF runner for hooks
 */
typedef void (*PICOHOOK_FUNC_3)(IMPORTFUNCS * funcs, IpcInstance* ipc);

void RunHookCOFF(IMPORTFUNCS * funcs, char * dstCode, IpcInstance* ipc) {
	char * src = findAppendedHOOKS();
	char * dst = SetupCOFF(funcs, dstCode, src);

	/* execute our pico */
	((PICOHOOK_FUNC_3)PicoEntryPoint(src, dst)) (funcs, ipc);

	/* we don't free this COFF because we want it to persist in memory, after all, it's modified
	 * our funcs->GetProcAddress to affect everything that's loaded from here on out */
}

/*
 * Get the start address of our PIC DLL loader.
 */
void go();

char * getStart() {
	return (char *)go;
}

/*
 * Our PICO loader, have fun, go nuts!
 */
typedef void (*PICOMAIN_FUNC_3)(char * loader, char * dllEntry, char * dllBase, IpcInstance* ipc);

void RunViaFreeCOFF(IMPORTFUNCS * funcs, char * dstCode, char * dllEntry, char *dllBase, IpcInstance* ipc) {
	char * src = findAppendedPICO();
	char * dst = SetupCOFF(funcs, dstCode, src);

	/* execute our pico */
	((PICOMAIN_FUNC_3)PicoEntryPoint(src, dst)) (getStart(), dllEntry, dllBase, ipc);

	/* Same thing as above, we don't care about free()ing this, because it's persistent */
}

/* This is how we're going to layout the DLL data and COFFs. The .text sections of our COFFs
 * are CRAZY small here. I'm giving each of them one page though. */
typedef struct {
	char freeandrun[4096];
	char hooks[4096];
	char dllbase[0];
} LAYOUT;

/*
 * Our reflective loader itself, have fun, go nuts!
 */
void go() {
	char       * src;
	DLLDATA      data;
	IMPORTFUNCS  funcs;
	LAYOUT     * dst;

	/* find our DLL appended to this PIC */
	src = findAppendedDLL();

	/* resolve the functions we'll need */
	funcs.GetProcAddress = GetProcAddress;
	funcs.LoadLibraryA   = LoadLibraryA;

	/* parse our DLL! */
	ParseDLL(src, &data);

	/* allocate memory for our DLL and the other stuff within our layout.  */
	dst = (LAYOUT *)KERNEL32$VirtualAlloc( NULL, sizeof(LAYOUT) + SizeOfDLL(&data), MEM_RESERVE|MEM_COMMIT|MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE );

	IpcInstance ipc = {0};

	/* Before we go ANY further, let's run our COFF to setup our hooks */
	RunHookCOFF(&funcs, dst->hooks, &ipc);

	/* load the damned thing */
	LoadDLL(&data, src, dst->dllbase);

	/* process the imports */
	ProcessImports(&funcs, &data, dst->dllbase);

	/* pass to our BOF */
	RunViaFreeCOFF(&funcs, dst->freeandrun, (char *)EntryPoint(&data, dst->dllbase), dst->dllbase, &ipc);
}
