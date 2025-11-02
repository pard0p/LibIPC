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
#include <stdio.h>
#include "IPC.h"

#define LINE_BUFFER_MAX (MAX_MESSAGE_SIZE - 8) // 8 = strlen("@START@")

typedef struct {
	__typeof__(LoadLibraryA)   * LoadLibraryA;
	__typeof__(GetProcAddress) * GetProcAddress;
} IMPORTFUNCS;

WINBASEAPI int WINAPI MSVCRT$fputc(int c, FILE* stream);

/* Global IPC instance for our hooks */
IpcInstance* g_ipc;

/* Temporary line buffer for fputc hook */
char  g_line_buffer[MAX_MESSAGE_SIZE];
DWORD g_line_buffer_len = 0;

int WINAPI _fputc(int c, FILE* stream) {
	if (g_ipc != NULL) {
		/* If newline or buffer full, send and clear */
		if (c == '\n' || g_line_buffer_len >= LINE_BUFFER_MAX) {
			if (g_line_buffer_len > 0) {
				g_line_buffer[g_line_buffer_len] = '\0';
				/* TODO: encrypt the buffer here */
				AddIpcMessage(g_ipc, g_line_buffer);
				g_line_buffer_len = 0;
			}
		} else if (c != '\r') {
			/* Add character to temporary buffer */
			g_line_buffer[g_line_buffer_len++] = (char)c;
		}
	}

	return c;
}

/*
 * This is the GetProcAddress our loader will use. It's a chance for us to hook things. Notice, we also use
 * this to replace any other references to GetProcAddress with our hook function too. In this way, for the
 * things our DLL does... everything will use this GetProcAddress giving us nice visibility into everything.
 */
char * WINAPI _GetProcAddress(HMODULE hModule, LPCSTR lpProcName) {
	char * result = (char *)GetProcAddress(hModule, lpProcName);

	if ((char *)GetProcAddress == result) {
		return (char *)_GetProcAddress;
	}
	/* I like this method of identifying hook targets, because there are no strings to fuss with. BUT,
	 * beware... LIBRARY$func WILL LoadLibrary the specified library, here USER32.dll when the PICO is
	 * loaded into memory. So, if that's not desireable, you may need to switch to using string hashes or
	 * something else to decide what to hook */
	if ((char *)MSVCRT$fputc == result) {
		return (char *)_fputc;
	}

	return result;
}

/* Our entry point and our initialization for everything fun that will happen... */
void go(IMPORTFUNCS * funcs, IpcInstance* ipc) {
	/* update the GetProcAddress our loader uses */
	funcs->GetProcAddress = (__typeof__(GetProcAddress) *)_GetProcAddress;

	/* Set our global ipc instance */
	g_ipc = ipc;
}
