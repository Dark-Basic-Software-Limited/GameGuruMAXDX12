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
    
    //if (!SymInitialize(process, NULL, FALSE)) {
    if (!SymInitialize(process, NULL, TRUE))
    {
        // TODO: removed MessageBox during DX12 migration debugging
        // MessageBoxA(NULL, "Failed to initialize symbols.", "GameGuru MAX Crash", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Load the module (EXE)
    DWORD64 baseAddress;
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

    // the address we need is not the runtime address the exception provides!
    DWORD64 moduleBase = (DWORD64)GetModuleHandle(NULL);
    DWORD64 crashAddress = (DWORD64)pExceptionInfo->ExceptionRecord->ExceptionAddress;
    DWORD64 offset = crashAddress - moduleBase;
    DWORD64 lookupAddress = baseAddress + offset;

    // Get source line info
    std::string lineInfo;
    IMAGEHLP_LINE64 lineData = { 0 };
    DWORD displacement = 0;
    lineData.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    //if (SymGetLineFromAddr64(process, lookupAddress, &displacement, &lineData))
    //PE: Use ExceptionAddress directly.
    if (SymGetLineFromAddr64(process, (DWORD64)pExceptionInfo->ExceptionRecord->ExceptionAddress, &displacement, &lineData))
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
    // Which thread died matters: a fault on a jobsystem worker points at background work
    // (texture streaming, terrain generation) rather than anything the frame loop did.
    log << "Thread id:       " << std::dec << GetCurrentThreadId()
        << (GetCurrentThreadId() == g_dwCrashMainThreadId ? "  (MAIN THREAD)" : "  (worker thread)") << "\r\n";
    log << "Exception code:  0x" << std::hex << pExceptionInfo->ExceptionRecord->ExceptionCode << "\r\n";
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
    if (pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION
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
        WalkStackGuarded(process, pExceptionInfo->ContextRecord, stackText, sizeof(stackText));
        log << stackText;
    }
    log << "=====================================\r\n";

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

    //PE: Also create dump that we can load in visual studio later to debug.
    strcpy_s(logPath, exePath);
    strcat_s(logPath, "crashdump.dmp");

    hFile = CreateFileA(logPath, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
    exceptionInfo.ThreadId = GetCurrentThreadId();
    exceptionInfo.ExceptionPointers = pExceptionInfo;
    exceptionInfo.ClientPointers = TRUE;

    BOOL success = MiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        hFile,
        MiniDumpNormal, //MiniDumpWithFullMemory,
        &exceptionInfo,
        nullptr,
        nullptr
    );
    CloseHandle(hFile);

    Sleep(100);
    return EXCEPTION_EXECUTE_HANDLER;
}

void InitCrashHandler()
{
    g_dwCrashMainThreadId = GetCurrentThreadId();
    SetUnhandledExceptionFilter(CrashHandler);
}

