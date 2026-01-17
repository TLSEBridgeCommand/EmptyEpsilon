#include "networkHealthMonitor.h"
#include "crashLogger.h"
#include <sstream>
#include <algorithm>
#include <iomanip>

NetworkHealthMonitor* NetworkHealthMonitor::instance = nullptr;

NetworkHealthMonitor::NetworkHealthMonitor()
{
    // Initialize with clean state
    resetStatistics();
}

NetworkHealthMonitor* NetworkHealthMonitor::getInstance()
{
    if (!instance) {
        instance = new NetworkHealthMonitor();
    }
    return instance;
}

void NetworkHealthMonitor::recordSuccessfulOperation(const std::string& operationType)
{
    NetworkOperation op;
    op.timestamp = std::chrono::system_clock::now();
    op.type = operationType;
    op.success = true;
    op.errorReason = "";
    op.latencyMs = 0.0f;
    
    recentOperations.push_back(op);
    totalOperations++;
    
    cleanupOldData();
    
    // Log to crash logger for context
    CrashLogger::getInstance()->logNetworkEvent("successful_operation", operationType);
}

void NetworkHealthMonitor::recordFailedOperation(const std::string& operationType, const std::string& errorReason)
{
    NetworkOperation op;
    op.timestamp = std::chrono::system_clock::now();
    op.type = operationType;
    op.success = false;
    op.errorReason = errorReason;
    op.latencyMs = 0.0f;
    
    recentOperations.push_back(op);
    totalOperations++;
    failedOperations++;
    
    cleanupOldData();
    
    // Log to crash logger for context
    CrashLogger::getInstance()->logNetworkEvent("failed_operation", operationType + " - " + errorReason);
    
    // Check if this indicates potential connection instability
    if (isConnectionUnstable()) {
        CrashLogger::getInstance()->logCrashIndicator("connection_unstable", 
            "High failure rate detected: " + std::to_string(calculateSuccessRate() * 100) + "% success rate");
    }
}

void NetworkHealthMonitor::recordLatency(const std::string& operationType, float latencyMs)
{
    // Find the most recent operation of this type and update its latency
    for (auto it = recentOperations.rbegin(); it != recentOperations.rend(); ++it) {
        if (it->type == operationType && it->success) {
            it->latencyMs = latencyMs;
            break;
        }
    }
}

void NetworkHealthMonitor::recordPacketLoss(const std::string& operationType)
{
    packetLossCount++;
    
    // Log to crash logger
    CrashLogger::getInstance()->logNetworkEvent("packet_loss", operationType);
    
    // Check if packet loss is becoming excessive
    if (packetLossCount > 50) { // Arbitrary threshold
        CrashLogger::getInstance()->logCrashIndicator("excessive_packet_loss", 
            "Packet loss count: " + std::to_string(packetLossCount));
    }
}

void NetworkHealthMonitor::recordConnectionStateChange(const std::string& oldState, const std::string& newState)
{
    ConnectionState state;
    state.timestamp = std::chrono::system_clock::now();
    state.oldState = oldState;
    state.newState = newState;
    
    connectionStates.push_back(state);
    
    // Keep only recent connection states
    if (connectionStates.size() > MAX_CONNECTION_STATES) {
        connectionStates.pop_front();
    }
    
    // Log to crash logger
    CrashLogger::getInstance()->logConnectionStateChange(oldState, newState, "local", "");
    
    // Check for suspicious state transition patterns
    std::string pattern = getStateTransitionPattern();
    if (pattern.find("Disconnected->Connecting->Disconnected") != std::string::npos ||
        pattern.find("Connected->Disconnected->Connected") != std::string::npos) {
        CrashLogger::getInstance()->logCrashIndicator("unstable_connection_pattern", pattern);
    }
}

int NetworkHealthMonitor::getConnectionHealthScore()
{
    return calculateHealthScore();
}

std::string NetworkHealthMonitor::getConnectionStats()
{
    std::stringstream ss;
    
    float successRate = calculateSuccessRate();
    float avgLatency = calculateAverageLatency();
    int healthScore = calculateHealthScore();
    
    ss << "Connection Statistics:\n";
    ss << "  Total Operations: " << totalOperations << "\n";
    ss << "  Failed Operations: " << failedOperations << "\n";
    ss << "  Success Rate: " << std::fixed << std::setprecision(1) << (successRate * 100) << "%\n";
    ss << "  Average Latency: " << std::fixed << std::setprecision(1) << avgLatency << "ms\n";
    ss << "  Packet Loss Count: " << packetLossCount << "\n";
    ss << "  Health Score: " << healthScore << "/100\n";
    ss << "  Connection States: " << connectionStates.size() << " recent changes\n";
    
    if (!connectionStates.empty()) {
        ss << "  Recent State Changes:\n";
        auto recentStates = connectionStates;
        if (recentStates.size() > 5) {
            recentStates.erase(recentStates.begin(), recentStates.begin() + (recentStates.size() - 5));
        }
        
        for (const auto& state : recentStates) {
            auto time_t = std::chrono::system_clock::to_time_t(state.timestamp);
            ss << "    " << std::put_time(std::localtime(&time_t), "%H:%M:%S") 
               << " " << state.oldState << " -> " << state.newState << "\n";
        }
    }
    
    return ss.str();
}

bool NetworkHealthMonitor::isConnectionUnstable()
{
    if (totalOperations < 10) {
        return false; // Need more data to make a determination
    }
    
    float successRate = calculateSuccessRate();
    return successRate < (1.0f - UNSTABLE_THRESHOLD);
}

std::string NetworkHealthMonitor::getConnectionRecommendations()
{
    std::stringstream ss;
    
    if (isConnectionUnstable()) {
        ss << "Connection appears unstable. Recommendations:\n";
        ss << "  1. Check network cable/wireless connection\n";
        ss << "  2. Restart router/modem\n";
        ss << "  3. Check for network congestion\n";
        ss << "  4. Verify server is accessible\n";
        ss << "  5. Consider using wired connection instead of wireless\n";
    }
    
    if (packetLossCount > 20) {
        ss << "High packet loss detected. This may indicate:\n";
        ss << "  1. Network congestion\n";
        ss << "  2. Poor wireless signal quality\n";
        ss << "  3. Router issues\n";
        ss << "  4. ISP problems\n";
    }
    
    if (connectionStates.size() > 10) {
        ss << "Frequent connection state changes detected. This may indicate:\n";
        ss << "  1. Network instability\n";
        ss << "  2. Server issues\n";
        ss << "  3. Firewall/antivirus interference\n";
    }
    
    if (ss.str().empty()) {
        ss << "Connection appears stable. No immediate action required.";
    }
    
    return ss.str();
}

void NetworkHealthMonitor::resetStatistics()
{
    recentOperations.clear();
    connectionStates.clear();
    totalOperations = 0;
    failedOperations = 0;
    packetLossCount = 0;
}

void NetworkHealthMonitor::cleanupOldData()
{
    // Remove old operations beyond our history limit
    while (recentOperations.size() > MAX_OPERATIONS_HISTORY) {
        recentOperations.pop_front();
    }
    
    // Remove old connection states beyond our limit
    while (connectionStates.size() > MAX_CONNECTION_STATES) {
        connectionStates.pop_front();
    }
}

float NetworkHealthMonitor::calculateSuccessRate()
{
    if (totalOperations == 0) {
        return 1.0f;
    }
    
    return static_cast<float>(totalOperations - failedOperations) / static_cast<float>(totalOperations);
}

float NetworkHealthMonitor::calculateAverageLatency()
{
    if (recentOperations.empty()) {
        return 0.0f;
    }
    
    float totalLatency = 0.0f;
    int latencyCount = 0;
    
    for (const auto& op : recentOperations) {
        if (op.success && op.latencyMs > 0.0f) {
            totalLatency += op.latencyMs;
            latencyCount++;
        }
    }
    
    if (latencyCount == 0) {
        return 0.0f;
    }
    
    return totalLatency / static_cast<float>(latencyCount);
}

int NetworkHealthMonitor::calculateHealthScore()
{
    if (totalOperations == 0) {
        return 100; // No operations yet, assume healthy
    }
    
    float successRate = calculateSuccessRate();
    float latencyScore = 1.0f;
    
    // Factor in latency if we have data
    float avgLatency = calculateAverageLatency();
    if (avgLatency > 0.0f) {
        // Penalize high latency (100ms+ gets penalty)
        if (avgLatency > 100.0f) {
            latencyScore = 100.0f / avgLatency;
        }
    }
    
    // Factor in packet loss
    float packetLossPenalty = 0.0f;
    if (packetLossCount > 0) {
        packetLossPenalty = std::min(0.3f, static_cast<float>(packetLossCount) * 0.01f);
    }
    
    // Calculate final score
    float score = (successRate * 0.6f + latencyScore * 0.3f) * (1.0f - packetLossPenalty);
    
    return static_cast<int>(std::max(0.0f, std::min(100.0f, score * 100.0f)));
}

std::string NetworkHealthMonitor::getStateTransitionPattern()
{
    if (connectionStates.size() < 2) {
        return "Insufficient data";
    }
    
    std::stringstream ss;
    auto recentStates = connectionStates;
    if (recentStates.size() > 10) {
        recentStates.erase(recentStates.begin(), recentStates.begin() + (recentStates.size() - 10));
    }
    
    for (size_t i = 0; i < recentStates.size(); ++i) {
        if (i > 0) ss << "->";
        ss << recentStates[i].newState;
    }
    
    return ss.str();
}
