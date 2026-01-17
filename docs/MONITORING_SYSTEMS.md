# Monitoring Systems Overview

This document explains the three monitoring systems in EmptyEpsilon and their performance characteristics.

## System Overview

There are **three separate but interconnected monitoring systems**:

1. **CrashLogger** - Handles crash detection and logging
2. **NetworkHealthMonitor** - Tracks network connection health metrics
3. **ConnectionMonitor** - Integrates the above two with the game's connection system

## 1. CrashLogger (`crashLogger.h/cpp`)

### Purpose
- **Primary Role**: Captures crash context when the game crashes or encounters errors
- **Focus**: Post-mortem analysis and debugging

### What It Does
- Sets up exception handlers (Windows SEH, Unix signals)
- Captures stack traces with function names and line numbers
- Logs game events, screen changes, and player actions for context
- Detects "soft crashes" (unexpected returns to main menu)
- Writes crash reports to `logs/crash_log.txt`

### When It Runs
- **On crashes**: Exception handlers trigger automatically
- **On events**: When explicitly called (e.g., `logGameEvent()`, `logScreenChange()`)
- **Periodic**: Soft crash pattern detection runs after logging events

### Performance Impact
- **Low**: Most operations are event-driven (only runs when something happens)
- **Crash handlers**: Zero overhead until a crash occurs
- **Event logging**: Minimal - just stores strings in memory vectors
- **File I/O**: Only during crashes or when explicitly saving context

### Memory Usage
- Stores recent events in vectors (limited to 100 network events, 50 game events)
- Automatic cleanup prevents memory bloat

---

## 2. NetworkHealthMonitor (`networkHealthMonitor.h/cpp`)

### Purpose
- **Primary Role**: Tracks network connection quality metrics in real-time
- **Focus**: Active monitoring and health scoring

### What It Does
- Records successful/failed network operations
- Tracks latency measurements
- Counts packet loss events
- Calculates connection health score (0-100)
- Detects connection instability patterns
- Provides recommendations for connection issues

### When It Runs
- **On network operations**: When `recordSuccessfulOperation()`, `recordFailedOperation()`, etc. are called
- **On demand**: When `getConnectionHealthScore()` or `isConnectionUnstable()` is queried
- **Periodic cleanup**: Removes old data beyond history limits

### Performance Impact
- **Low to Medium**: 
  - Operations are O(1) - just pushing to deque and incrementing counters
  - Health score calculation: O(n) where n = recent operations (max 1000)
  - Cleanup: O(1) - just popping from front of deque
- **Memory bounded**: Limited to 1000 operations and 100 connection states

### Memory Usage
- Fixed maximum: 1000 operations + 100 connection states
- Uses `std::deque` for efficient front/back operations
- Atomic counters for thread-safe statistics

---

## 3. ConnectionMonitor (`connectionMonitor.h/cpp`)

### Purpose
- **Primary Role**: Integration layer that connects the game's connection system to the monitoring systems
- **Focus**: Real-time connection status monitoring

### What It Does
- Monitors `game_client->getStatus()` for state changes
- Calls `NetworkHealthMonitor` and `CrashLogger` when events occur
- Simulates ping operations every 5 seconds
- Provides diagnostic information
- Logs connection state changes to all systems

### When It Runs
- **On status changes**: Only when connection status actually changes (efficient)
- **Periodic health checks**: Every 60 seconds (not every frame)
- **Ping simulation**: Every 5 seconds

### Performance Impact
- **Very Low**:
  - Status check: Just reading an integer (`game_client->getStatus()`)
  - Only processes when status changes (not every frame)
  - Health checks run once per minute
  - Ping simulation runs every 5 seconds

### Memory Usage
- Minimal - just static variables for timing

---

## How They Work Together

```
Game Connection System
        ↓
ConnectionMonitor (watches for changes)
        ↓
    ┌───┴───┐
    ↓       ↓
NetworkHealthMonitor  CrashLogger
(metrics & health)    (crash context)
```

### Example Flow:
1. Connection status changes (e.g., Connected → Disconnected)
2. `ConnectionMonitor::monitorConnectionStatus()` detects the change
3. Calls `NetworkHealthMonitor::recordConnectionStateChange()`
4. Calls `CrashLogger::logConnectionStateChange()`
5. If unexpected, calls `CrashLogger::logCrashIndicator()`

---

## Performance Analysis

### Overall Impact: **LOW**

#### Why It's Low:
1. **Event-driven**: Most operations only run when something happens, not every frame
2. **Throttled**: Health checks run every 60 seconds, pings every 5 seconds
3. **Bounded memory**: All systems have fixed maximums for stored data
4. **Efficient data structures**: Uses deques, vectors with automatic cleanup
5. **No blocking I/O**: File writes only during crashes or explicit saves

#### Potential Concerns:
1. **Health score calculation**: O(n) where n can be up to 1000 operations
   - **Mitigation**: Only called every 60 seconds, not every frame
   - **Impact**: Negligible (< 1ms even with 1000 operations)

2. **String operations**: Some string concatenation and formatting
   - **Mitigation**: Only happens on events (infrequent)
   - **Impact**: Minimal

3. **Multiple singleton lookups**: `getInstance()` called multiple times
   - **Mitigation**: Singleton pattern ensures only one instance exists
   - **Impact**: Just a pointer check, negligible

### Optimization Opportunities (if needed):

1. **Cache singleton instances**: Store pointers instead of calling `getInstance()` repeatedly
2. **Reduce history size**: Lower `MAX_OPERATIONS_HISTORY` from 1000 to 500 if memory is a concern
3. **Lazy health calculation**: Only calculate health score when queried, cache result
4. **Disable in release builds**: Add compile-time flags to disable detailed monitoring

---

## Recommendations

### Current State: **Good for Production**
- Performance impact is minimal
- Memory usage is bounded and reasonable
- Provides valuable debugging information

### If Performance Issues Arise:
1. **Profile first**: Use a profiler to confirm these systems are the bottleneck
2. **Increase throttling**: Make health checks run every 2-3 minutes instead of 1 minute
3. **Reduce history**: Lower operation history from 1000 to 500
4. **Conditional compilation**: Add `#ifdef ENABLE_DETAILED_MONITORING` guards

### Best Practices:
- Keep all three systems enabled for debugging
- They provide valuable crash context and network diagnostics
- The performance cost is minimal compared to the debugging value

---

## Summary Table

| System | Primary Purpose | Update Frequency | Performance Impact | Memory Usage |
|--------|----------------|------------------|---------------------|--------------|
| **CrashLogger** | Crash detection & logging | Event-driven | Very Low | ~100-150 events |
| **NetworkHealthMonitor** | Network health metrics | Event-driven + periodic | Low | ~1000 ops + 100 states |
| **ConnectionMonitor** | Integration layer | Status changes + 5s/60s | Very Low | Minimal (static vars) |

**Total Impact**: All three systems combined have **negligible performance impact** in normal operation.
