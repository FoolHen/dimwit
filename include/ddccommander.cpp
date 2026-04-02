#include "ddccommander.h"
#include <QProcess>
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDebug>

// Helper function to parse ddcutil output for brightness/contrast
static int extractValue(const QString& line, const QString& keyword) {
    int pos = line.indexOf(keyword);
    if (pos == -1) return -1;
    
    pos += keyword.length();
    while (pos < line.length() && (line[pos].isSpace() || line[pos] == ':')) {
        ++pos;
    }
    
    if (pos >= line.length()) return -1;
    
    int value = 0;
    bool ok = false;
    value = line.mid(pos, line.length() - pos).toInt(&ok);
    return ok ? value : -1;
}

// Get current brightness from ddcutil output
static QString getBrightnessLine(const QString& devicePath) {
    QProcess process;
    process.startDetached("ddcutil", {"getbrightness", devicePath});
    
    // Wait briefly for output (ddcutil outputs to terminal, not stdout)
    QThread::msleep(500);
    
    return QString();  // ddcutil writes to stderr/terminal, we'll use alternative method
}

// Alternative: parse EDID info using ddcutil getmonitorinfo
MonitorInfo DDCCommander::getMonitorInfo(const QString& devicePath) {
    MonitorInfo info;
    info.devicePath = devicePath;
    
    QString cmd = "ddcutil getmonitorinfo " + devicePath;
    QProcess process2;
    process2.start("bash", {"-c", cmd});
    process2.waitForStarted();
    
    // Read stderr (where ddcutil outputs)
    process2.readAllStandardError();
    process2.waitForFinished(5000);
    
    QString output = QString::fromUtf8(process2.readAllStandardOutput());
    if (!output.isEmpty()) {
        info.name = "Unknown Monitor";
        
        // Parse vendor/model from output (format varies by ddcutil version)
        int modelPos = output.indexOf("Model:");
        if (modelPos != -1) {
            QString modelName = output.mid(modelPos + 6).trimmed();
            info.name = modelName;
        } else {
            // Try to extract from EDID string
            int edidPos = output.indexOf("EDID:");
            if (edidPos != -1) {
                QString edidStr = output.mid(edidPos + 5).trimmed();
                info.name = "Monitor (" + edidStr.left(20) + ")";
            }
        }
    } else {
        // Try ddcutil list to get device paths and names
        QProcess process3;
        process3.start("ddcutil", {"list"});
        process3.waitForStarted();
        QString listOutput = QString::fromUtf8(process3.readAllStandardError());
        
        if (!listOutput.isEmpty()) {
            // Parse the list output to find our device
            QStringList lines = listOutput.split('\n');
            for (const QString& line : lines) {
                if (line.contains(devicePath)) {
                    info.name = "Monitor";
                    break;
                }
            }
        }
    }
    
    // Set defaults if we couldn't get real values
    info.currentBrightness = 50;
    info.currentContrast = -1;  // Assume unsupported until proven otherwise
    
    return info;
}

QList<MonitorInfo> DDCCommander::discoverMonitors() {
    QList<MonitorInfo> monitors;
    
    // First, get list of all DDC-capable devices using ddcutil list
    QProcess process;
    QString cmd = "ddcutil list";
    process.start("bash", {"-c", cmd});
    process.waitForStarted();
    
    QString output = QString::fromUtf8(process.readAllStandardError());
    if (output.isEmpty()) {
        qWarning() << "Failed to run ddcutil list - check permissions";
        return monitors;
    }
    
    // Parse device paths from output
    QStringList lines = output.split('\n');
    for (const QString& line : lines) {
        if (line.contains("DDC")) {
            // Extract device path (format: "Device: /dev/i2c-1")
            int devPos = line.indexOf("Device:");
            if (devPos != -1) {
                QString devicePath = line.mid(devPos + 7).trimmed();
                
                MonitorInfo info = getMonitorInfo(devicePath);
                monitors.append(info);
            }
        }
    }
    
    return monitors;
}

bool DDCCommander::setBrightness(const QString& devicePath, int level) {
    if (level < 0 || level > 100) {
        qWarning() << "Invalid brightness level:" << level;
        return false;
    }
    
    QProcess process;
    // ddcutil setbrightness takes value 0-255, we map 0-100 to that range
    int vcpValue = (level * 255) / 100;
    
    QString cmd = QString("ddcutil setbrightness %1 %2").arg(devicePath).arg(vcpValue);
    process.start("bash", {"-c", cmd});
    process.waitForFinished(3000);
    
    if (process.exitCode() == 0) {
        return true;
    } else {
        qWarning() << "Failed to set brightness:" << process.readAllStandardError();
        return false;
    }
}

bool DDCCommander::setContrast(const QString& devicePath, int level) {
    if (level < 0 || level > 100) {
        qWarning() << "Invalid contrast level:" << level;
        return false;
    }
    
    QProcess process;
    // Contrast uses vendor-specific VCP code, typically 47 or similar
    int vcpValue = (level * 255) / 100;
    
    QString cmd = QString("ddcutil setcontrast %1 %2").arg(devicePath).arg(vcpValue);
    process.start("bash", {"-c", cmd});
    process.waitForFinished(3000);
    
    if (process.exitCode() == 0) {
        return true;
    } else {
        qWarning() << "Failed to set contrast:" << process.readAllStandardError();
        return false;
    }
}

int DDCCommander::getCurrentBrightness(const QString& devicePath) {
    QProcess process;
    QString cmd = QString("ddcutil getbrightness %1").arg(devicePath);
    
    // ddcutil outputs to stderr, so we need to capture it differently
    // Use a temporary file or read from terminal output
    
    // Alternative: use ddcutil with -q and parse output
    process.startDetached(cmd);
    QThread::msleep(500);  // Wait for command to complete
    
    return 50;  // Default value (would need better parsing)
}

int DDCCommander::getCurrentContrast(const QString& devicePath) {
    // Most monitors don't support contrast via DDC, return -1 by default
    return -1;
}
