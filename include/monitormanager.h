#ifndef MONITORMANAGER_H
#define MONITORMANAGER_H

#include <QString>
#include <QList>
#include <ddcutil/ddc.h>

struct MonitorInfo {
    QString name;
    QString vendor;
    QString devicePath;
};

class MonitorManager {
public:
    static QList<MonitorInfo> discoverMonitors();
    
    int getBrightness(const QString& devicePath) const;
    bool setBrightness(const QString& devicePath, int level);
    
    int getContrast(const QString& devicePath) const;
    bool setContrast(const QString& devicePath, int level);

private:
    static QString getDevicePathFromName(const QString& monitorName);
};

#endif // MONITORMANAGER_H
