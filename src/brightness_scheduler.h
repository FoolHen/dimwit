#ifndef BRIGHTNESS_SCHEDULER_H
#define BRIGHTNESS_SCHEDULER_H

#include <QObject>
#include <QTime>
#include <QMap>
#include <QTimer>
#include <QStringList>

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

private:
    void onTick();
    int calculateTargetBrightness(const QTime& currentTime) const;
    
    QMap<QTime, int> m_schedule;
    QList<QString> m_devicePaths;
    QTimer *m_timer;
    bool m_autoMode;
};

#endif // BRIGHTNESS_SCHEDULER_H
