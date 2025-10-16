#ifdef _TRACE
#define LOGTRACE log.Trace
#else
#define LOGTRACE(...)
#endif

#include <windows.h>
#include <sddl.h>
#include "IPCChannel.hpp"

namespace AnyFSE::Manager
{
    IPCChannel::IPCChannel(const std::wstring &name, bool isServer, HANDLE cancelEvent)
        : m_pipeName(L"\\\\.\\pipe\\Global\\" + name)
        , m_isServer(isServer)
        , m_pipeHandle(INVALID_HANDLE_VALUE)
        , m_cancelEvent(cancelEvent)
        , m_connectionState(ConnectionState::Disconnected)
        , m_readPending(false)
        , m_writePending(false)
        , log(LogManager::GetLogger(isServer ? "ServerIPC" : "ClientIPC"))
    {
        LOGTRACE("Creating object");

        ZeroMemory(&m_connectOverlap, sizeof(m_connectOverlap));
        m_connectOverlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!m_connectOverlap.hEvent)
        {
            log.Error(log.APIError(), "Failed to create connect event");
        }

        ZeroMemory(&m_readOverlap, sizeof(m_readOverlap));
        m_readOverlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!m_readOverlap.hEvent)
        {
            log.Error(log.APIError(), "Failed to create read event");
        }

        ZeroMemory(&m_writeOverlap, sizeof(m_writeOverlap));
        m_writeOverlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!m_writeOverlap.hEvent)
        {
            log.Error(log.APIError(), "Failed to create write event");
        }
    }

    IPCChannel::~IPCChannel()
    {
        LOGTRACE("Destroying channel for pipe '%ls'", m_pipeName.c_str());

        // Cancel any pending I/O since we're shutting down
        if (m_pipeHandle != INVALID_HANDLE_VALUE)
        {
            CancelIo(m_pipeHandle);
        }

        if (m_connectOverlap.hEvent)
        {
            CloseHandle(m_connectOverlap.hEvent);
            m_connectOverlap.hEvent = NULL;
        }
        if (m_readOverlap.hEvent)
        {
            CloseHandle(m_readOverlap.hEvent);
            m_readOverlap.hEvent = NULL;
        }
        if (m_writeOverlap.hEvent)
        {
            CloseHandle(m_writeOverlap.hEvent);
            m_writeOverlap.hEvent = NULL;
        }
        if (m_pipeHandle != INVALID_HANDLE_VALUE)
        {
            if (m_isServer && m_connectionState == ConnectionState::Connected)
            {
                DisconnectNamedPipe(m_pipeHandle);
            }
            CloseHandle(m_pipeHandle);
            m_pipeHandle = NULL;
        }

        LOGTRACE("Channel destroyed");
    }


    // Connection state queries
    IPCChannel::ConnectionState IPCChannel::GetConnectionState() const
    {
        return m_connectionState;
    }

    bool IPCChannel::IsConnected() const
    {
        return m_connectionState == ConnectionState::Connected;
    }

    bool IPCChannel::IsConnecting() const
    {
        return m_connectionState == ConnectionState::Connecting;
    }

    bool IPCChannel::IsDisconnected() const
    {
        return m_connectionState == ConnectionState::Disconnected;
    }

    // Async I/O operations
    bool IPCChannel::Wait(DWORD timeout)
    {
        LOGTRACE("IPCChannel::Wait: Waiting for data, timeout=%lu", timeout);

        // Ensure pipe is ready and connected first
        if (!EnsurePipeReady(timeout))
        {
            LOGTRACE("IPCChannel::Wait: Pipe not ready");
            return false;
        }

        // If we have a pending read, check if it's completed
        if (m_readPending)
        {
            LOGTRACE("IPCChannel::Wait: Checking pending read completion");
            return CheckPendingReadCompletion(timeout);
        }

        // No pending read - check if data is available immediately
        if (CheckDataAvailableImmediately())
        {
            LOGTRACE("IPCChannel::Wait: Data available immediately");
            return true;
        }

        // Start an async read and wait for it
        LOGTRACE("IPCChannel::Wait: Starting async read and wait");
        return StartAsyncReadAndWait(timeout);
    }

    bool IPCChannel::Read(Message *message)
    {
        if (!message)
        {
            LOGTRACE("IPCChannel::Read: Null message pointer");
            return false;
        }

        LOGTRACE("IPCChannel::Read: Attempting to read message");

        // Ensure pipe is ready and connected (non-blocking check)
        if (!EnsurePipeReady(0))
        {
            LOGTRACE("IPCChannel::Read: Pipe not ready for reading");
            return false;
        }

        // If we have a pending read, check if it completed
        if (m_readPending && CheckPendingReadCompletion(0))
        {
            LOGTRACE("IPCChannel::Read: Completing pending read");
            return CompletePendingRead(message);
        }

        // Check if data is available immediately for synchronous read
        if (CheckDataAvailableImmediately())
        {
            LOGTRACE("IPCChannel::Read: Reading available data immediately");
            return ReadAvailableData(message);
        }

        // No data available immediately and no completed pending read
        LOGTRACE("IPCChannel::Read: No data available");
        return false;
    }

    bool IPCChannel::StartAsyncRead()
    {
        LOGTRACE("IPCChannel::StartAsyncRead: Starting async read operation");

        if (!EnsurePipeReady(0))
        {
            LOGTRACE("IPCChannel::StartAsyncRead: Pipe not ready");
            return false;
        }

        // Don't start a new read if one is already pending
        if (m_readPending)
        {
            LOGTRACE("IPCChannel::StartAsyncRead: Read already pending");
            return true;
        }

        ResetEvent(m_readOverlap.hEvent);

        DWORD bytesRead = 0;
        BOOL readResult = ReadFile(
            m_pipeHandle,
            &m_pendingMessage,
            sizeof(Message),
            &bytesRead,
            &m_readOverlap);

        if (readResult)
        {
            // Read completed immediately
            LOGTRACE("IPCChannel::StartAsyncRead: Read completed immediately, bytes=%lu", bytesRead);
            m_readPending = true;
            return true;
        }

        DWORD error = GetLastError();
        if (error == ERROR_IO_PENDING)
        {
            // Read is in progress
            LOGTRACE("IPCChannel::StartAsyncRead: Read pending (async)");
            m_readPending = true;
            return true;
        }
        else
        {
            // Read failed immediately
            log.Error(log.APIError(), "IPCChannel::StartAsyncRead: Read failed immediately");
            if (error == ERROR_BROKEN_PIPE || error == ERROR_PIPE_NOT_CONNECTED)
            {
                m_connectionState = ConnectionState::Disconnected;
                LOGTRACE("IPCChannel::StartAsyncRead: Pipe disconnected due to error");
            }
            return false;
        }
    }

    bool IPCChannel::IsReadPending() const
    {
        return m_readPending;
    }

    void IPCChannel::CancelRead()
    {
        if (m_readPending && m_pipeHandle != INVALID_HANDLE_VALUE)
        {
            LOGTRACE("IPCChannel::CancelRead: Canceling pending read");
            CancelIo(m_pipeHandle);
            m_readPending = false;
        }
    }

    bool IPCChannel::Write(const Message *message, DWORD timeout)
    {
        if (!message)
        {
            LOGTRACE("IPCChannel::Write: Null message pointer");
            return false;
        }

        LOGTRACE("IPCChannel::Write: Attempting to write message, timeout=%lu", timeout);

        // Check if write is already in progress (non-reentrant)
        if (m_writePending)
        {
            LOGTRACE("IPCChannel::Write: Write already in progress, dropping message");
            return false;
        }

        // Ensure pipe is ready and connected
        if (!EnsurePipeReady(timeout))
        {
            LOGTRACE("IPCChannel::Write: Pipe not ready within timeout");
            return false;
        }

        return AsyncWriteWithTimeout(message, timeout);
    }

    void IPCChannel::SetCancelEvent(HANDLE cancelEvent)
    {
        LOGTRACE("IPCChannel::SetCancelEvent: Setting cancel event");
        m_cancelEvent = cancelEvent;
    }

    void IPCChannel::ResetConnection()
    {
        LOGTRACE("IPCChannel::ResetConnection: Resetting connection state");

        if (m_isServer && m_pipeHandle != INVALID_HANDLE_VALUE)
        {
            if (m_connectionState == ConnectionState::Connected)
            {
                DisconnectNamedPipe(m_pipeHandle);
                LOGTRACE("IPCChannel::ResetConnection: Disconnected named pipe");
            }
            m_connectionState = ConnectionState::Disconnected;
        }
        m_readPending = false;
        m_writePending = false;

        LOGTRACE("IPCChannel::ResetConnection: Connection reset complete");
    }

    // Private implementation methods
    bool IPCChannel::EnsurePipeReady(DWORD timeout)
    {
        // For non-blocking check, use timeout = 0
        if (timeout == 0)
        {
            // Non-blocking: quick check if we're ready
            if (m_pipeHandle == INVALID_HANDLE_VALUE)
            {
                return false;
            }
            if (m_isServer && m_connectionState != ConnectionState::Connected)
            {
                return false;
            }
            return true;
        }

        // Blocking: ensure pipe exists and is connected
        if (!EnsureNamedPipe(timeout))
        {
            LOGTRACE("IPCChannel::EnsurePipeReady: Failed to ensure named pipe");
            return false;
        }

        // For server, ensure we have a connection
        if (m_isServer && m_connectionState != ConnectionState::Connected)
        {
            if (!ServerWaitForConnection(timeout))
            {
                LOGTRACE("IPCChannel::EnsurePipeReady: Server failed to wait for connection");
                return false;
            }
        }

        bool ready = (m_pipeHandle != INVALID_HANDLE_VALUE &&
                      (!m_isServer || m_connectionState == ConnectionState::Connected));

        LOGTRACE("IPCChannel::EnsurePipeReady: Pipe ready=%d", ready);
        return ready;
    }

    bool IPCChannel::CheckDataAvailableImmediately()
    {
        DWORD bytesAvailable = 0;
        if (PeekNamedPipe(m_pipeHandle, NULL, 0, NULL, &bytesAvailable, NULL))
        {
            bool available = (bytesAvailable >= sizeof(Message));
            if (available)
            {
                LOGTRACE("IPCChannel::CheckDataAvailableImmediately: Data available, bytes=%lu", bytesAvailable);
            }
            return available;
        }
        else
        {
            DWORD error = GetLastError();
            LOGTRACE("IPCChannel::CheckDataAvailableImmediately: PeekNamedPipe failed, error=%lu", error);
            return false;
        }
    }

    bool IPCChannel::CheckPendingReadCompletion(DWORD timeout)
    {
        LOGTRACE("IPCChannel::CheckPendingReadCompletion: Checking with timeout=%lu", timeout);

        HANDLE waitHandles[2] = {m_readOverlap.hEvent, m_cancelEvent};
        DWORD handleCount = (m_cancelEvent != NULL) ? 2 : 1;

        LOGTRACE("IPCChannel::AsyncWriteWithTimeout: Write pending, waiting for completion");
        DWORD result = WaitForMultipleObjects(handleCount, waitHandles, FALSE, timeout);

        bool completed = (result == WAIT_OBJECT_0);

        LOGTRACE("IPCChannel::CheckPendingReadCompletion: Wait result=%lu, completed=%d", result, completed);
        return completed;
    }

    bool IPCChannel::StartAsyncReadAndWait(DWORD timeout)
    {
        LOGTRACE("IPCChannel::StartAsyncReadAndWait: Starting async read with wait, timeout=%lu", timeout);

        ResetEvent(m_readOverlap.hEvent);

        DWORD bytesRead = 0;
        BOOL readResult = ReadFile(
            m_pipeHandle,
            &m_pendingMessage,
            sizeof(Message),
            &bytesRead,
            &m_readOverlap);

        if (readResult)
        {
            // Read completed immediately
            LOGTRACE("IPCChannel::StartAsyncReadAndWait: Read completed immediately, bytes=%lu", bytesRead);
            m_readPending = true;
            return true;
        }

        DWORD error = GetLastError();
        if (error == ERROR_IO_PENDING)
        {
            // Read is in progress
            LOGTRACE("IPCChannel::StartAsyncReadAndWait: Read pending, waiting for completion");
            m_readPending = true;
            return CheckPendingReadCompletion(timeout);
        }
        else
        {
            // Read failed immediately
            log.Error(log.APIError(), "IPCChannel::StartAsyncReadAndWait: Read failed immediately");
            if (error == ERROR_BROKEN_PIPE || error == ERROR_PIPE_NOT_CONNECTED)
            {
                m_connectionState = ConnectionState::Disconnected;
                LOGTRACE("IPCChannel::StartAsyncReadAndWait: Pipe disconnected due to error");
            }
            return false;
        }
    }

    bool IPCChannel::CompletePendingRead(Message *message)
    {
        LOGTRACE("IPCChannel::CompletePendingRead: Completing pending read operation");

        DWORD bytesTransferred;
        if (GetOverlappedResult(m_pipeHandle, &m_readOverlap, &bytesTransferred, FALSE))
        {
            if (bytesTransferred == sizeof(Message))
            {
                *message = m_pendingMessage;
                m_readPending = false;
                LOGTRACE("IPCChannel::CompletePendingRead: Read completed successfully");
                return true;
            }
            else
            {
                LOGTRACE("IPCChannel::CompletePendingRead: Incomplete read, bytes=%lu, expected=%zu",
                          bytesTransferred, sizeof(Message));
            }
        }
        else
        {
            log.Error(log.APIError(), "IPCChannel::CompletePendingRead: GetOverlappedResult failed");
        }

        // Read failed
        m_readPending = false;
        DWORD error = GetLastError();
        if (error == ERROR_BROKEN_PIPE || error == ERROR_PIPE_NOT_CONNECTED)
        {
            m_connectionState = ConnectionState::Disconnected;
            LOGTRACE("IPCChannel::CompletePendingRead: Pipe disconnected due to error");
        }
        return false;
    }

    bool IPCChannel::ReadAvailableData(Message *message)
    {
        LOGTRACE("IPCChannel::ReadAvailableData: Reading available data synchronously");

        DWORD bytesRead = 0;
        BOOL readResult = ReadFile(
            m_pipeHandle,
            message,
            sizeof(Message),
            &bytesRead,
            NULL);

        if (readResult && bytesRead == sizeof(Message))
        {
            LOGTRACE("IPCChannel::ReadAvailableData: Read completed, bytes=%lu", bytesRead);
            return true;
        }

        // Read failed
        DWORD error = GetLastError();
        log.Error(log.APIError(), "IPCChannel::ReadAvailableData: Synchronous read failed");
        if (error == ERROR_BROKEN_PIPE || error == ERROR_PIPE_NOT_CONNECTED)
        {
            m_connectionState = ConnectionState::Disconnected;
            LOGTRACE("IPCChannel::ReadAvailableData: Pipe disconnected due to error");
        }
        return false;
    }

    bool IPCChannel::AsyncWriteWithTimeout(const Message *message, DWORD timeout)
    {
        LOGTRACE("IPCChannel::AsyncWriteWithTimeout: Starting async write, timeout=%lu", timeout);

        // Mark write as pending (non-reentrant)
        m_writePending = true;

        // Reset the write event
        ResetEvent(m_writeOverlap.hEvent);

        DWORD bytesWritten = 0;
        BOOL writeResult = WriteFile(
            m_pipeHandle,
            message,
            sizeof(Message),
            &bytesWritten,
            &m_writeOverlap);

        if (writeResult)
        {
            // Write completed immediately
            m_writePending = false;
            if (bytesWritten == sizeof(Message))
            {
                LOGTRACE("IPCChannel::AsyncWriteWithTimeout: Write completed immediately, bytes=%lu", bytesWritten);
                // FlushFileBuffers(m_pipeHandle);
                return true;
            }
            else
            {
                LOGTRACE("IPCChannel::AsyncWriteWithTimeout: Incomplete write, bytes=%lu, expected=%zu",
                          bytesWritten, sizeof(Message));
                return false;
            }
        }

        DWORD error = GetLastError();
        if (error == ERROR_IO_PENDING)
        {
            // Write is in progress - wait for completion
            HANDLE waitHandles[2] = {m_writeOverlap.hEvent, m_cancelEvent};
            DWORD handleCount = (m_cancelEvent != NULL) ? 2 : 1;

            LOGTRACE("IPCChannel::AsyncWriteWithTimeout: Write pending, waiting for completion");
            DWORD result = WaitForMultipleObjects(handleCount, waitHandles, FALSE, timeout);

            switch (result)
            {
            case WAIT_OBJECT_0:
                // Write completed
                {
                    DWORD bytesTransferred;
                    if (GetOverlappedResult(m_pipeHandle, &m_writeOverlap, &bytesTransferred, FALSE))
                    {
                        m_writePending = false;
                        if (bytesTransferred == sizeof(Message))
                        {
                            LOGTRACE("IPCChannel::AsyncWriteWithTimeout: Write completed successfully, bytes=%lu",
                                      bytesTransferred);
                            // FlushFileBuffers(m_pipeHandle);
                            return true;
                        }
                        else
                        {
                            LOGTRACE("IPCChannel::AsyncWriteWithTimeout: Incomplete write after wait, bytes=%lu, expected=%zu",
                                      bytesTransferred, sizeof(Message));
                        }
                    }
                    else
                    {
                        log.Error(log.APIError(), "IPCChannel::AsyncWriteWithTimeout: GetOverlappedResult failed after write completion");
                    }
                }
                break;

            case WAIT_OBJECT_0 + 1:
                // Cancel event signaled
                LOGTRACE("IPCChannel::AsyncWriteWithTimeout: Write canceled by cancel event");
                CancelIo(m_pipeHandle);
                m_writePending = false;
                return false;

            case WAIT_TIMEOUT:
                // Timeout occurred - drop the message
                LOGTRACE("IPCChannel::AsyncWriteWithTimeout: Write timeout, dropping message");
                CancelIo(m_pipeHandle);
                m_writePending = false;
                return false;

            default:
                // Wait failed
                log.Error(log.APIError(), "IPCChannel::AsyncWriteWithTimeout: WaitForMultipleObjects failed");
                CancelIo(m_pipeHandle);
                m_writePending = false;
                return false;
            }
        }
        else
        {
            // Write failed immediately
            log.Error(log.APIError(), "IPCChannel::AsyncWriteWithTimeout: Write failed immediately");
            if (error == ERROR_BROKEN_PIPE || error == ERROR_PIPE_NOT_CONNECTED)
            {
                m_connectionState = ConnectionState::Disconnected;
                LOGTRACE("IPCChannel::AsyncWriteWithTimeout: Pipe disconnected due to error");
            }
            m_writePending = false;
            return false;
        }

        m_writePending = false;
        return false;
    }

    bool IPCChannel::EnsureNamedPipe(DWORD timeout)
    {
        if (m_pipeHandle != INVALID_HANDLE_VALUE)
        {
            return true;
        }

        LOGTRACE("IPCChannel::EnsureNamedPipe: Creating named pipe, timeout=%lu", timeout);

        ULONGLONG startTime = GetTickCount64();

        while (m_pipeHandle == INVALID_HANDLE_VALUE)
        {
            if (m_isServer)
            {

                SECURITY_ATTRIBUTES sa;
                PSECURITY_DESCRIPTOR pSD = NULL;

                // Create security descriptor allowing cross-session access
                const char* szSDDL = "D:(A;OICI;GA;;;SY)(A;OICI;GA;;;AU)(A;OICI;GA;;;IU)";

                if (!ConvertStringSecurityDescriptorToSecurityDescriptorA(
                        szSDDL, SDDL_REVISION_1, &pSD, NULL))
                {
                    log.Error(log.APIError(), "ConvertStringSecurityDescriptor failed:");
                    return false;
                }

                sa.nLength = sizeof(SECURITY_ATTRIBUTES);
                sa.lpSecurityDescriptor = pSD;
                sa.bInheritHandle = FALSE;

                m_pipeHandle = CreateNamedPipe(
                    m_pipeName.c_str(),
                    PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                    PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                    PIPE_UNLIMITED_INSTANCES,
                    BUFFER_SIZE,
                    BUFFER_SIZE,
                    CONNECT_TIMEOUT,
                    &sa);

                if (m_pipeHandle != INVALID_HANDLE_VALUE)
                {
                    m_connectionState = ConnectionState::Disconnected;
                    LOGTRACE("IPCChannel::EnsureNamedPipe: Server pipe created successfully");
                }
                else
                {
                    log.Error(log.APIError(), "IPCChannel::EnsureNamedPipe: Failed to create server pipe");
                }
            }
            else
            {
                m_pipeHandle = CreateFile(
                    m_pipeName.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_OVERLAPPED,
                    NULL);

                if (m_pipeHandle != INVALID_HANDLE_VALUE)
                {
                    DWORD mode = PIPE_READMODE_MESSAGE;
                    if (SetNamedPipeHandleState(m_pipeHandle, &mode, NULL, NULL))
                    {
                        m_connectionState = ConnectionState::Connected;
                        LOGTRACE("IPCChannel::EnsureNamedPipe: Client pipe connected successfully");
                    }
                    else
                    {
                        log.Error(log.APIError(), "IPCChannel::EnsureNamedPipe: Failed to set pipe mode");
                        CloseHandle(m_pipeHandle);
                        m_pipeHandle = INVALID_HANDLE_VALUE;
                    }
                }
                else
                {
                    DWORD error = GetLastError();
                    if (error != ERROR_PIPE_BUSY)
                    {
                        log.Error(log.APIError(), "IPCChannel::EnsureNamedPipe: Failed to connect to server pipe");
                    }
                }
            }

            // Check if we succeeded
            if (m_pipeHandle != INVALID_HANDLE_VALUE)
            {
                return true;
            }

            // For non-blocking, return immediately
            if (timeout == 0)
            {
                return false;
            }

            // Check for cancellation
            if (m_cancelEvent && WaitForSingleObject(m_cancelEvent, 0) == WAIT_OBJECT_0)
            {
                LOGTRACE("IPCChannel::EnsureNamedPipe: Operation canceled");
                return false;
            }

            // Check timeout
            if ((GetTickCount64() - startTime) >= timeout)
            {
                LOGTRACE("IPCChannel::EnsureNamedPipe: Timeout creating pipe");
                return false;
            }

            // Wait before retry
            Sleep(100);
        }

        return true;
    }

    bool IPCChannel::ServerWaitForConnection(DWORD timeout)
    {
        if (!m_isServer || m_pipeHandle == INVALID_HANDLE_VALUE)
        {
            LOGTRACE("IPCChannel::ServerWaitForConnection: Invalid state for connection");
            return false;
        }

        LOGTRACE("IPCChannel::ServerWaitForConnection: Waiting for client connection, timeout=%lu", timeout);

        // If we're already connected, return true
        if (m_connectionState == ConnectionState::Connected)
        {
            LOGTRACE("IPCChannel::ServerWaitForConnection: Already connected");
            return true;
        }

        // If no connection is in progress, start one
        if (m_connectionState == ConnectionState::Disconnected)
        {
            // Reset the connection event
            ResetEvent(m_connectOverlap.hEvent);

            // Start async connection
            BOOL connected = ConnectNamedPipe(m_pipeHandle, &m_connectOverlap);
            DWORD error = GetLastError();

            if (connected)
            {
                // Connected immediately (shouldn't happen with FILE_FLAG_OVERLAPPED)
                LOGTRACE("IPCChannel::ServerWaitForConnection: Connected immediately");
                m_connectionState = ConnectionState::Connected;
                return true;
            }

            switch (error)
            {
            case ERROR_IO_PENDING:
                // Connection is in progress
                LOGTRACE("IPCChannel::ServerWaitForConnection: Connection pending (async)");
                m_connectionState = ConnectionState::Connecting;
                break;

            case ERROR_PIPE_CONNECTED:
                // Client already connected
                LOGTRACE("IPCChannel::ServerWaitForConnection: Client already connected");
                SetEvent(m_connectOverlap.hEvent);
                m_connectionState = ConnectionState::Connected;
                return true;

            default:
                // Other error - connection failed
                log.Error(log.APIError(), "IPCChannel::ServerWaitForConnection: ConnectNamedPipe failed");
                m_pipeHandle = INVALID_HANDLE_VALUE;
                m_connectionState = ConnectionState::Disconnected;
                return false;
            }
        }

        // We're in Connecting state - wait for the pending connection to complete
        HANDLE waitHandles[2] = {m_connectOverlap.hEvent, m_cancelEvent};
        DWORD handleCount = (m_cancelEvent != NULL) ? 2 : 1;

        LOGTRACE("IPCChannel::ServerWaitForConnection: Waiting for connection completion");
        DWORD result = WaitForMultipleObjects(handleCount, waitHandles, FALSE, min(timeout, CONNECT_TIMEOUT));

        switch (result)
        {
        case WAIT_OBJECT_0:
            // Connection completed
            {
                DWORD bytesTransferred;
                if (GetOverlappedResult(m_pipeHandle, &m_connectOverlap, &bytesTransferred, FALSE))
                {
                    m_connectionState = ConnectionState::Connected;
                    LOGTRACE("IPCChannel::ServerWaitForConnection: Connection established successfully");
                    return true;
                }
                else
                {
                    // Connection failed
                    log.Error(log.APIError(), "IPCChannel::ServerWaitForConnection: GetOverlappedResult failed");
                    m_connectionState = ConnectionState::Disconnected;
                    return false;
                }
            }
            break;

        case WAIT_OBJECT_0 + 1:
            // Cancel event signaled - don't cancel I/O, just return
            LOGTRACE("IPCChannel::ServerWaitForConnection: Connection wait canceled");
            return false;

        case WAIT_TIMEOUT:
            // Timeout occurred - don't cancel I/O, just return
            LOGTRACE("IPCChannel::ServerWaitForConnection: Connection wait timeout");
            return false;

        default:
            // Wait failed
            log.Error(log.APIError(), "IPCChannel::ServerWaitForConnection: WaitForMultipleObjects failed");
            m_connectionState = ConnectionState::Disconnected;
            return false;
        }
    }
}