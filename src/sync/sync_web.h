#pragma once
// Web configuration UI served by the grinder (port 80):
//  * first setup: open access point "EurekaMate-Setup" + captive portal
//  * then in the home WiFi: http://eurekamate.local/
// Pages: status (live), recipes, regulation, dial/motor + calibration jog,
// WiFi & GaggiMate (scan, mDNS discovery, connection test), shot history, firmware update.
#include <Arduino.h>

class SyncWeb {
public:
    void init();
private:
    static void task_entry(void* arg);
    void task_loop();
};

extern SyncWeb sync_web;
