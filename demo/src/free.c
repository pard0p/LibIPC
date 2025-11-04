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
#include "IPC.h"

WINBASEAPI DECLSPEC_NORETURN VOID WINAPI KERNEL32$ExitThread (DWORD dwExitCode);
WINBASEAPI WINBOOL WINAPI KERNEL32$VirtualFree (LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType);

typedef BOOL WINAPI (*DLLMAIN_FUNC)(HINSTANCE, DWORD, LPVOID);

void go(char * loader, char * dllEntry, char * dllBase, IpcInstance* ipc) {
	/* free our loader */
	KERNEL32$VirtualFree(loader, 0, MEM_RELEASE);

	/* Initialize our IPC channel */
	ipc->h_channel = InitializeIpcClient("async_channel", NULL);

	/* call the entry point of our Reflective DLL */
	((DLLMAIN_FUNC)dllEntry)((HINSTANCE)dllBase, DLL_PROCESS_ATTACH, NULL);

	/* Close our IPC channel */
	CloseIpcClient(ipc);

	/* exit the current thread.. else... we return to our free'd() memory and we don't want that. */
	KERNEL32$ExitThread(0);
}
