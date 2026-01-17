#ifndef CONNECTION_MONITOR_H
#define CONNECTION_MONITOR_H

#include <string>

// Real integration with the actual game connection system
// These functions monitor the global game_client and log connection state changes

// Function to monitor connection status changes and log them
void monitorConnectionStatus(const std::string& serverAddress);

// Convert status enum to string
std::string getStatusString(int status);

// Log network operations (success/failure, latency)
void logNetworkOperation(const std::string& operationType, bool success, 
                        const std::string& errorReason, float latencyMs);

// Log packet loss events
void logPacketLoss(const std::string& operationType);

// Get comprehensive connection diagnostics
std::string getConnectionDiagnostics();

// Reset connection monitoring statistics
void resetConnectionMonitoring();

// Update function to be called regularly for monitoring
void updateConnectionMonitoring(const std::string& serverAddress);

#endif // CONNECTION_MONITOR_H
