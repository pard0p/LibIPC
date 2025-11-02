#ifndef IPC_H
#define IPC_H

#include <windows.h>

/* ========================================================================== */
/* DYNAMIC FUNCTION RESOLUTION - MODULE$FUNCTION DECLARATIONS               */
/* ========================================================================== */

/* KERNEL32 Functions */
WINBASEAPI HANDLE WINAPI KERNEL32$CreateNamedPipeA(
    LPCSTR lpName,
    DWORD dwOpenMode,
    DWORD dwPipeMode,
    DWORD nMaxInstances,
    DWORD nOutBufferSize,
    DWORD nInBufferSize,
    DWORD nDefaultTimeOut,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
);

WINBASEAPI BOOL WINAPI KERNEL32$ConnectNamedPipe(
    HANDLE hNamedPipe,
    LPOVERLAPPED lpOverlapped
);

WINBASEAPI HANDLE WINAPI KERNEL32$CreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
);

WINBASEAPI BOOL WINAPI KERNEL32$SetNamedPipeHandleState(
    HANDLE hNamedPipe,
    LPDWORD lpMode,
    LPDWORD lpMaxCollectionCount,
    LPDWORD lpCollectDataTimeout
);

WINBASEAPI BOOL WINAPI KERNEL32$PeekNamedPipe(
    HANDLE hNamedPipe,
    LPVOID lpBuffer,
    DWORD nBufferSize,
    LPDWORD lpBytesRead,
    LPDWORD lpTotalBytesAvail,
    LPDWORD lpBytesLeftThisMessage
);

WINBASEAPI BOOL WINAPI KERNEL32$ReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
);

WINBASEAPI BOOL WINAPI KERNEL32$WriteFile(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped
);

WINBASEAPI BOOL WINAPI KERNEL32$FlushFileBuffers(
    HANDLE hFile
);

WINBASEAPI BOOL WINAPI KERNEL32$DisconnectNamedPipe(
    HANDLE hNamedPipe
);

WINBASEAPI BOOL WINAPI KERNEL32$CloseHandle(
    HANDLE hObject
);

WINBASEAPI DWORD WINAPI KERNEL32$GetLastError(VOID);

WINBASEAPI BOOL WINAPI KERNEL32$CreatePipe(
    PHANDLE hReadPipe,
    PHANDLE hWritePipe,
    LPSECURITY_ATTRIBUTES lpPipeAttributes,
    DWORD nSize
);

WINBASEAPI VOID WINAPI KERNEL32$Sleep(DWORD dwMilliseconds);

WINBASEAPI BOOL WINAPI KERNEL32$WaitNamedPipeA(
    LPCSTR lpNamedPipeName,
    DWORD nTimeOut
);

WINBASEAPI BOOL WINAPI KERNEL32$WriteFileEx(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPOVERLAPPED lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

WINBASEAPI BOOL WINAPI KERNEL32$GetFileType(
    HANDLE hFile
);

/* MSVCRT Functions */
WINBASEAPI size_t WINAPI MSVCRT$strlen(
    const char* str
);

WINBASEAPI void* WINAPI MSVCRT$memcpy(
    void* dest,
    const void* src,
    size_t n
);

/* ========================================================================== */
/* IPC CONFIGURATION                                                          */
/* ========================================================================== */

#define IPC_BUFFER_SIZE 1024
#define MAX_MESSAGE_SIZE 1024

/* Control signals */
#define CLIENT_DISCONNECT "@END@"
#define MESSAGE_DELIMITER "@START@"

typedef struct {
    HANDLE h_channel;
    char buffer[MAX_MESSAGE_SIZE];
    DWORD buffer_len;
} IpcInstance;

/* ========================================================================== */
/* IPC FUNCTIONS                                                             */
/* ========================================================================== */

/*
 * InitializeIpcServer - Creates and initializes a named pipe server
 * Returns: IpcInstance with h_channel set on success, or empty on failure
 */
HANDLE InitializeIpcServer(const char* channel_name);

/*
 * InitializeIpcClient - Connects to an existing named pipe server
 * Returns: IpcInstance with h_channel set on success, or empty on failure
 */
HANDLE InitializeIpcClient(const char* channel_name);

/*
 * CheckIpcMessages - Checks if there are pending messages in the pipe
 */
BOOL CheckIpcMessages(IpcInstance* ipc);

/*
 * RetrieveIpcMessage - Reads a message from the IPC channel
 */
BOOL RetrieveIpcMessage(IpcInstance* ipc, char* buffer, DWORD buffer_size);

/*
 * SendIpcMessage - Sends a message through the IPC channel
 */
BOOL SendIpcMessage(IpcInstance* ipc, const char* message);

/*
 * AddIpcMessage - Adds a message to the buffer. If buffer is full, sends current buffer (without END) and adds new message
 */
BOOL AddIpcMessage(IpcInstance* ipc, const char* message);

/*
 * CloseIpcClient - Gracefully closes a client connection and sends END marker
 */
BOOL CloseIpcClient(IpcInstance* ipc);

/*
 * CloseIpcServer - Closes the server and cleans up resources
 */
BOOL CloseIpcServer(IpcInstance* ipc);

#endif /* IPC_H */