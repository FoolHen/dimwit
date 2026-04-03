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
#include <QPointer>

#include "monitor_controller.h"
#include "brightness_scheduler.h"

namespace Config {
    static const int WINDOW_WIDTH = 350;
    static const QString APP_NAME = "Dimwit";
    static const QString TRAY_ICON_PATH = ":/icons/dimwit-tray.png";
    static const QString SHARED_MEM_KEY = "DimwitAppInstance";
}

class MonitorControlApp : public QApplication {
    
public:
    explicit MonitorControlApp(int &argc, char **argv);
    virtual ~MonitorControlApp();
    void showControlWindow();
    
private:
    void setupSystemTray();
    void setupTopControls(QVBoxLayout *mainLayout);
    void populateMonitorSliders(QVBoxLayout *mainLayout);

    QSystemTrayIcon *m_trayIcon;
    QMenu *m_contextMenu;
    MonitorController *m_monitorController;
    QCheckBox *m_syncCheckbox;
    QCheckBox *m_autoCheckbox;
    QPushButton *m_settingsButton;
    QList<QSlider*> m_monitorSliders;
    QList<QString> m_monitorDevicePaths;
    BrightnessScheduler *m_scheduler;
    QPointer<QWidget> m_controlWindow;
};

#endif // MAIN_H
