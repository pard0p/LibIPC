#include "IPC.h"
#include <string.h>
#include <stdio.h>
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

/* Helper functions */
static void BuildPipeName(char* dest, const char* name, const char* host) {
    int i = 0;
    int j = 0;
    
    if (host == NULL) {
        /* Local pipe: \\.\pipe\ */
        const char* localPrefix = "\\\\.\\pipe\\";
        for (i = 0; localPrefix[i] != '\0'; i++) {
            dest[i] = localPrefix[i];
        }
    } else {
        /* Remote pipe: \\<host>\pipe\ */
        dest[i++] = '\\';
        dest[i++] = '\\';
        
        /* Copy hostname */
        for (j = 0; host[j] != '\0'; j++) {
            dest[i + j] = host[j];
        }
        i += j;
        
        dest[i++] = '\\';
        dest[i++] = 'p';
        dest[i++] = 'i';
        dest[i++] = 'p';
        dest[i++] = 'e';
        dest[i++] = '\\';
    }
    
    /* Copy pipe name */
    for (j = 0; name[j] != '\0'; j++) {
        dest[i + j] = name[j];
    }
    dest[i + j] = '\0';
}

HANDLE InitializeIpcServer(const char* channel_name) {
    char pipe_name[256];
    BuildPipeName(pipe_name, channel_name, NULL);
    
    HANDLE h_pipe = KERNEL32$CreateNamedPipeA(
        pipe_name,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT, /* Non-blocking mode */
        PIPE_UNLIMITED_INSTANCES,
        IPC_BUFFER_SIZE,
        IPC_BUFFER_SIZE,
        0,
        NULL
    );
    
    if (h_pipe == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    
    /* Try to accept connection (non-blocking due to PIPE_NOWAIT) */
    BOOL connected = KERNEL32$ConnectNamedPipe(h_pipe, NULL);
    if (!connected) {
        DWORD error = KERNEL32$GetLastError();
        /* ERROR_PIPE_CONNECTED means client already connected */
        /* ERROR_NO_DATA means no client yet - both are OK for non-blocking */
        /* ERROR_IO_PENDING means operation is pending - also OK for non-blocking */
        /* Error 536 also seems to be normal for non-blocking pipes */
        if (error != ERROR_PIPE_CONNECTED && error != ERROR_NO_DATA && error != ERROR_IO_PENDING && error != 536) {
            KERNEL32$CloseHandle(h_pipe);
            return NULL;
        }
    }

    return h_pipe;
}

HANDLE InitializeIpcClient(const char* channel_name, const char* host) {
    char pipe_name[256];
    BuildPipeName(pipe_name, channel_name, host);
    
    /* Try to connect immediately, don't wait */
    HANDLE h_pipe = KERNEL32$CreateFileA(
        pipe_name,
        GENERIC_READ | GENERIC_WRITE, /* Need both for SetNamedPipeHandleState */
        0,
        NULL,
        OPEN_EXISTING,
        0, /* No special flags */
        NULL
    );
    
    if (h_pipe == INVALID_HANDLE_VALUE) {
        /* If pipe doesn't exist or busy, just return empty IpcInstance */
        return NULL;
    }
    
    /* Set byte mode for client */
    DWORD mode = PIPE_READMODE_BYTE;
    KERNEL32$SetNamedPipeHandleState(h_pipe, &mode, NULL, NULL);
    
    return h_pipe;
}

BOOL CheckIpcMessages(IpcInstance* ipc) {
    if (!ipc || !ipc->h_channel || ipc->h_channel == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    DWORD bytes_available = 0;
    if (!KERNEL32$PeekNamedPipe(ipc->h_channel, NULL, 0, NULL, &bytes_available, NULL)) {
        return FALSE;
    }
    
    return bytes_available > 0;
}

BOOL RetrieveIpcMessage(IpcInstance* ipc, char* buffer, DWORD buffer_size) {
    if (!ipc || !ipc->h_channel || ipc->h_channel == INVALID_HANDLE_VALUE || !buffer) {
        return FALSE;
    }
    
    DWORD bytes_read = 0;
    
    /* Non-blocking read - returns immediately if no data */
    BOOL result = KERNEL32$ReadFile(ipc->h_channel, buffer, buffer_size - 1, &bytes_read, NULL);
    
    if (result && bytes_read > 0) {
        buffer[bytes_read] = '\0';
        return TRUE;
    }
    
    /* Check if no data available (normal for non-blocking) */
    DWORD error = KERNEL32$GetLastError();
    if (error == ERROR_NO_DATA || error == ERROR_PIPE_LISTENING) {
        /* No data available right now - not an error */
        return FALSE;
    }
    
    return FALSE;
}

BOOL SendIpcMessage(IpcInstance* ipc, const char* message) {
    if (!ipc || !ipc->h_channel || ipc->h_channel == INVALID_HANDLE_VALUE || !message) {
        return FALSE;
    }
    
    /* Calculate lengths */
    DWORD delimiter_len = MSVCRT$strlen(MESSAGE_DELIMITER);
    DWORD message_len = MSVCRT$strlen(message);
    DWORD total_len = delimiter_len + message_len;
    
    /* Create buffer for delimited message */
    char delimited_message[MAX_MESSAGE_SIZE];
    if (total_len >= MAX_MESSAGE_SIZE) {
        return FALSE; /* Message too long */
    }
    
    /* Copy delimiter first */
    DWORD i;
    for (i = 0; i < delimiter_len; i++) {
        delimited_message[i] = MESSAGE_DELIMITER[i];
    }
    
    /* Copy original message */
    for (i = 0; i < message_len; i++) {
        delimited_message[delimiter_len + i] = message[i];
    }
    
    DWORD bytes_written = 0;
    
    /* Non-blocking write - if pipe is full, this will fail immediately */
    BOOL result = KERNEL32$WriteFile(ipc->h_channel, delimited_message, total_len, &bytes_written, NULL);
    
    if (result) {
        /* Try to flush, but don't care if it fails */
        KERNEL32$FlushFileBuffers(ipc->h_channel);
        return bytes_written == total_len;
    }
    
    /* Check error - if pipe is full, just ignore (message lost) */
    DWORD error = KERNEL32$GetLastError();
    if (error == ERROR_NO_DATA || error == ERROR_PIPE_NOT_CONNECTED) {
        /* Pipe full or not connected - message lost, but not a critical error */
        return FALSE;
    }
    
    return FALSE;
}

BOOL AddIpcMessage(IpcInstance* ipc, const char* message) {
    if (!ipc || !message) return FALSE;
    DWORD delim_len = MSVCRT$strlen(MESSAGE_DELIMITER);
    DWORD msg_len = MSVCRT$strlen(message);
    DWORD total_len = ipc->buffer_len + delim_len + msg_len;
    if (total_len >= MAX_MESSAGE_SIZE) {
        /* Buffer full, send what we have (without END) */
        DWORD bytes_written = 0;
        KERNEL32$WriteFile(ipc->h_channel, ipc->buffer, ipc->buffer_len, &bytes_written, NULL);
        ipc->buffer_len = 0;
        ipc->buffer[0] = '\0';
        /* Now add the new message */
        MSVCRT$memcpy(ipc->buffer, MESSAGE_DELIMITER, delim_len);
        MSVCRT$memcpy(ipc->buffer + delim_len, message, msg_len);
        ipc->buffer_len = delim_len + msg_len;
        ipc->buffer[ipc->buffer_len] = '\0';
        return FALSE;
    } else {
        MSVCRT$memcpy(ipc->buffer + ipc->buffer_len, MESSAGE_DELIMITER, delim_len);
        MSVCRT$memcpy(ipc->buffer + ipc->buffer_len + delim_len, message, msg_len);
        ipc->buffer_len += delim_len + msg_len;
        ipc->buffer[ipc->buffer_len] = '\0';
        return TRUE;
    }
}

BOOL CloseIpcClient(IpcInstance* ipc) {
    if (!ipc || !ipc->h_channel || ipc->h_channel == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    BOOL  result  = TRUE;
    DWORD end_len = MSVCRT$strlen(CLIENT_DISCONNECT);
    
    if (ipc->buffer_len + end_len >= MAX_MESSAGE_SIZE) {
        /* If it doesn't fit, send what we have and then only END */
        DWORD bytes_written = 0;
        KERNEL32$WriteFile(ipc->h_channel, ipc->buffer, ipc->buffer_len, &bytes_written, NULL);
        KERNEL32$WriteFile(ipc->h_channel, CLIENT_DISCONNECT, end_len, &bytes_written, NULL);
        ipc->buffer_len = 0;
        ipc->buffer[0] = '\0';
    } else {
        MSVCRT$memcpy(ipc->buffer + ipc->buffer_len, CLIENT_DISCONNECT, end_len);
        ipc->buffer_len += end_len;
        ipc->buffer[ipc->buffer_len] = '\0';
        DWORD bytes_written = 0;
        result = KERNEL32$WriteFile(ipc->h_channel, ipc->buffer, ipc->buffer_len, &bytes_written, NULL);
        ipc->buffer_len = 0;
        ipc->buffer[0] = '\0';
    }

    KERNEL32$CloseHandle(ipc->h_channel);
    return result;
}

BOOL CloseIpcServer(IpcInstance* ipc) {
    if (!ipc || !ipc->h_channel || ipc->h_channel == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    KERNEL32$DisconnectNamedPipe(ipc->h_channel);
    KERNEL32$CloseHandle(ipc->h_channel);
    return TRUE;
}