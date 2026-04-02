#include "monitor_manager.h"

MonitorManager::MonitorManager() {
    discoverMonitors();
}

QList<MonitorInfo>* MonitorManager::getMonitorList() {
    return &m_monitors;
}

void MonitorManager::discoverMonitors() {
    m_monitors = DDCCommander::discoverMonitors();
    
    // Update current values for each discovered monitor
    for (auto& monitor : m_monitors) {
        int brightness = DDCCommander::getCurrentBrightness(monitor.devicePath);
        if (brightness < 0) brightness = 50;  // Default
        
        int contrast = DDCCommander::getCurrentContrast(monitor.devicePath);
        
        monitor.currentBrightness = brightness;
        monitor.currentContrast = contrast;
    }
}
