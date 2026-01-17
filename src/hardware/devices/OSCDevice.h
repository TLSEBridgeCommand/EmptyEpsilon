#ifndef OSC_DEVICE_H
#define OSC_DEVICE_H

#include <SFML/System.hpp>
#include <SFML/Network.hpp>
#include "hardware/hardwareOutputDevice.h"

#include <stdint.h>
#include <thread>

class OSCDevice : public HardwareOutputDevice
{
private:
    static constexpr int maximum_udp_packet_size = 65507;

    sf::UdpSocket socket;
    
    int channel_count;
    sf::IpAddress address;
    int port_number;
    string osc_addresses[512];
    float previous_values[512];
    bool send_data_only_if_changed;
    bool primary_ship;
    
public:
    OSCDevice();
    virtual ~OSCDevice() = default;
    
    virtual bool configure(std::unordered_map<string, string> settings) override;

    virtual void setChannelData(int channel, float value) override;

    virtual int getChannelCount() override;

};

#endif//OSC_DEVICE_H
