#ifndef NETWORK_HEALTH_MONITOR_H
#define NETWORK_HEALTH_MONITOR_H

#include <string>
#include <chrono>
#include <deque>
#include <atomic>
#include <memory>

class NetworkHealthMonitor
{
public:
    static NetworkHealthMonitor* getInstance();
    
    // Record a successful network operation
    void recordSuccessfulOperation(const std::string& operationType);
    
    // Record a failed network operation
    void recordFailedOperation(const std::string& operationType, const std::string& errorReason);
    
    // Record connection latency (if available)
    void recordLatency(const std::string& operationType, float latencyMs);
    
    // Record packet loss indicators
    void recordPacketLoss(const std::string& operationType);
    
    // Record connection state changes
    void recordConnectionStateChange(const std::string& oldState, const std::string& newState);
    
    // Get current connection health score (0-100, higher is better)
    int getConnectionHealthScore();
    
    // Get recent connection statistics
    std::string getConnectionStats();
    
    // Check if connection is showing signs of instability
    bool isConnectionUnstable();
    
    // Get recommendations for connection issues
    std::string getConnectionRecommendations();
    
    // Reset statistics (useful after reconnection)
    void resetStatistics();

private:
    NetworkHealthMonitor();
    ~NetworkHealthMonitor() = default;
    
    static NetworkHealthMonitor* instance;
    
    struct NetworkOperation {
        std::chrono::system_clock::time_point timestamp;
        std::string type;
        bool success;
        std::string errorReason;
        float latencyMs;
    };
    
    struct ConnectionState {
        std::chrono::system_clock::time_point timestamp;
        std::string oldState;
        std::string newState;
    };
    
    std::deque<NetworkOperation> recentOperations;
    std::deque<ConnectionState> connectionStates;
    
    std::atomic<int> totalOperations{0};
    std::atomic<int> failedOperations{0};
    std::atomic<int> packetLossCount{0};
    
    // Configuration
    static constexpr size_t MAX_OPERATIONS_HISTORY = 1000;
    static constexpr size_t MAX_CONNECTION_STATES = 100;
    static constexpr float UNSTABLE_THRESHOLD = 0.15f; // 15% failure rate
    
    void cleanupOldData();
    float calculateSuccessRate();
    float calculateAverageLatency();
    int calculateHealthScore();
    std::string getStateTransitionPattern();
};

#endif // NETWORK_HEALTH_MONITOR_H
