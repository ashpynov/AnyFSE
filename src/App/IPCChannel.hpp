// MIT License
//
// Copyright (c) 2025 Artem Shpynov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//


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

        static bool IsServerAvailable(const std::wstring &name);

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