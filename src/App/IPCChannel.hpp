
#pragma once

#include <windows.h>
#include <string>
#include "App/AppEvents.hpp"
#include "Logging/LogManager.hpp"

namespace AnyFSE::App
{
#pragma pack(push, 1)
    struct Message
    {
        AppEvents event;
        LONGLONG ticks;
    };
#pragma pack(pop)

    class IPCChannel
    {
    public:
        enum class ConnectionState
        {
            Disconnected,
            Connecting,
            Connected
        };

        IPCChannel(const std::wstring &name, bool isServer, HANDLE cancelEvent = NULL);
        ~IPCChannel();

        // Connection state queries
        ConnectionState GetConnectionState() const;
        bool IsConnected() const;
        bool IsConnecting() const;
        bool IsDisconnected() const;

        // Async I/O operations
        bool Wait(DWORD timeout);
        bool Read(Message *message);
        bool StartAsyncRead();
        bool IsReadPending() const;
        void CancelRead();

        // Write operation (non-reentrant, drops message on timeout)
        bool Write(const Message *message, DWORD timeout = 3000);

        // Event management
        void SetCancelEvent(HANDLE cancelEvent);

        // Connection management
        void ResetConnection();

    private:
        // Internal state management
        bool EnsurePipeReady(DWORD timeout);
        bool EnsureNamedPipe(DWORD timeout);
        bool ServerWaitForConnection(DWORD timeout);

        // Read operations
        bool CheckDataAvailableImmediately();
        bool CheckPendingReadCompletion(DWORD timeout);
        bool StartAsyncReadAndWait(DWORD timeout);
        bool CompletePendingRead(Message *message);
        bool ReadAvailableData(Message *message);

        // Write operations
        bool AsyncWriteWithTimeout(const Message *message, DWORD timeout);

        // Member variables
        std::wstring m_pipeName;
        bool m_isServer;
        HANDLE m_pipeHandle;
        HANDLE m_cancelEvent;
        OVERLAPPED m_connectOverlap;
        OVERLAPPED m_readOverlap;
        OVERLAPPED m_writeOverlap;
        ConnectionState m_connectionState;
        bool m_readPending;
        bool m_writePending;
        Message m_pendingMessage;
        Logger log;

        static const DWORD BUFFER_SIZE = sizeof(Message)*100;
        static const DWORD CONNECT_TIMEOUT = 5000;
        static const DWORD WRITE_TIMEOUT = 3000;
    };
}