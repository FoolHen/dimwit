#include "brightness_scheduler.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QProcess>

BrightnessScheduler::BrightnessScheduler(QObject *parent) 
    : QObject(parent), m_autoMode(false) 
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &BrightnessScheduler::onTick);
    // Tick every 15 minutes to adjust brightness
    m_timer->start(15 * 60 * 1000); 
}

void BrightnessScheduler::setSchedule(const QMap<QTime, int>& schedule) {
    m_schedule = schedule;
    if (m_autoMode) onTick();
}

QMap<QTime, int> BrightnessScheduler::getSchedule() const {
    return m_schedule;
}

void BrightnessScheduler::setDevicePaths(const QList<QString>& paths) {
    m_devicePaths = paths;
    if (m_autoMode) {
        onTick();
    }
}

void BrightnessScheduler::setAutoModeEnabled(bool enabled) {
    m_autoMode = enabled;
    if (m_autoMode) {
        onTick();
    }
}

bool BrightnessScheduler::isAutoModeEnabled() const {
    return m_autoMode;
}

int BrightnessScheduler::calculateTargetBrightness(const QTime& currentTime) const {
    if (m_schedule.isEmpty()) return 50;
    if (m_schedule.size() == 1) return m_schedule.first();
    
    auto nextIt = m_schedule.upperBound(currentTime);
    auto prevIt = nextIt;
    
    if (nextIt == m_schedule.end()) {
        nextIt = m_schedule.begin();
        prevIt = m_schedule.end() - 1;
    } else if (nextIt == m_schedule.begin()) {
        prevIt = m_schedule.end() - 1;
    } else {
        prevIt = nextIt - 1;
    }
    
    QTime prevTime = prevIt.key();
    int prevBright = prevIt.value();
    QTime nextTime = nextIt.key();
    int nextBright = nextIt.value();
    
    int prevMins = prevTime.hour() * 60 + prevTime.minute();
    int nextMins = nextTime.hour() * 60 + nextTime.minute();
    int currMins = currentTime.hour() * 60 + currentTime.minute();
    
    if (nextMins <= prevMins) {
        nextMins += 24 * 60; // Crosses midnight
    }
    if (currMins < prevMins) {
        currMins += 24 * 60; // Currently past midnight boundary
    }
    
    int totalDiff = nextMins - prevMins;
    if (totalDiff == 0) return prevBright; 
    
    int passed = currMins - prevMins;
    double fraction = static_cast<double>(passed) / totalDiff;
    
    return prevBright + static_cast<int>((nextBright - prevBright) * fraction);
}

void BrightnessScheduler::onTick() {
    if (!m_autoMode || m_devicePaths.isEmpty() || m_schedule.isEmpty()) return;
    
    int target = calculateTargetBrightness(QTime::currentTime());
    qDebug() << "Auto-scaling brightness to target:" << target;
    
    emit brightnessChanged(target);
    
    for (const QString& path : m_devicePaths) {
        QString busNum = path.section('-', -1);
        QProcess::startDetached("ddcutil", {"--bus", busNum, "setvcp", "10", QString::number(target)});
    }
}

void BrightnessScheduler::loadConfig() {
    // Hardcoded safety net defaults
    m_schedule.clear();
    m_schedule.insert(QTime(7, 0), 20);
    m_schedule.insert(QTime(12, 0), 100);
    m_schedule.insert(QTime(18, 0), 80);
    m_schedule.insert(QTime(22, 0), 20);
    
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/dimwit";
    QDir().mkpath(configPath);
    QFile file(configPath + "/schedule.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject obj = doc.object();
        
        if (obj.contains("autoMode")) {
            m_autoMode = obj["autoMode"].toBool(false);
        }
        
        QJsonArray arr = obj["schedule"].toArray();
        if (!arr.isEmpty()) {
            m_schedule.clear();
            for (const QJsonValue& val : arr) {
                QJsonObject item = val.toObject();
                QTime t = QTime::fromString(item["time"].toString(), "HH:mm");
                if (t.isValid()) {
                    m_schedule.insert(t, item["brightness"].toInt());
                }
            }
        }
    }
}

void BrightnessScheduler::saveConfig() {
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/dimwit";
    QDir().mkpath(configPath);
    QFile file(configPath + "/schedule.json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        obj["autoMode"] = m_autoMode;
        
        QJsonArray arr;
        for (auto it = m_schedule.begin(); it != m_schedule.end(); ++it) {
            QJsonObject item;
            item["time"] = it.key().toString("HH:mm");
            item["brightness"] = it.value();
            arr.append(item);
        }
        
        obj["schedule"] = arr;
        file.write(QJsonDocument(obj).toJson());
    }
}
