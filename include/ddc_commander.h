#ifndef DDC_COMMANDER_H
#define DDC_COMMANDER_H

#include <QString>
#include <QList>
#include <QPair>

struct MonitorInfo {
    QString name;           // Vendor + Model (e.g., "Dell U2715")
    QString devicePath;     // /dev/i2c-* path or ddcutil identifier
    int currentBrightness;  // 0-100%
    int currentContrast;    // 0-100% or -1 if unsupported
};

class DDCCommander {
public:
    static QList<MonitorInfo> discoverMonitors();
    
    static bool setBrightness(const QString& devicePath, int level);  // 0-100%
    static bool setContrast(const QString& devicePath, int level);    // 0-100%
    
    static int getCurrentBrightness(const QString& devicePath);       // 0-100% or -1 if error
    static int getCurrentContrast(const QString& devicePath);         // 0-100% or -1 if unsupported

private:
    static MonitorInfo getMonitorInfo(const QString& devicePath);
};

#endif // DDC_COMMANDER_H
