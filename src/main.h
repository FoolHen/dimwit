#ifndef MAIN_H
#define MAIN_H

#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QSlider>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QCheckBox>
#include <QList>
#include <QString>

#include "monitormanager.h"
#include "brightness_scheduler.h"

class MonitorControlApp : public QApplication {
    
public:
    explicit MonitorControlApp(int &argc, char **argv);
    virtual ~MonitorControlApp();
    void showControlWindow();
    
private:
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_contextMenu;
    MonitorManager *m_monitorManager;
    QCheckBox *m_syncCheckbox;
    QCheckBox *m_autoCheckbox;
    QPushButton *m_settingsButton;
    QList<QSlider*> m_monitorSliders;
    QList<QString> m_monitorDevicePaths;
    BrightnessScheduler *m_scheduler;
};

#endif // MAIN_H
