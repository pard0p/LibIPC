# LibIPC

LibIPC is a simple Crystal Palace shared library for inter-process communication (IPC), based on Named Pipes. It is designed for asynchronous client-to-server communication, making it ideal for integration into Command & Control (C2) implants and enabling remote execution capabilities such as fork&run.

## Key Features

- **Asynchronous IPC**: Client → server communication using Named Pipes.
- **Local and Remote Pipes**: Support for both local (`\\.\pipe\`) and remote (`\\host\pipe\`) named pipes.
- **Easy integration**: Focused on facilitating Crystal Palace integration in custom C2
- **Remote capabilities**: Enables execution of modules or DLLs and receiving their output in the C2 implant.

## API Reference (`IPC.h`)

LibIPC exposes the following main functions:

- **InitializeIpcServer**  
	Creates and initializes a named pipe server. Returns a valid handle on success.

- **InitializeIpcClient**  
	Connects to an existing named pipe server (local or remote). 
	- Local connection: `InitializeIpcClient("name", NULL)` → `\\.\pipe\name`
	- Remote connection: `InitializeIpcClient("name", "hostname")` → `\\hostname\pipe\name`
	
	Returns a valid handle on success.

- **CheckIpcMessages**  
	Checks if there are pending messages in the pipe.

- **RetrieveIpcMessage**  
	Reads a message from the IPC channel into the provided buffer.

- **SendIpcMessage**  
	Sends a message through the IPC channel.

- **AddIpcMessage**  
	Adds a message to the buffer. If the buffer is full, sends the current buffer (without END marker) and adds the new message.

- **CloseIpcClient**  
	Gracefully closes a client connection and sends an END marker.

- **CloseIpcServer**  
	Closes the server and cleans up resources.

## Use Case / Demo

LibIPC's potential is greatly enhanced when combined with hooking techniques, such as those described in the Crystal Palace loader (`simple_rdll_hook`). See more at [Tradecraft Garden - simplehook](https://tradecraftgarden.org/simplehook.html).

### Practical Example

This Proof of Concept (PoC) demonstrates how LibIPC, together with Crystal Palace's hooking capabilities, can be used to capture and exfiltrate sensitive data from a target process (KeePass.exe) using a standard keylogger DLL. The workflow shows how remote capabilities can be executed and their output reliably transmitted to a C2 server, without modifying the original DLL.

1. **Keylogger DLL**: Example of the DLL compiled to capture keystrokes from KeePass.exe.
	![Keylogger DLL](images/keylogger_dll.png)

2. **Hook setup**: Demonstration of the hook used to intercept and redirect output.
	![Hook Example](images/fputc_hook.png)

3. **PIC Loader Injection**: Injecting the loader PIC into KeePass.exe.
	![PIC Injection](images/keepass_injection.png)

4. **Writing the master key**: Example of KeePass.exe writing the master key.
	![KeePass Master Key](images/keepass.png)

5. **C2 implant**: The fake implant listening and receiving the result from the DLL.
	![Server Receiving Output](images/server.png)

6. **Formatted Output**: The final output, well formatted and received by the C2 implant.
	![Formatted Output](images/output.png)

This workflow demonstrates how a conventional DLL can be used in a C2 context thanks to LibIPC and hooking techniques, without modifying the original DLL.

## Status and Limitations

 - **Simplicity**: LibIPC is a very simple library, focused on easy integration rather than covering all IPC scenarios on Windows.
 - **Purpose**: LibIPC is designed to facilitate the integration of Crystal Palace for Red Team devs into their custom C2, allowing them to focus on developing new remote capabilities rather than IPC implementation.
 - **Not designed for evasion**: LibIPC is not an evasion-oriented library. Any evasion or stealth mechanisms must be implemented logically on the PIC loader side or elsewhere. LibIPC is solely intended for communication purposes.