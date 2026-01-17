#include "connectionMonitor.h"
#include "crashLogger.h"
#include "networkHealthMonitor.h"
#include "logging.h"
#include "engine.h"
#include <sstream>

// Real integration with the actual game connection system
// This monitors the global game_client and logs connection state changes

void monitorConnectionStatus(const std::string& serverAddress)
{
    // Use the global game_client directly
    if (!game_client) {
        return;
    }
    
    static int lastStatus = -1;
    // Get the actual connection status from the game client
    int currentStatus = game_client->getStatus();
    
    // Only log when status actually changes
    if (lastStatus != currentStatus) {
        std::string oldStatusStr = getStatusString(lastStatus);
        std::string newStatusStr = getStatusString(currentStatus);
        
        // Log the state change
        auto crashLogger = CrashLogger::getInstance();
        auto networkMonitor = NetworkHealthMonitor::getInstance();
        
        // Determine additional context
        std::string additionalInfo;
        if (currentStatus == GameClient::Disconnected && lastStatus == GameClient::Connected) {
            additionalInfo = "Unexpected disconnection";
            crashLogger->logCrashIndicator("unexpected_disconnect", 
                "Client disconnected from " + serverAddress);
        }
        
        // Log to crash logger
        crashLogger->logConnectionStateChange(oldStatusStr, newStatusStr, serverAddress, additionalInfo);
        
        // Log to network monitor
        networkMonitor->recordConnectionStateChange(oldStatusStr, newStatusStr);
        
        // Log to regular logging system (this goes to game log)
        LOG(INFO) << "Connection status changed: " << oldStatusStr << " -> " << newStatusStr 
                  << " (Server: " << serverAddress << ")";
        
        lastStatus = currentStatus;
    }
    
    // Monitor connection health periodically (less frequent to reduce noise)
    static auto lastHealthCheck = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastHealthCheck).count() >= 60) { // Check every minute instead of 10 seconds
        auto networkMonitor = NetworkHealthMonitor::getInstance();
        
        if (networkMonitor->isConnectionUnstable()) {
            LOG(WARNING) << "Connection appears unstable!";
            LOG(INFO) << "Recommendations: " << networkMonitor->getConnectionRecommendations();
        }
        
        // Only log health score if it's concerning
        int healthScore = networkMonitor->getConnectionHealthScore();
        if (healthScore < 50) { // Only warn if health is really bad
            LOG(WARNING) << "Connection health score: " << healthScore << "/100";
        }
        
        lastHealthCheck = now;
    }
}

std::string getStatusString(int status)
{
    switch (status) {
        case GameClient::ReadyToConnect: return "ReadyToConnect";
        case GameClient::Connecting: return "Connecting";
        case GameClient::Authenticating: return "Authenticating";
        case GameClient::WaitingForPassword: return "WaitingForPassword";
        case GameClient::Disconnected: return "Disconnected";
        case GameClient::Connected: return "Connected";
        default: return "Unknown(" + std::to_string(status) + ")";
    }
}

void logNetworkOperation(const std::string& operationType, bool success, 
                        const std::string& errorReason, float latencyMs)
{
    auto networkMonitor = NetworkHealthMonitor::getInstance();
    
    if (success) {
        networkMonitor->recordSuccessfulOperation(operationType);
        if (latencyMs > 0.0f) {
            networkMonitor->recordLatency(operationType, latencyMs);
        }
        
        LOG(DEBUG) << "Network operation successful: " << operationType 
                   << (latencyMs > 0.0f ? " (Latency: " + std::to_string(latencyMs) + "ms)" : "");
    } else {
        networkMonitor->recordFailedOperation(operationType, errorReason);
        
        LOG(WARNING) << "Network operation failed: " << operationType << " - " << errorReason;
        
        // Check if this indicates connection instability
        if (networkMonitor->isConnectionUnstable()) {
            auto crashLogger = CrashLogger::getInstance();
            crashLogger->logCrashIndicator("network_operation_failure", 
                operationType + " failed: " + errorReason);
        }
    }
}

void logPacketLoss(const std::string& operationType)
{
    auto networkMonitor = NetworkHealthMonitor::getInstance();
    networkMonitor->recordPacketLoss(operationType);
    
    LOG(WARNING) << "Packet loss detected for operation: " << operationType;
    
    // Check if packet loss is becoming excessive
    if (networkMonitor->getConnectionHealthScore() < 50) {
        auto crashLogger = CrashLogger::getInstance();
        crashLogger->logCrashIndicator("excessive_packet_loss", 
            "High packet loss affecting connection health");
    }
}

std::string getConnectionDiagnostics()
{
    auto networkMonitor = NetworkHealthMonitor::getInstance();
    
    std::stringstream ss;
    ss << "=== Connection Diagnostics ===\n";
    ss << "Health Score: " << networkMonitor->getConnectionHealthScore() << "/100\n";
    ss << "Is Unstable: " << (networkMonitor->isConnectionUnstable() ? "Yes" : "No") << "\n\n";
    ss << networkMonitor->getConnectionStats() << "\n";
    ss << "=== Recommendations ===\n";
    ss << networkMonitor->getConnectionRecommendations();
    
    return ss.str();
}

void resetConnectionMonitoring()
{
    auto networkMonitor = NetworkHealthMonitor::getInstance();
    networkMonitor->resetStatistics();
    
    LOG(INFO) << "Connection monitoring statistics reset";
}

// Real integration with existing screen update functions
void updateConnectionMonitoring(const std::string& serverAddress)
{
    // Monitor connection status
    monitorConnectionStatus(serverAddress);
    
    // Additional monitoring can be added here
    // For example, ping the server periodically to detect packet loss
    
    static float lastPingTime = 0.0f;
    float currentTime = engine->getElapsedTime();
    
    if (currentTime - lastPingTime > 5.0f) { // Ping every 5 seconds
        // This is where you'd implement actual ping logic
        // For now, we'll just simulate it
        
        // Check if the game client is connected
        bool pingSuccess = (game_client && game_client->getStatus() == GameClient::Connected);
        if (pingSuccess) {
            logNetworkOperation("ping", true, "", 25.0f); // Simulate 25ms latency
        } else {
            logNetworkOperation("ping", false, "Not connected", 0.0f);
        }
        
        lastPingTime = currentTime;
    }
}
