# Crash Logging System Documentation

## Overview
The crash logging system provides comprehensive logging for both hard crashes (game termination) and soft crashes (unexpected returns to main menu, error recovery, etc.) in EmptyEpsilon.

## Features

### Hard Crash Detection
- **Windows Exception Handling**: Catches system-level crashes with detailed exception information
- **Unix Signal Handling**: Catches crashes on Linux/macOS systems
- **Crash Context**: Captures game state, recent events, and system information before crash
- **Automatic Logging**: Writes crash reports to `logs/crash_log.txt`

### Soft Crash Detection
- **Unexpected Menu Returns**: Detects when the game returns to main menu unexpectedly
- **Game State Resets**: Logs when game state is reset due to errors
- **Error Recovery Actions**: Tracks error recovery mechanisms
- **Pattern Detection**: Identifies rapid screen changes, error cascades, and execution issues
- **Automatic Detection**: Monitors game events and detects soft crash patterns
- **Separate Logging**: Writes soft crash reports to `logs/soft_crash_log.txt`

### Network Health Monitoring
- **Connection Quality**: Tracks packet loss, latency, and connection stability
- **Instability Detection**: Identifies patterns that might lead to crashes
- **Real-time Monitoring**: Continuously monitors network health during gameplay

## Log Files

### `logs/crash_log.txt`
Contains detailed crash reports when the game fully crashes:
```
=== CRASH REPORT ===
Timestamp: 2025-08-13 01:42:52.387
Crash Reason: Windows Exception: Access Violation (null pointer, invalid memory access)
System Info: Windows, Processors: 16, Memory: 31GB
Connection State: Connected to server
Recent Events by Type:
  Screen Changes: crew1 -> main -> crew1
  Player Actions: accessed weapons, accessed engineering
  Game Events: ship spawned, enemy detected
=== END CRASH REPORT ===
```

### `logs/soft_crash_log.txt`
Contains reports when the game returns to main menu unexpectedly:
```
=== SOFT CRASH REPORT ===
Timestamp: 2025-08-13 01:42:52.387
Reason: Unexpected return to main menu
Context: Previous screen: crew1 | Reason: Unexpected return to main menu
System Info: Windows, Processors: 16, Memory: 31GB
Connection State: Connected to server
Recent Events Before Soft Crash:
  2025-08-13 01:42:50.123 - Screen Change: crew1
  2025-08-13 01:42:51.456 - Lua Error: script execution failed
  2025-08-13 01:42:52.001 - Screen Change: main
=== END SOFT CRASH REPORT ===
```

### `logs/connection_log.txt`
Contains network-related events and connection state changes:
```
2025-08-13 01:42:52.387 - Connection Lost: Network timeout
2025-08-13 01:42:52.500 - Reconnection Attempt: Connecting to server
2025-08-13 01:42:53.123 - Connection Restored: Successfully reconnected
```

## Usage

### C++ Integration
The crash logger is automatically initialized in `main()` and provides these methods:

```cpp
// Log game events for soft crash detection
CrashLogger::getInstance().logGameEvent("Ship Spawned", "Enemy cruiser");
CrashLogger::getInstance().logScreenChange("crew1");
CrashLogger::getInstance().logPlayerAction("accessed weapons");

// Log soft crashes explicitly
CrashLogger::getInstance().logSoftCrash("Network timeout", "Connection lost");
CrashLogger::getInstance().logUnexpectedMenuReturn("crew1", "Lua error");
CrashLogger::getInstance().logGameStateReset("Resource failure", "Connected");

// Check soft crash state
if (CrashLogger::getInstance().isInSoftCrashState()) {
    std::string reason = CrashLogger::getInstance().getLastSoftCrashReason();
    // Handle soft crash state
}
```

### Lua Integration
Include `scripts/softCrashDetection.lua` in your scenario to enable soft crash detection:

```lua
-- Detect unexpected menu returns
onScreenChange("main")  -- When returning to main menu

-- Log errors with cascade detection
logErrorAndCheckCascade("Network", "Connection lost")
logErrorAndCheckCascade("Lua", "Script execution failed")

-- Log game state changes
logGameStateReset("Network timeout", "Connected")
logErrorRecovery("Lua error", "Restarting script")
```

## Automatic Detection

The system automatically detects soft crashes by monitoring:

1. **Rapid Screen Changes**: Multiple screen changes within 5 seconds
2. **Error Cascades**: 3+ errors within 10 seconds
3. **Unexpected Menu Returns**: Returning to main menu from game screens
4. **Lua Execution Issues**: Timeouts, infinite loops, recursion problems

## Configuration

### Build Options
- `ENABLE_CRASH_LOGGER`: Enables the crash logging system (default: ON)
- `ENABLE_NETWORK_MONITORING`: Enables network health monitoring (default: ON)

### Log File Management
- **Automatic Cleanup**: Old log files are automatically cleaned up
- **Size Limits**: Log files are truncated to prevent excessive growth
- **Directory**: All logs are stored in the `logs/` directory

## Troubleshooting

### Common Issues

1. **Generic "Windows Exception" messages**: The enhanced exception handler may not be working. Check that the crash logger is properly initialized in `main()`.

2. **Missing log files**: Ensure the `logs/` directory exists and has write permissions.

3. **Lua errors not detected**: Make sure to include the soft crash detection script and override the global `error` function.

4. **Soft crashes not logged**: Verify that game events are being logged using the provided methods.

### Testing

To test the crash logging system:

1. **Force a hard crash**: Use the instability monitor or other broken features
2. **Force a soft crash**: Create Lua errors that return to main menu
3. **Test network issues**: Disconnect network during multiplayer

## Integration Points

The system integrates with:
- **Main Game Loop**: Initialized early in `main()`
- **Screen Management**: Monitors screen changes for unexpected returns
- **Lua Engine**: Catches and logs Lua errors and execution issues
- **Network Layer**: Monitors connection health and stability
- **Error Recovery**: Tracks error recovery mechanisms and state resets

## Performance Impact

- **Minimal overhead**: Lightweight logging with configurable verbosity
- **Asynchronous**: Logging operations don't block gameplay
- **Smart filtering**: Only logs relevant events and warnings
- **Automatic cleanup**: Prevents log file bloat
