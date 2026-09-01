#include "scriptErrorLogging.h"
#include "crashLogger.h"
#include "scriptInterface.h"

static void onLuaError(const string& context, const string& error, const string& stackTrace)
{
    CrashLogger::getInstance()->logLuaError(context + ": " + error, stackTrace);
}

void registerScriptErrorLogging()
{
    ScriptObject::setLuaErrorHandler(onLuaError);
}
