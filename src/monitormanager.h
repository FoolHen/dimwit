#ifndef MONITORMANAGER_H
#define MONITORMANAGER_H

#include <QString>
#include <QList>
#include <QPair>
#include "ddccommander.h"

class MonitorManager {
public:
    static QList<MonitorInfo> discoverMonitors();
    
    static int getBrightness(const QString& devicePath);
    static bool setBrightness(const QString& devicePath, int level);
    
    static int getContrast(const QString& devicePath);
    static bool setContrast(const QString& devicePath, int level);

};

#endif // MONITORMANAGER_H
