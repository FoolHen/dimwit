#ifndef MONITOR_CONTROLLER_H
#define MONITOR_CONTROLLER_H

#include <QString>
#include <QList>
#include <QPair>
#include "ddc_commander.h"

class MonitorController {
public:
    static QList<MonitorInfo> discoverMonitors();
    
    static int getBrightness(const QString& devicePath);
    static bool setBrightness(const QString& devicePath, int level);
    
    static int getContrast(const QString& devicePath);
    static bool setContrast(const QString& devicePath, int level);

};

#endif // MONITOR_CONTROLLER_H
