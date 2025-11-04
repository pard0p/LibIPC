#ifndef IPC_H
#define IPC_H

#include <windows.h>

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
 * InitializeIpcServer - Creates and initializes a named pipe server (always local)
 * channel_name: pipe name (e.g., "mypipe")
 * Returns: handle on success, or NULL on failure
 */
HANDLE InitializeIpcServer(const char* channel_name);

/*
 * InitializeIpcClient - Connects to an existing named pipe server
 * channel_name: pipe name (e.g., "mypipe")
 * host: hostname for remote connection, or NULL for local machine
 *       Local format: \\.\pipe\name | Remote format: \\host\pipe\name
 * Returns: handle on success, or NULL on failure
 */
HANDLE InitializeIpcClient(const char* channel_name, const char* host);

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