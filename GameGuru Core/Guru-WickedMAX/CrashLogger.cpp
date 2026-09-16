#include <windows.h>
#include <dbghelp.h>
#include <tchar.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cstdio>   // _snprintf_s in the SEH-guarded stack walk (no C++ objects allowed there)
#include <cstring>

#pragma comment(lib, "dbghelp.lib")

#define MAX_PATH 1024

// global we can populate with the current running version to match EXE/PDB pairs
char g_pCrashVersionINIValue[256] = "Very Early";

// ★ GGMAX 3.38 (DX11): the Wicked MAX debug log that GetCrashHandlerDebugLogRef() returns a
// pointer into, so a crash report can carry the GFX log with it. ⚠ static: the accessor hands out
// the address, nothing outside this file may define its own.
static char g_pDebugExtraInfo[10240] = { 0 };

// Recorded when the handler is installed (from the main thread), so a crash report can say
// whether the faulting thread was the frame loop or a background worker.
DWORD g_dwCrashMainThreadId = 0;

// What time is it
std::string GetTimestamp()
{
    time_t now = time(nullptr);
    struct tm localTime;
    localtime_s(&localTime, &now);

    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
    return std::string(buffer);
}


// Walk and symbolize the faulting thread's stack into a plain char buffer.
//
// Deliberately its own function with NO C++ objects that need unwinding, because that is the
// only way MSVC will allow __try/__except here. The guard matters: StackWalk64 follows the same
// stack memory that just caused an access violation, and dbghelp is not guaranteed safe against
// a corrupted frame chain. If the walk faults we keep whatever frames were already written and
// still emit the rest of the crash report, instead of losing the report entirely.
static void WalkStackGuarded(HANDLE process, const CONTEXT* sourceContext, char* out, size_t outSize)
{
    __try
    {
        CONTEXT walkContext = *sourceContext;
        STACKFRAME64 frame = {};
        frame.AddrPC.Offset = walkContext.Rip;
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Offset = walkContext.Rbp;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrStack.Offset = walkContext.Rsp;
        frame.AddrStack.Mode = AddrModeFlat;

        // SYMBOL_INFO is variable-length: the name is written past the end of the struct.
        char symbolBuffer[sizeof(SYMBOL_INFO) + 1024] = {};
        SYMBOL_INFO* symbol = (SYMBOL_INFO*)symbolBuffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 1023;

        size_t used = 0;
        for (int depth = 0; depth < 48; ++depth)
        {
            if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, GetCurrentThread(),
                             &frame, &walkContext, NULL,
                             SymFunctionTableAccess64, SymGetModuleBase64, NULL))
            {
                break;
            }
            if (frame.AddrPC.Offset == 0)
                break;
            if (used + 512 >= outSize)
                break;

            DWORD64 symDisplacement = 0;
            const bool haveSym = (SymFromAddr(process, frame.AddrPC.Offset, &symDisplacement, symbol) != FALSE);

            IMAGEHLP_LINE64 frameLine = { 0 };
            frameLine.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            DWORD lineDisplacement = 0;
            const bool haveLine = (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &lineDisplacement, &frameLine) != FALSE);

            int written = _snprintf_s(out + used, outSize - used, _TRUNCATE,
                "  [%d] 0x%llx  %s",
                depth, (unsigned long long)frame.AddrPC.Offset,
                haveSym ? symbol->Name : "<no symbol>");
            if (written < 0) break;
            used += (size_t)written;

            if (haveSym)
            {
                written = _snprintf_s(out + used, outSize - used, _TRUNCATE,
                    " + 0x%llx", (unsigned long long)symDisplacement);
                if (written < 0) break;
                used += (size_t)written;
            }
            if (haveLine)
            {
                written = _snprintf_s(out + used, outSize - used, _TRUNCATE,
                    "  (%s:%lu)", frameLine.FileName, (unsigned long)frameLine.LineNumber);
                if (written < 0) break;
                used += (size_t)written;
            }
            written = _snprintf_s(out + used, outSize - used, _TRUNCATE, "\r\n");
            if (written < 0) break;
            used += (size_t)written;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        // Whatever frames made it into `out` are still worth having.
        const char* note = "  <stack walk faulted - frames above are all that could be recovered>\r\n";
        size_t len = strlen(out);
        if (outSize > len + strlen(note) + 1)
            strcpy_s(out + len, outSize - len, note);
    }
}

// Crash handler
LONG WINAPI CrashHandler(EXCEPTION_POINTERS* pExceptionInfo)
{
    // TODO: removed MessageBox during DX12 migration debugging — re-enable when stable
    // MessageBoxA(
    //     NULL,
    //     "A crash has been detected! A crash report has been created in file 'Guru-Crash.log'",
    //     "GameGuru MAX Crash",
    //     MB_OK | MB_ICONERROR
    // );

    // Get path to the EXE folder
    char exeFile[MAX_PATH];
    GetModuleFileNameA(NULL, exeFile, MAX_PATH);

    char exePath[MAX_PATH];
    strcpy_s(exePath, MAX_PATH, exeFile);
    char* lastSlash = strrchr(exePath, '\\');
    if (lastSlash) *(lastSlash + 1) = '\0';

    // Build paths
    char logPath[MAX_PATH];
    strcpy_s(logPath, exePath);
    strcat_s(logPath, "Guru-Crash.log");

    // Initialize symbol handler
    HANDLE process = GetCurrentProcess();

    // GGMAX 3.47 (DX11 0471a9a3): a failed SymInitialize used to "return 1" right here and write
    // NOTHING - no log, no minidump. That is the worst possible outcome, because the crash report
    // is the single artefact we ask a tester to send us. Degrade instead of vanishing: retry
    // without invading the process, and if even that fails carry on and emit an UNSYMBOLISED
    // report. Raw addresses plus the shipped PDB still locate the crash site offline.
    // (The "Failed to initialize symbols." MessageBoxA that sat in the old failure branch
    //  was already commented out for DX12 migration debugging; that branch no longer aborts,
    //  so the modal is gone for good rather than pending re-enable.)
    bool bInvadeProcessMode = true;   // invade=TRUE worked, so runtime addresses resolve directly
    bool bSymbolsAvailable = true;    // dbghelp is usable at all
    DWORD dwSymInitInvadeError = 0;   // GetLastError() from the failed invade attempt

    SymSetOptions(SymGetOptions() | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

    if (!SymInitialize(process, NULL, TRUE))
    {
        dwSymInitInvadeError = GetLastError();
        bInvadeProcessMode = false;
        bSymbolsAvailable = false;

        SymCleanup(process); // result ignored - this only resets state before the second attempt

        // invade=TRUE enumerates and loads every module in the process; when that fails the
        // cheaper invade=FALSE form usually still succeeds, and we load the EXE ourselves below.
        if (SymInitialize(process, NULL, FALSE))
        {
            bSymbolsAvailable = true;
        }
    }

    // Load the module (EXE). Skipped when dbghelp never came up: SymLoadModuleEx would merely
    // return 0, but baseAddress must still be defined for the report lines further down.
    DWORD64 baseAddress = 0;
    if (bSymbolsAvailable)
    {
        // NOTE (DX12): we deliberately keep passing the REAL runtime base here. DX11 0471a9a3
        // changed this argument to 0 ("let DbgHelp decide") and then needed a TranslateAddrIfNeeded
        // helper to convert every runtime address to the preferred base whenever invade had
        // failed. Handing dbghelp the runtime base makes both modes agree, so no address
        // translation is needed anywhere below and the working invade=TRUE path is unchanged.
        baseAddress = SymLoadModuleEx(
            process,
            NULL,
            exeFile,
            NULL,
            (DWORD64)GetModuleHandle(NULL),
            0,
            NULL,
            0
        );
    }

    // the address we need is not the runtime address the exception provides!
    DWORD64 moduleBase = (DWORD64)GetModuleHandle(NULL);
    // GGMAX 3.44: pExceptionInfo is NULL when CError.cpp calls CrashHandler(NULL) for a Lua
    // runtime error - phase D wired that caller up without these guards. Dereferencing it here
    // faulted INSIDE the crash handler, so the one error class this feature exists to report
    // produced no log at all. Every use below is now guarded.
    DWORD64 crashAddress = pExceptionInfo ? (DWORD64)pExceptionInfo->ExceptionRecord->ExceptionAddress : 0;
    DWORD64 offset = crashAddress - moduleBase;
    DWORD64 lookupAddress = baseAddress + offset;

    // Get source line info
    std::string lineInfo;
    IMAGEHLP_LINE64 lineData = { 0 };
    DWORD displacement = 0;
    lineData.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    //if (SymGetLineFromAddr64(process, lookupAddress, &displacement, &lineData))
    //PE: Use ExceptionAddress directly.
    if (pExceptionInfo && bSymbolsAvailable && SymGetLineFromAddr64(process, (DWORD64)pExceptionInfo->ExceptionRecord->ExceptionAddress, &displacement, &lineData))
    {
        std::ostringstream l;
        l << lineData.FileName << ":" << lineData.LineNumber;
        lineInfo = l.str();
    }
    // NOTE: SymCleanup deliberately happens AFTER the stack walk below — it used to run here,
    // which would have torn down the symbol handler the walk depends on.

    std::ostringstream log;
    log << "\r\n==== GAMEGURU MAX CRASH DETECTED ====\r\n";
    log << "Time:            " << GetTimestamp() << "\r\n";
    log << "Build:           " << g_pCrashVersionINIValue << "\r\n";
    // Say plainly how much of the rest of this report can be trusted. A silently unsymbolised
    // log just looks like a log with a useless stack; one that says so is still actionable.
    if (!bInvadeProcessMode)
    {
        log << "SymInitialize:   invade-process attempt FAILED (err " << std::dec << dwSymInitInvadeError << ")";
        log << (bSymbolsAvailable ? " - retried with invade=FALSE, symbols OK\r\n"
                                  : " - retry also FAILED, this report is UNSYMBOLISED\r\n");
    }
    // Which thread died matters: a fault on a jobsystem worker points at background work
    // (texture streaming, terrain generation) rather than anything the frame loop did.
    log << "Thread id:       " << std::dec << GetCurrentThreadId()
        << (GetCurrentThreadId() == g_dwCrashMainThreadId ? "  (MAIN THREAD)" : "  (worker thread)") << "\r\n";
    if (pExceptionInfo)
        log << "Exception code:  0x" << std::hex << pExceptionInfo->ExceptionRecord->ExceptionCode << "\r\n";
    else
        log << "Exception code:  (none - reported by RunTimeError, not a hardware fault)\r\n";
    log << "Module address:  0x" << std::hex << moduleBase << "\r\n";
    log << "Crash address:   0x" << std::hex << crashAddress << "\r\n";
    log << "Base address:    0x" << std::hex << baseAddress << "\r\n";
    log << "Offset value:    0x" << std::hex << offset << "\r\n";
    log << "Lookup address:  0x" << std::hex << lookupAddress << "\r\n";
    if (!lineInfo.empty())
    {
        log << "Source Code:     " << lineInfo << "\r\n";
    }

    // An access violation carries which operation failed and at what address. "Crash address"
    // above is the INSTRUCTION; this is the DATA pointer it tried to touch. For an out-of-bounds
    // read the two are unrelated, and only this one tells you how far off the end you went.
    if (pExceptionInfo
     && pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION
     && pExceptionInfo->ExceptionRecord->NumberParameters >= 2)
    {
        const ULONG_PTR opType = pExceptionInfo->ExceptionRecord->ExceptionInformation[0];
        const ULONG_PTR badAddr = pExceptionInfo->ExceptionRecord->ExceptionInformation[1];
        const char* opName = (opType == 0) ? "READ" : (opType == 1) ? "WRITE" : (opType == 8) ? "EXECUTE(DEP)" : "UNKNOWN";
        log << "AV operation:    " << opName << "\r\n";
        log << "AV address:      0x" << std::hex << (DWORD64)badAddr << "\r\n";
        // Tell us whether the page is simply unmapped (ran off the end of a heap block) or
        // mapped-but-protected. Unmapped strongly implies a buffer overrun, not a stale pointer.
        MEMORY_BASIC_INFORMATION mbi = {};
        if (VirtualQuery((LPCVOID)badAddr, &mbi, sizeof(mbi)) == sizeof(mbi))
        {
            log << "AV page state:   0x" << std::hex << mbi.State << " (0x1000=COMMIT 0x2000=RESERVE 0x10000=FREE)\r\n";
        }
        else
        {
            log << "AV page state:   unqueryable\r\n";
        }
    }

    // Full symbolized call stack of the faulting thread. Without this, a crash inside a CRT
    // routine like memcpy names the victim and never the caller, which is useless for anything
    // that gets called from a hundred places.
    log << "---- CALL STACK (faulting thread) ----\r\n";
    {
        // Written into a plain buffer by an SEH-guarded helper (see WalkStackGuarded): the walk
        // itself dereferences the very stack that just faulted, so it can fault in turn. A crash
        // handler that crashes produces NO report at all, which is worse than no stack.
        static char stackText[48 * 512];
        stackText[0] = 0;
        if (pExceptionInfo && bSymbolsAvailable)
        {
            WalkStackGuarded(process, pExceptionInfo->ContextRecord, stackText, sizeof(stackText));
            log << stackText;
        }
        else if (pExceptionInfo && pExceptionInfo->ContextRecord)
        {
            // Without a symbol handler StackWalk64 cannot unwind x64 at all (it needs dbghelp's
            // function-table access for the unwind data), so the best we can do is the raw
            // faulting PC plus its offset inside the module that owns it - and that offset is
            // all anyone needs to resolve the site offline against the shipped PDB.
            const DWORD64 rawPC = (DWORD64)pExceptionInfo->ContextRecord->Rip;
            log << "  (no symbol handler - stack walk skipped, address is raw)\r\n";
            log << "  [0] 0x" << std::hex << rawPC;
            // Ask which module owns that PC rather than assuming the EXE: a fault inside a
            // driver or runtime DLL would otherwise be reported as a nonsense EXE offset.
            HMODULE hFaultMod = NULL;
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   (LPCSTR)rawPC, &hFaultMod) && hFaultMod != NULL)
            {
                char faultModPath[MAX_PATH] = {};
                GetModuleFileNameA(hFaultMod, faultModPath, MAX_PATH);
                const char* faultModName = strrchr(faultModPath, '\\');
                faultModName = faultModName ? faultModName + 1 : faultModPath;
                log << "  (" << faultModName << "+0x" << std::hex << (rawPC - (DWORD64)hFaultMod) << ")";
            }
            log << "\r\n";
        }
        else
        {
            log << "  (no context record - software-reported error, not a hardware fault)\r\n";
        }
    }
    // GGMAX 3.44 (DX11 1aacfb90): the breadcrumb ring buffer was being filled by HideLimb/
    // ShowLimb and by RunTimeError and then THROWN AWAY - nothing ever read it. Every
    // breadcrumb the port installed was wasted until this line.
    {
        extern thread_local char g_CrashContext[1024];
        if (g_CrashContext[0])
        {
            log << "---- BREADCRUMBS (most recent last) ----\r\n";
            log << g_CrashContext << "\r\n";
        }
    }
    log << "=====================================\r\n";

    if (bSymbolsAvailable)
        SymCleanup(process);

    // Write to log
    HANDLE hFile = CreateFileA(logPath, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        WriteFile(hFile, log.str().c_str(), (DWORD)log.str().size(), &bytesWritten, NULL);
        FlushFileBuffers(hFile);
        CloseHandle(hFile);
    }

    // Also create dump that we can load in visual studio later to debug.
    strcpy_s(logPath, exePath);
    strcat_s(logPath, "crashdump.dmp");

    hFile = CreateFileA(logPath, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    if (pExceptionInfo)
    {
        MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
        exceptionInfo.ThreadId = GetCurrentThreadId();
        exceptionInfo.ExceptionPointers = pExceptionInfo;
        exceptionInfo.ClientPointers = TRUE;
        BOOL success = MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            MiniDumpNormal, // MiniDumpWithFullMemory,
            &exceptionInfo,
            nullptr,
            nullptr
        );
    }

    CloseHandle(hFile);
    Sleep(100);
    return EXCEPTION_EXECUTE_HANDLER;
}

void InitCrashHandler()
{
    g_dwCrashMainThreadId = GetCurrentThreadId();
    SetUnhandledExceptionFilter(CrashHandler);
}

char* GetCrashHandlerDebugLogRef()
{
    if (strlen(g_pDebugExtraInfo) == 0) strcpy_s(g_pDebugExtraInfo, 10240, "GFX Debug Log:");
    return g_pDebugExtraInfo;
}
