// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "owlanzi/config.hpp"
#include <memory>
#include <string>
#include <vector>
namespace owlanzi {
struct WifiNetwork {std::string ssid;int rssi=-100;bool secure=true,supported=true;};
struct WifiLink {bool supported=false,connected=false,hotspot=false;std::string ssid,ip;};
class WifiDriver {
public:
 virtual ~WifiDriver()=default;
 virtual void enable()=0;
 // Enabling the radio alone need not resume a saved station connection.
 virtual void reconnect()=0;
 virtual bool hasSavedNetwork()=0;
 virtual WifiLink inspect()=0;
 virtual std::vector<WifiNetwork> scan()=0;
 virtual void startHotspot()=0;
 virtual void stopHotspot()=0;
 virtual void connect(const std::string& ssid,const std::string& password)=0;
 virtual void commit()=0;
 virtual void rollback()=0;
};
struct WifiTiming {int bootMs=20000,joinMs=45000,pollMs=200,responseMs=700,reconnectAttempts=2,recoveryHotspotMs=300000;};
class WifiService {
public:
 explicit WifiService(std::unique_ptr<WifiDriver> driver,WifiTiming timing={});
 ~WifiService();
 void start();void stop();
 Json status() const;
 void request(const std::string& action,const Json& input=Json::object());
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
std::shared_ptr<WifiService> makeDemoWifi();
void validateWifiConnect(const Json& input);
bool hasSavedWifiNetwork(const std::string& config);
}
