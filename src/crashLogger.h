#ifndef CRASH_LOGGER_H
#define CRASH_LOGGER_H

#include <string>
#include <vector>
#include <chrono>
#include <memory>

class CrashLogger
{
public:
    static CrashLogger* getInstance();
    
    // Struct definitions
    struct NetworkEvent {
        std::chrono::system_clock::time_point timestamp;
        std::string type;
        std::string details;
    };
    
    struct GameEvent {
        std::chrono::steady_clock::time_point timestamp;
        std::string eventType;
        std::string details;
    };
    
    struct GameContext {
        std::chrono::system_clock::time_point timestamp;
        std::string currentScreen;
        std::string currentPlayer;
        std::string currentSystem;
        std::string context;
    };
    
    // Log connection state changes with additional context
    void logConnectionStateChange(const std::string& oldState, const std::string& newState, 
                                const std::string& serverAddress, const std::string& additionalInfo = "");
    
    // Log network-related events that might indicate connection issues
    void logNetworkEvent(const std::string& eventType, const std::string& details);
    
    // Log potential crash indicators
    void logCrashIndicator(const std::string& indicator, const std::string& context);
    
    // Log Lua script errors specifically
    void logLuaError(const std::string& error, const std::string& stackTrace);
    
    // Log Lua script execution issues
    void logLuaExecutionIssue(const std::string& issue, const std::string& context);
    
    // Set up crash handlers for different platforms
    void setupCrashHandlers();
    
    // Save current state to crash log file
    void saveCrashContext(const std::string& crashReason, void* exceptionContext = nullptr);
    
    // Get recent network events for debugging
    std::vector<std::string> getRecentNetworkEvents();
    
    // Clean up old crash logs
    void cleanupOldLogs();
    
    // Enhanced crash context methods
    void logGameEvent(const std::string& eventType, const std::string& details);
    void logScreenChange(const std::string& screenName);
    void logPlayerAction(const std::string& action, const std::string& details = "");
    void setCurrentContext(const GameContext& context);
    
    // Soft crash detection (game returns to main menu unexpectedly)
    void logSoftCrash(const std::string& reason, const std::string& context = "");
    void logUnexpectedMenuReturn(const std::string& previousScreen, const std::string& reason);
    void logGameStateReset(const std::string& reason, const std::string& previousState);
    void logErrorRecovery(const std::string& errorType, const std::string& recoveryAction);
    
    // Check if we're in a soft crash state
    bool isInSoftCrashState() const { return softCrashDetected; }
    std::string getLastSoftCrashReason() const { return lastSoftCrashReason; }

private:
    CrashLogger();
    ~CrashLogger();
    
    static CrashLogger* instance;
    
    std::vector<NetworkEvent> recentEvents;
    std::vector<GameEvent> recentGameEvents;
    std::vector<GameContext> gameContext;
    std::string currentContext;
    GameContext currentGameContext;
    std::string logDirectory;
    std::string logFilePath;
    std::string crashLogPath;
    
    void writeToLog(const std::string& message);
    void writeCrashLog(const std::string& crashReason, void* exceptionContext = nullptr);
    std::string getCurrentTimestamp();
    std::string getSystemInfo();
    std::string getConnectionState();
    
    // Stack trace generation
    std::string generateStackTrace(void* exceptionContext = nullptr);

#ifdef _WIN32
    void initializeSymbols();
    void shutdownSymbols();
    bool symbolsInitialized;
#endif

    std::string lastSoftCrashReason;
    bool softCrashDetected;
    std::chrono::steady_clock::time_point lastSoftCrashTime;
    std::chrono::steady_clock::time_point lastCrashTime;
    
    // Soft crash detection helpers
    void writeSoftCrashLog(const std::string& reason, const std::string& context);
    void detectSoftCrashPatterns();
};

#endif // CRASH_LOGGER_H
