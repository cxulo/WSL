#include "stdafx.h"

namespace Oobe
{

    void Win32ChildProcess::onClose(void* data, BOOLEAN /*unused*/)
    {
        auto* instance = static_cast<Win32ChildProcess*>(data);
        instance->unsubscribe();
        // Process ended anyway.
        instance->destroy();
        instance->notifyListener();
    }

    void Win32ChildProcess::do_unsubscribe()
    {
        if (waiterHandle != nullptr) {
            UnregisterWait(waiterHandle);
        }
    }

    void Win32ChildProcess::destroy()
    {
        CloseHandle(procInfo.hThread);
        CloseHandle(procInfo.hProcess);
        procInfo.hProcess = nullptr;
    }

    void Win32ChildProcess::do_terminate()
    {
        if (procInfo.hProcess != nullptr && procInfo.hProcess != INVALID_HANDLE_VALUE) {
            TerminateProcess(procInfo.hProcess, 0);
            destroy();
        }
    }

    bool Win32ChildProcess::do_start()
    {
        if (!std::filesystem::exists(executable)) {
            std::wcerr << L"Executable <" << executable << L"> doesn't exist.\n";
            return false;
        }

        auto cli = executable.wstring();
        if (!arguments.empty()) {
            cli.append(1, L' ');
            cli.append(arguments);
        }

        BOOL res = CreateProcess(nullptr,                   // command line
                                 cli.data(),                // non-const CLI
                                 &sa,                       // process security attributes
                                 nullptr,                   // primary thread security attributes
                                 TRUE,                      // handles are inherited
                                 0,                         // creation flags
                                 nullptr,                   // use parent's environment
                                 nullptr,                   // use parent's current directory
                                 &startInfo,                // STARTUPINFO pointer
                                 &procInfo);                // output: PROCESS_INFORMATION
        auto ok = res != 0 && procInfo.hProcess != nullptr; // success
        if (ok) {
            RegisterWaitForSingleObject(&waiterHandle, procInfo.hProcess, onClose, this, INFINITE,
                                        WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE);
        }

        return ok;
    }

    DWORD Win32ChildProcess::do_waitExitSync(DWORD timeout)
    {
        if (auto waitRes = WaitForSingleObject(procInfo.hProcess, timeout); waitRes != WAIT_OBJECT_0) {
            return waitRes;
        }
        DWORD exitCode = 0;
        GetExitCodeProcess(procInfo.hProcess, &exitCode);
        return exitCode;
    }
}
