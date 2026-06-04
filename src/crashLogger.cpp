#include "crashLogger.h"
#include "logging.h"
#include "preferenceManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <map>
#include <iomanip>
#include <chrono>
#include <exception>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>
#include <winbase.h>
#elif defined(__linux__)
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#elif defined(__APPLE__)
#include <signal.h>
#include <execinfo.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#endif

CrashLogger* CrashLogger::instance = nullptr;

CrashLogger::CrashLogger()
{
    lastCrashTime = std::chrono::steady_clock::now();
    lastSoftCrashReason = "";
    softCrashDetected = false;
    lastSoftCrashTime = std::chrono::steady_clock::now();
    
    // Create logs directory if it doesn't exist
#ifdef _WIN32
    _mkdir("logs");
#else
    mkdir("logs", 0755);
#endif
    
    // Set up log file paths
    logDirectory = "logs";
    logFilePath = "logs/connection_log.txt";
    crashLogPath = "logs/crash_log.txt";
    
    // Clean up old logs on startup
    cleanupOldLogs();
    
    // Set up crash handlers
    setupCrashHandlers();
    
    LOG(INFO) << "CrashLogger initialized";
}

CrashLogger::~CrashLogger()
{
    // Save any pending events before shutdown
    if (!recentEvents.empty()) {
        writeToLog("Application shutdown - saving pending events");
    }
}

CrashLogger* CrashLogger::getInstance()
{
    if (!instance) {
        instance = new CrashLogger();
    }
    return instance;
}

void CrashLogger::logConnectionStateChange(const std::string& oldState, const std::string& newState, 
                                         const std::string& serverAddress, const std::string& additionalInfo)
{
    std::string message = "CONNECTION_STATE_CHANGE: " + oldState + " -> " + newState + 
                         " (Server: " + serverAddress + ")";
    if (!additionalInfo.empty()) {
        message += " - " + additionalInfo;
    }
    
    // Only log to regular logging system for connection changes
    // Don't spam the crash log with routine connection info
    LOG(INFO) << message;
    
    // Only write to crash log if it's a disconnection (potential problem)
    if (newState == "Disconnected" || newState == "Failed") {
        writeToLog(message);
        
        // Store in recent events for crash context
        NetworkEvent event;
        event.timestamp = std::chrono::system_clock::now();
        event.type = "connection_state_change";
        event.details = message;
        recentEvents.push_back(event);
        
        // Keep only last 100 events to prevent memory bloat
        if (recentEvents.size() > 100) {
            recentEvents.erase(recentEvents.begin());
        }
    }
}

void CrashLogger::logNetworkEvent(const std::string& eventType, const std::string& details)
{
    std::string message = "NETWORK_EVENT: " + eventType + " - " + details;
    
    // Only log network events to crash log if they're important
    // Skip routine network operations to reduce noise
    if (eventType.find("ping") == std::string::npos && 
        eventType.find("heartbeat") == std::string::npos) {
        writeToLog(message);
        
        // Store in recent events
        NetworkEvent event;
        event.timestamp = std::chrono::system_clock::now();
        event.type = "network_event";
        event.details = message;
        recentEvents.push_back(event);
        
        // Keep only last 100 events
        if (recentEvents.size() > 100) {
            recentEvents.erase(recentEvents.begin());
        }
    }
}

void CrashLogger::logCrashIndicator(const std::string& indicator, const std::string& context)
{
    std::string message = "CRASH_INDICATOR: " + indicator + " - " + context;
    
    LOG(WARNING) << message;
    writeToLog(message);
    
    // Store in recent events
    NetworkEvent event;
    event.timestamp = std::chrono::system_clock::now();
    event.type = "crash_indicator";
    event.details = message;
    recentEvents.push_back(event);
    
    // Keep only last 100 events
    if (recentEvents.size() > 100) {
        recentEvents.erase(recentEvents.begin());
    }
}

void CrashLogger::logLuaError(const std::string& error, const std::string& stackTrace) {
    std::string message = "LUA ERROR: " + error;
    if (!stackTrace.empty()) {
        message += "\nStack Trace:\n" + stackTrace;
    }
    writeToLog(message);
    
    // Log as game event for soft crash detection
    logGameEvent("Lua Error", error);
    
    // Check if this Lua error might cause a soft crash
    if (error.find("script") != std::string::npos || 
        error.find("function") != std::string::npos ||
        error.find("table") != std::string::npos) {
        logSoftCrash("Lua script error", error);
    }
}

void CrashLogger::logGameEvent(const std::string& eventType, const std::string& details) {
    GameEvent event;
    event.timestamp = std::chrono::steady_clock::now();
    event.eventType = eventType;
    event.details = details;
    
    recentGameEvents.push_back(event);
    
    // Keep only last 50 events
    if (recentGameEvents.size() > 50) {
        recentGameEvents.erase(recentGameEvents.begin());
    }
    
    // Check for soft crash patterns after logging
    detectSoftCrashPatterns();
}

void CrashLogger::logScreenChange(const std::string& screenName) {
    logGameEvent("Screen Change", screenName);
    
    // Check if this is an unexpected return to main menu
    if (screenName.find("main") != std::string::npos || screenName.find("Main") != std::string::npos) {
        // Check if we were recently in a game screen
        for (auto it = recentGameEvents.rbegin(); it != recentGameEvents.rend(); ++it) {
            if (it->eventType == "Screen Change" && 
                (it->details.find("game") != std::string::npos || 
                 it->details.find("Game") != std::string::npos ||
                 it->details.find("crew") != std::string::npos ||
                 it->details.find("Crew") != std::string::npos)) {
                logUnexpectedMenuReturn(it->details, "Unexpected return to main menu");
                break;
            }
        }
    }
}

void CrashLogger::logPlayerAction(const std::string& action, const std::string& details) {
    std::string fullAction = action;
    if (!details.empty()) {
        fullAction += ": " + details;
    }
    logGameEvent("Player Action", fullAction);
}

void CrashLogger::setCurrentContext(const GameContext& context) {
    currentGameContext = context;
    
    // Log the context change
    std::string contextStr = "Screen: " + context.currentScreen + 
                            " | Player: " + context.currentPlayer + 
                            " | System: " + context.currentSystem;
    logGameEvent("Context Change", contextStr);
}

void CrashLogger::logLuaExecutionIssue(const std::string& issue, const std::string& context) {
    std::string message = "LUA EXECUTION ISSUE: " + issue;
    if (!context.empty()) {
        message += " | Context: " + context;
    }
    writeToLog(message);
    
    // Log as game event for soft crash detection
    logGameEvent("Lua Execution Issue", issue + " | " + context);
    
    // Check if this execution issue might cause a soft crash
    if (issue.find("timeout") != std::string::npos || 
        issue.find("infinite") != std::string::npos ||
        issue.find("recursion") != std::string::npos) {
        logSoftCrash("Lua execution problem", issue + " | " + context);
    }
}

// Global exception handler for Windows
#ifdef _WIN32
static LONG WINAPI exceptionHandler(EXCEPTION_POINTERS* exceptionInfo) {
    std::string crashReason = "Windows Exception";
    void* exceptionContext = nullptr;
    
    // Get specific exception details
    if (exceptionInfo && exceptionInfo->ExceptionRecord) {
        DWORD exceptionCode = exceptionInfo->ExceptionRecord->ExceptionCode;
        exceptionContext = exceptionInfo->ContextRecord;
        
        // Get fault address for access violations
        std::string faultAddress = "";
        if (exceptionCode == EXCEPTION_ACCESS_VIOLATION && exceptionInfo->ExceptionRecord->NumberParameters >= 2) {
            void* faultAddr = (void*)exceptionInfo->ExceptionRecord->ExceptionInformation[1];
            std::stringstream ss;
            ss << " at address 0x" << std::hex << faultAddr;
            faultAddress = ss.str();
        }
        
        switch (exceptionCode) {
            case EXCEPTION_ACCESS_VIOLATION:
                crashReason = "Windows Exception: Access Violation (null pointer, invalid memory access)" + faultAddress;
                break;
            case EXCEPTION_STACK_OVERFLOW:
                crashReason = "Windows Exception: Stack Overflow";
                break;
            case EXCEPTION_INT_DIVIDE_BY_ZERO:
                crashReason = "Windows Exception: Division by Zero";
                break;
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:
                crashReason = "Windows Exception: Floating Point Division by Zero";
                break;
            case EXCEPTION_ILLEGAL_INSTRUCTION:
                crashReason = "Windows Exception: Illegal Instruction";
                break;
            default: {
                std::stringstream code_ss;
                code_ss << "Windows Exception: Code 0x" << std::hex << exceptionCode << std::dec;
                crashReason = code_ss.str();
                break;
            }
        }
    }
    
    // Generate and save stack trace
    CrashLogger* logger = CrashLogger::getInstance();
    logger->saveCrashContext(crashReason, exceptionContext);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

void CrashLogger::setupCrashHandlers()
{
#ifdef _WIN32
    // Windows: Set up structured exception handler
    SetUnhandledExceptionFilter(exceptionHandler);
    
    // Uncaught C++ exceptions call std::terminate; try to include std::exception::what() when available.
    std::set_terminate([]() {
        std::string reason = "C++ Exception";
        try
        {
            if (std::current_exception())
                std::rethrow_exception(std::current_exception());
        }
        catch (const std::exception& e)
        {
            reason += std::string(": ");
            reason += e.what();
        }
        catch (...)
        {
            reason += " (non-standard exception type)";
        }
        CrashLogger::getInstance()->saveCrashContext(reason);
        std::abort();
    });
    
#elif defined(__linux__) || defined(__APPLE__)
    // Unix-like systems: Set up signal handlers
    signal(SIGSEGV, [](int sig) {
        CrashLogger::getInstance()->saveCrashContext("Segmentation Fault (SIGSEGV)");
        exit(1);
    });
    
    signal(SIGABRT, [](int sig) {
        CrashLogger::getInstance()->saveCrashContext("Abort Signal (SIGABRT)");
        exit(1);
    });
    
    signal(SIGFPE, [](int sig) {
        CrashLogger::getInstance()->saveCrashContext("Floating Point Exception (SIGFPE)");
        exit(1);
    });
    
    signal(SIGILL, [](int sig) {
        CrashLogger::getInstance()->saveCrashContext("Illegal Instruction (SIGILL)");
        exit(1);
    });
    
    signal(SIGTERM, [](int sig) {
        CrashLogger::getInstance()->saveCrashContext("Termination Signal (SIGTERM)");
        exit(1);
    });
    
    std::set_terminate([]() {
        std::string reason = "C++ Exception";
        try
        {
            if (std::current_exception())
                std::rethrow_exception(std::current_exception());
        }
        catch (const std::exception& e)
        {
            reason += std::string(": ");
            reason += e.what();
        }
        catch (...)
        {
            reason += " (non-standard exception type)";
        }
        CrashLogger::getInstance()->saveCrashContext(reason);
        std::abort();
    });
#endif
}

void CrashLogger::saveCrashContext(const std::string& crashReason, void* exceptionContext)
{
    try {
        writeCrashLog(crashReason, exceptionContext);
    } catch (...) {
        // If crash logging fails, at least try to write to stderr
        std::cerr << "Failed to save crash context: " << crashReason << std::endl;
    }
}

std::vector<std::string> CrashLogger::getRecentNetworkEvents()
{
    std::vector<std::string> events;
    for (const auto& event : recentEvents) {
        std::stringstream ss;
        ss << getCurrentTimestamp() << " - " << event.type << ": " << event.details;
        events.push_back(ss.str());
    }
    return events;
}

void CrashLogger::cleanupOldLogs()
{
    try {
        // Keep only last 10 crash logs and connection logs
        std::vector<std::string> logFiles = {"logs/connection_log.txt", "logs/crash_log.txt"};
        
        for (const auto& logFile : logFiles) {
#ifdef _WIN32
            if (_access(logFile.c_str(), 0) == 0) {
                // Check file size - if over 10MB, truncate it
                // Use Windows API directly to avoid MinGW stat naming conflicts
                WIN32_FILE_ATTRIBUTE_DATA fileAttr;
                if (GetFileAttributesExA(logFile.c_str(), GetFileExInfoStandard, &fileAttr)) {
                    ULARGE_INTEGER fileSize;
                    fileSize.LowPart = fileAttr.nFileSizeLow;
                    fileSize.HighPart = fileAttr.nFileSizeHigh;
                    if (fileSize.QuadPart > 10 * 1024 * 1024) { // 10MB
                        std::ofstream file(logFile, std::ios::trunc);
                        file << "Log file truncated due to size limit\n";
                        file.close();
                    }
                }
            }
#else
            if (access(logFile.c_str(), F_OK) == 0) {
                // Check file size - if over 10MB, truncate it
                struct stat fileStat;
                if (stat(logFile.c_str(), &fileStat) == 0) {
                    auto fileSize = fileStat.st_size;
                    if (fileSize > 10 * 1024 * 1024) { // 10MB
                        std::ofstream file(logFile, std::ios::trunc);
                        file << "Log file truncated due to size limit\n";
                        file.close();
                    }
                }
            }
#endif
        }
    } catch (...) {
        // Ignore cleanup errors
    }
}

void CrashLogger::writeToLog(const std::string& message)
{
    try {
        std::ofstream logFile(logFilePath, std::ios::app);
        if (logFile.is_open()) {
            logFile << getCurrentTimestamp() << " - " << message << std::endl;
            logFile.close();
        }
    } catch (...) {
        // Ignore write errors to prevent crashes in crash logger
    }
}

void CrashLogger::writeCrashLog(const std::string& crashReason, void* exceptionContext)
{
    try {
        std::ofstream crashFile(crashLogPath, std::ios::app);
        if (crashFile.is_open()) {
            crashFile << "=== CRASH REPORT ===" << std::endl;
            crashFile << "Timestamp: " << getCurrentTimestamp() << std::endl;
            crashFile << "Crash Reason: " << crashReason << std::endl;
            crashFile << "System Info: " << getSystemInfo() << std::endl;
            crashFile << "Connection State: " << getConnectionState() << std::endl;
            
            // Add stack trace
            std::string stackTrace = generateStackTrace(exceptionContext);
            if (!stackTrace.empty()) {
                crashFile << "\n=== STACK TRACE ===" << std::endl;
                crashFile << stackTrace << std::endl;
            }
            
            // Group events by type for better readability
            std::map<std::string, std::vector<std::string>> eventsByType;
            for (const auto& event : recentEvents) {
                eventsByType[event.type].push_back(event.details);
            }
            
            crashFile << "Recent Events by Type:" << std::endl;
            for (const auto& typeGroup : eventsByType) {
                crashFile << "  " << typeGroup.first << " (" << typeGroup.second.size() << " events):" << std::endl;
                for (const auto& detail : typeGroup.second) {
                    crashFile << "    " << detail << std::endl;
                }
            }
            
            // Add enhanced game context information
            if (!gameContext.empty()) {
                crashFile << "\n=== GAME CONTEXT BEFORE CRASH ===" << std::endl;
                crashFile << "Current Context: " << currentContext << std::endl;
                crashFile << "Recent Game Events:" << std::endl;
                
                // Group game context by type
                std::map<std::string, std::vector<std::string>> contextByType;
                for (const auto& context : recentEvents) {
                    contextByType[context.type].push_back(context.details);
                }
                
                for (const auto& typeGroup : contextByType) {
                    crashFile << "  " << typeGroup.first << " (" << typeGroup.second.size() << " events):" << std::endl;
                    for (const auto& detail : typeGroup.second) {
                        crashFile << "    " << detail << std::endl;
                    }
                }
            }
            
            // Highlight Lua errors specifically
            auto luaErrors = eventsByType.find("lua_error");
            if (luaErrors != eventsByType.end()) {
                crashFile << "!!! LUA ERRORS DETECTED - These may have caused the crash !!!" << std::endl;
                for (const auto& error : luaErrors->second) {
                    crashFile << "  LUA ERROR: " << error << std::endl;
                }
            }
            
            crashFile << "=== END CRASH REPORT ===" << std::endl << std::endl;
            crashFile.close();
        }
    } catch (...) {
        // Ignore write errors
    }
}

std::string CrashLogger::getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string CrashLogger::getSystemInfo()
{
    std::stringstream ss;
    
#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    ss << "Windows, Processors: " << sysInfo.dwNumberOfProcessors;
    
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    ss << ", Memory: " << (memInfo.ullTotalPhys / (1024*1024*1024)) << "GB";
    
#elif defined(__linux__)
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        ss << "Linux, Uptime: " << si.uptime << "s";
        ss << ", Memory: " << (si.totalram / (1024*1024*1024)) << "GB";
    }
    
#elif defined(__APPLE__)
    ss << "macOS";
    host_basic_info_data_t hostInfo;
    mach_msg_type_number_t infoCount = HOST_BASIC_INFO_COUNT;
    host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)&hostInfo, &infoCount);
    ss << ", Processors: " << hostInfo.physical_cpu;
#endif
    
    return ss.str();
}

std::string CrashLogger::getConnectionState()
{
    // This would need to be implemented based on the actual game state
    // For now, return a placeholder
    return "Connection state not available";
}

// Soft crash detection methods
void CrashLogger::logSoftCrash(const std::string& reason, const std::string& context) {
    softCrashDetected = true;
    lastSoftCrashReason = reason;
    lastSoftCrashTime = std::chrono::steady_clock::now();
    
    writeSoftCrashLog(reason, context);
    
    // Also log to main log
    std::string message = "SOFT CRASH DETECTED: " + reason;
    if (!context.empty()) {
        message += " | Context: " + context;
    }
    writeToLog(message);
}

void CrashLogger::logUnexpectedMenuReturn(const std::string& previousScreen, const std::string& reason) {
    std::string context = "Previous screen: " + previousScreen;
    if (!reason.empty()) {
        context += " | Reason: " + reason;
    }
    logSoftCrash("Unexpected return to main menu", context);
}

void CrashLogger::logGameStateReset(const std::string& reason, const std::string& previousState) {
    std::string context = "Previous state: " + previousState;
    if (!reason.empty()) {
        context += " | Reason: " + reason;
    }
    logSoftCrash("Game state reset", context);
}

void CrashLogger::logErrorRecovery(const std::string& errorType, const std::string& recoveryAction) {
    std::string message = "ERROR RECOVERY: " + errorType + " | Action: " + recoveryAction;
    writeToLog(message);
    
    // Check if this recovery suggests a soft crash
    if (recoveryAction.find("menu") != std::string::npos || 
        recoveryAction.find("reset") != std::string::npos ||
        recoveryAction.find("restart") != std::string::npos) {
        logSoftCrash("Error recovery triggered", errorType + " -> " + recoveryAction);
    }
}

void CrashLogger::writeSoftCrashLog(const std::string& reason, const std::string& context) {
    std::string logFile = logDirectory + "/soft_crash_log.txt";
    
    std::ofstream file(logFile, std::ios::app);
    if (file.is_open()) {
        file << "=== SOFT CRASH REPORT ===" << std::endl;
        file << "Timestamp: " << getCurrentTimestamp() << std::endl;
        file << "Reason: " << reason << std::endl;
        if (!context.empty()) {
            file << "Context: " << context << std::endl;
        }
        file << "System Info: " << getSystemInfo() << std::endl;
        file << "Connection State: " << getConnectionState() << std::endl;
        
        // Include recent game context
        if (!recentGameEvents.empty()) {
            file << "Recent Events Before Soft Crash:" << std::endl;
            for (const auto& event : recentGameEvents) {
                // Convert steady_clock timestamp to readable format
                auto duration = event.timestamp.time_since_epoch();
                auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
                file << "  " << seconds << "s - " << event.eventType << ": " << event.details << std::endl;
            }
        }
        
        file << "=== END SOFT CRASH REPORT ===" << std::endl << std::endl;
        file.close();
    }
}

std::string CrashLogger::generateStackTrace(void* exceptionContext)
{
    std::stringstream ss;
    
#ifdef _WIN32
    HANDLE process = GetCurrentProcess();
    if (!SymInitialize(process, NULL, TRUE))
    {
        ss << "  (SymInitialize failed; stack addresses unavailable)\n";
        return ss.str();
    }
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

    void* stack[62];
    USHORT frames = 0;

    if (exceptionContext)
    {
        CONTEXT* ctx = (CONTEXT*)exceptionContext;
        STACKFRAME64 stackFrame = {};

#ifdef _M_IX86
        stackFrame.AddrPC.Offset = ctx->Eip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = ctx->Esp;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = ctx->Ebp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
#elif defined(_M_X64)
        stackFrame.AddrPC.Offset = ctx->Rip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = ctx->Rsp;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = ctx->Rbp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
#endif

        while (StackWalk64(
#ifdef _M_IX86
                   IMAGE_FILE_MACHINE_I386,
#else
                   IMAGE_FILE_MACHINE_AMD64,
#endif
                   process, GetCurrentThread(), &stackFrame, exceptionContext, NULL, SymFunctionTableAccess64,
                   SymGetModuleBase64, NULL)
               && frames < 62)
        {
            stack[frames++] = (void*)stackFrame.AddrPC.Offset;
        }
    }
    else
    {
        frames = CaptureStackBackTrace(0, 62, stack, NULL);
    }

    const DWORD kMaxSymName = 4095;
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + kMaxSymName + 1, 1);
    symbol->MaxNameLen = kMaxSymName;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    IMAGEHLP_LINE64 line = {};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD displacement = 0;

    for (USHORT i = 0; i < frames; i++)
    {
        DWORD64 address = (DWORD64)stack[i];

        if (SymFromAddr(process, address, NULL, symbol))
        {
            ss << "  [" << i << "] " << symbol->Name;

            if (SymGetLineFromAddr64(process, address, &displacement, &line))
                ss << " (" << line.FileName << ":" << line.LineNumber << ")";

            ss << " [0x" << std::hex << address << std::dec << "]" << std::endl;
        }
        else
        {
            DWORD64 moduleBase = SymGetModuleBase64(process, address);
            if (moduleBase != 0)
            {
                char modulePath[MAX_PATH];
                if (GetModuleFileNameA((HMODULE)moduleBase, modulePath, MAX_PATH) > 0)
                {
                    const char* base = strrchr(modulePath, '\\');
                    base = base ? base + 1 : modulePath;
                    DWORD64 rva = address - moduleBase;
                    ss << "  [" << i << "] " << base << "+0x" << std::hex << rva << std::dec
                       << " (no symbol; place PDB next to exe/DLL for names) [0x" << std::hex << address << std::dec << "]"
                       << std::endl;
                }
                else
                    ss << "  [" << i << "] <unknown> [0x" << std::hex << address << std::dec << "]" << std::endl;
            }
            else
                ss << "  [" << i << "] <unknown> [0x" << std::hex << address << std::dec << "]" << std::endl;
        }
    }

    free(symbol);
    SymCleanup(process);

    ss << "\n"
          "Stack hint (Windows): If you only see module+offset, copy EmptyEpsilon.pdb (and SFML/other PDBs if "
          "custom-built) next to the binaries so DbgHelp can resolve functions and lines.\n";
    
#elif defined(__linux__) || defined(__APPLE__)
    void* array[62];
    int size = backtrace(array, 62);
    char** symbols = backtrace_symbols(array, size);
    
    if (symbols) {
        for (int i = 0; i < size; i++) {
            ss << "  [" << i << "] " << symbols[i] << std::endl;
        }
        free(symbols);
    }
#endif
    
    return ss.str();
}

void CrashLogger::detectSoftCrashPatterns() {
    // Analyze recent events for soft crash patterns
    if (recentGameEvents.size() < 3) return;
    
    // Check for rapid screen changes (potential error recovery)
    auto now = std::chrono::steady_clock::now();
    int rapidChanges = 0;
    
    for (auto it = recentGameEvents.rbegin(); it != recentGameEvents.rend() && rapidChanges < 3; ++it) {
        if (it->eventType == "Screen Change") {
            auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->timestamp).count();
            if (timeDiff < 5000) { // Within 5 seconds
                rapidChanges++;
            }
        }
    }
    
    if (rapidChanges >= 3) {
        logSoftCrash("Rapid screen changes detected", "Possible error recovery loop");
    }
    
    // Check for multiple errors in short time
    int recentErrors = 0;
    for (auto it = recentGameEvents.rbegin(); it != recentGameEvents.rend() && recentErrors < 3; ++it) {
        if (it->eventType == "Error" || it->eventType == "Lua Error") {
            auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->timestamp).count();
            if (timeDiff < 10000) { // Within 10 seconds
                recentErrors++;
            }
        }
    }
    
    if (recentErrors >= 3) {
        logSoftCrash("Multiple errors in short time", "Error cascade detected");
    }
}
