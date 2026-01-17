# Debugging Access Violations and Crashes

## Overview
When EmptyEpsilon crashes with an Access Violation (like at address `6A223DF4`), you need to find the exact location in your code where the crash occurred. This guide explains multiple methods to debug crashes.

## Method 1: Enhanced Crash Logger (Recommended)

The crash logger has been enhanced to automatically capture stack traces with function names and line numbers. After a crash:

1. **Check the crash log file**: `logs/crash_log.txt`
   - This file now contains a full stack trace showing exactly where the crash occurred
   - Look for the "STACK TRACE" section which shows the call stack

2. **Example crash log entry**:
```
=== CRASH REPORT ===
Timestamp: 2025-01-XX XX:XX:XX.XXX
Crash Reason: Windows Exception: Access Violation (null pointer, invalid memory access) at address 0x6A223DF4
System Info: Windows, Processors: X, Memory: XGB
Connection State: ...

=== STACK TRACE ===
  [0] SomeFunction (src\someFile.cpp:123) [0x6A223DF4]
  [1] AnotherFunction (src\anotherFile.cpp:456) [0x6A223DF5]
  [2] main (src\main.cpp:789) [0x6A223DF6]
```

3. **What to look for**:
   - The top frame `[0]` shows where the crash occurred
   - The frames below show the call chain that led to the crash
   - File names and line numbers point you to the exact code location

## Method 2: Using Visual Studio Debugger

If you're building with Visual Studio:

1. **Build in Debug mode** (not Release)
   - Debug builds include symbol information needed for debugging

2. **Run under the debugger**:
   - Press F5 or go to Debug → Start Debugging
   - When the crash occurs, Visual Studio will break at the exact line

3. **View the call stack**:
   - Open Debug → Windows → Call Stack
   - This shows the full call chain leading to the crash

4. **Inspect variables**:
   - Hover over variables to see their values
   - Check if pointers are null or invalid

## Method 3: Using WinDbg (Advanced)

For more detailed analysis:

1. **Install WinDbg** (part of Windows SDK)

2. **Generate a crash dump**:
   - When the crash dialog appears, click "Debug"
   - Or configure Windows Error Reporting to create dumps

3. **Load the dump in WinDbg**:
   ```
   windbg -z crash.dmp
   ```

4. **Analyze the crash**:
   ```
   !analyze -v
   k          (shows call stack)
   !sym noisy (enables verbose symbol loading)
   ```

## Method 4: Using MinGW Debugger (GDB)

If building with MinGW:

1. **Build with debug symbols**:
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   make
   ```

2. **Run with GDB**:
   ```bash
   gdb EmptyEpsilon.exe
   run
   ```

3. **When it crashes**:
   ```
   bt          (backtrace - shows call stack)
   info locals  (shows local variables)
   print variable_name
   ```

## Method 5: Address Space Layout Randomization (ASLR)

The address `6A223DF4` is a memory address that changes each run due to ASLR. To get consistent addresses:

1. **Disable ASLR** (for debugging only):
   - Use `editbin /DYNAMICBASE:NO EmptyEpsilon.exe` (requires Visual Studio tools)
   - Or disable in linker settings: `/DYNAMICBASE:NO`

2. **Note**: This is only for debugging. Don't ship builds with ASLR disabled.

## Common Causes of Access Violations

1. **Null pointer dereference**:
   ```cpp
   SomeObject* obj = nullptr;
   obj->doSomething(); // CRASH!
   ```

2. **Dangling pointer**:
   ```cpp
   SomeObject* obj = new SomeObject();
   delete obj;
   obj->doSomething(); // CRASH! obj points to freed memory
   ```

3. **Array out of bounds**:
   ```cpp
   int arr[10];
   arr[20] = 5; // CRASH! Writing past array end
   ```

4. **Uninitialized pointer**:
   ```cpp
   SomeObject* obj; // Uninitialized
   obj->doSomething(); // CRASH! Random memory address
   ```

## Tips for Finding the Bug

1. **Check the crash log first** - it now has stack traces
2. **Look for recent code changes** - crashes often come from recent edits
3. **Check for null checks** - ensure pointers are validated before use
4. **Use static analysis tools** - tools like PVS-Studio or Clang Static Analyzer can find potential issues
5. **Enable more logging** - add LOG statements around suspicious code
6. **Use AddressSanitizer** (if available) - detects memory errors at runtime

## Building with Debug Symbols

To get useful stack traces, always build with debug symbols:

**CMake Debug Build**:
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

**Visual Studio**:
- Set configuration to "Debug" (not "Release")
- Ensure "Generate Debug Information" is enabled in project settings

## Next Steps After Finding the Crash Location

1. **Read the code** at the crash location
2. **Check for null/invalid pointers**
3. **Verify array bounds**
4. **Check object lifetimes** (is it still valid?)
5. **Add defensive checks** (null checks, bounds checks)
6. **Test the fix** thoroughly

## Getting Help

If you can't resolve the crash:
1. Share the stack trace from `logs/crash_log.txt`
2. Describe what you were doing when it crashed
3. Include recent code changes
4. Check if it's reproducible or random
