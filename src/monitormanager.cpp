#include "monitormanager.h"
#include <QProcess>
#include <QDebug>
#include <QRegularExpression>

QList<MonitorInfo> MonitorManager::discoverMonitors() {
    QList<MonitorInfo> monitors;

    QProcess process;
    process.start("ddcutil", {"detect"});
    process.waitForFinished(10000);

    QString output = process.readAllStandardOutput();
    QString error  = process.readAllStandardError();
    // qDebug() << "DDC detect stdout:" << output;
    // qDebug() << "DDC detect stderr:" << error;

    // Parse ddcutil output format:
    // Display 1
    //    I2C bus:  /dev/i2c-4
    //    DRM_connector:           card1-HDMI-A-1
    //    EDID synopsis:
    //       Model:                ASUS VG247Q1A

    QStringList lines = output.split('\n');
    int numLines = lines.size();

    for (int i = 0; i < numLines; ++i) {
        // Each monitor block starts with "Display N"
        if (!lines[i].trimmed().startsWith("Display "))
            continue;

        QString modelName;
        QString devicePath;

        // Scan forward through the block for I2C bus and Model
        for (int j = i + 1; j < numLines; ++j) {
            QString trimmed = lines[j].trimmed();

            // A new "Display" line or empty line signals end of block
            if (trimmed.startsWith("Display ") && j != i)
                break;

            if (trimmed.startsWith("I2C bus:")) {
                devicePath = trimmed.mid(QString("I2C bus:").length()).trimmed();
            } else if (trimmed.startsWith("Model:")) {
                modelName = trimmed.mid(QString("Model:").length()).trimmed();
            }
        }

        if (!modelName.isEmpty() || !devicePath.isEmpty()) {
            MonitorInfo info;
            info.name       = modelName.isEmpty() ? QStringLiteral("Unknown Monitor") : modelName;
            info.devicePath = devicePath;
            info.currentBrightness = -1;
            info.currentContrast   = -1;
            monitors.append(info);
        }
    }

    return monitors;
}

int MonitorManager::getBrightness(const QString& devicePath) {
    QProcess process;
    process.start("ddcutil", {"--bus", devicePath.section('-', -1), "getvcp", "10"});
    process.waitForFinished(5000);

    QString output = process.readAllStandardOutput();
    qDebug() << "Brightness get output:" << output;

    // ddcutil output: "VCP code 0x10 (Brightness): current value = 50, max value = 100"
    QRegularExpression regex(R"(current value\s*=\s*(\d+))");
    QRegularExpressionMatch match = regex.match(output);
    if (match.hasMatch())
        return match.captured(1).toInt();
    return -1;
}

bool MonitorManager::setBrightness(const QString& devicePath, int level) {
    QProcess process;
    QString busNum = devicePath.section('-', -1);  // extract number from /dev/i2c-N
    qDebug() << "Setting brightness on bus" << busNum << "to" << level;
    process.start("ddcutil", {"--bus", busNum, "setvcp", "10", QString::number(level)});
    return process.waitForFinished(5000) && process.exitCode() == 0;
}

int MonitorManager::getContrast(const QString& devicePath) {
    QProcess process;
    process.start("ddcutil", {"--bus", devicePath.section('-', -1), "getvcp", "12"});
    process.waitForFinished(5000);

    QString output = process.readAllStandardOutput();
    qDebug() << "Contrast get output:" << output;

    QRegularExpression regex(R"(current value\s*=\s*(\d+))");
    QRegularExpressionMatch match = regex.match(output);
    if (match.hasMatch())
        return match.captured(1).toInt();
    return -1;
}

bool MonitorManager::setContrast(const QString& devicePath, int level) {
    QProcess process;
    QString busNum = devicePath.section('-', -1);
    qDebug() << "Setting contrast on bus" << busNum << "to" << level;
    process.start("ddcutil", {"--bus", busNum, "setvcp", "12", QString::number(level)});
    return process.waitForFinished(5000) && process.exitCode() == 0;
}
