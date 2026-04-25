#ifndef BRIGHTNESS_SCHEDULER_H
#define BRIGHTNESS_SCHEDULER_H

#include <QObject>
#include <QTime>
#include <QMap>
#include <QTimer>
#include <QStringList>

namespace SchedulerConfig {
    static const int TICK_INTERVAL_MS = 15 * 60 * 1000;
    static const QString CONFIG_SUBDIR = "/dimwit";
    static const QString CONFIG_FILENAME = "/schedule.json";
    static const QString DDC_COMMAND = "ddcutil";
    static const QString VCP_BRIGHTNESS = "10";
}

class BrightnessScheduler : public QObject {
    Q_OBJECT
public:
    explicit BrightnessScheduler(QObject *parent = nullptr);
    
    void setSchedule(const QMap<QTime, int>& schedule);
    QMap<QTime, int> getSchedule() const;
    
    void setDevicePaths(const QList<QString>& paths);
    
    void setAutoModeEnabled(bool enabled);
    bool isAutoModeEnabled() const;
    
    void loadConfig();
    void saveConfig();

signals:
    void brightnessChanged(int newBrightness);

public slots:
    void onTick();

private:
    int calculateTargetBrightness(const QTime& currentTime) const;
    void loadDefaultSchedule();
    void applyBrightnessToDDC(int targetBrightness);
    
    QMap<QTime, int> m_schedule;
    QList<QString> m_devicePaths;
    QTimer *m_timer;
    bool m_autoMode;
};

#endif // BRIGHTNESS_SCHEDULER_H
