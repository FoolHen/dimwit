#include "main.h"
#include "monitor_controller.h"
#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollArea>
#include <QProcess>
#include <QCheckBox>
#include <QSharedMemory>
#include "schedule_dialog.h"

MonitorControlApp::MonitorControlApp(int &argc, char **argv) : QApplication(argc, argv) {
    QCoreApplication::setApplicationName(Config::APP_NAME);
    
    // m_monitorController initialization only
    m_monitorController = new MonitorController();
    m_scheduler = new BrightnessScheduler(this);
    m_scheduler->loadConfig();
    
    // Pre-discover for background auto-updates without requiring window open
    QList<MonitorInfo> ms = m_monitorController->discoverMonitors();
    QList<QString> dPaths;
    for (const auto& m : ms) dPaths.append(m.devicePath);
    m_scheduler->setDevicePaths(dPaths);
    
    setupSystemTray();
}

MonitorControlApp::~MonitorControlApp() {}

void MonitorControlApp::setupSystemTray() {
    m_trayIcon = new QSystemTrayIcon(this);
    m_contextMenu = new QMenu();

    QAction *exitAction = new QAction("Exit", this);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
    
    m_contextMenu->addAction(exitAction);
    
    QIcon icon(Config::TRAY_ICON_PATH);
    m_trayIcon->setIcon(icon);
    m_trayIcon->setContextMenu(m_contextMenu);
    m_trayIcon->show();
    
    connect(m_trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            showControlWindow();
        }
    });
}

void MonitorControlApp::showControlWindow() {
    if (m_controlWindow) {
        m_controlWindow->raise();
        m_controlWindow->activateWindow();
        return;
    }
    
    m_controlWindow = new QWidget(nullptr);
    m_controlWindow->setAttribute(Qt::WA_DeleteOnClose);
    QWidget *window = m_controlWindow;
    window->setWindowTitle(Config::APP_NAME);
    window->resize(Config::WINDOW_WIDTH, -1);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(window);
    
    m_monitorSliders.clear();
    m_monitorDevicePaths.clear();
    
    setupTopControls(mainLayout);
    populateMonitorSliders(mainLayout);
    
    // Initial UI state setup for the sliders based on Automode toggle
    for (QSlider *slider : m_monitorSliders) {
        slider->setEnabled(!m_scheduler->isAutoModeEnabled());
    }
    
    // Refresh scheduler attached device paths as it might have connected/disconnected 
    m_scheduler->setDevicePaths(m_monitorDevicePaths);
    
    connect(m_scheduler, &BrightnessScheduler::brightnessChanged, window, [this](int newBrightness) {
        for (QSlider *slider : m_monitorSliders) {
            slider->setValue(newBrightness);
        }
    });
    
    mainLayout->addStretch();
    window->show();
}

void MonitorControlApp::setupTopControls(QVBoxLayout *mainLayout) {
    m_syncCheckbox = new QCheckBox("Sync Brightness");
    m_autoCheckbox = new QCheckBox("Auto Brightness");
    m_autoCheckbox->setChecked(m_scheduler->isAutoModeEnabled());
    m_settingsButton = new QPushButton("Settings...");
    
    QHBoxLayout *topControls = new QHBoxLayout();
    topControls->addWidget(m_syncCheckbox);
    topControls->addWidget(m_autoCheckbox);
    topControls->addWidget(m_settingsButton);
    mainLayout->addLayout(topControls);
    
    connect(m_autoCheckbox, &QCheckBox::stateChanged, [this](int state) {
        bool autoMode = (state == Qt::Checked);
        m_scheduler->setAutoModeEnabled(autoMode);
        m_scheduler->saveConfig();
        for (QSlider *slider : m_monitorSliders) {
            slider->setEnabled(!autoMode);
        }
    });
    
    connect(m_settingsButton, &QPushButton::clicked, [this]() {
        ScheduleDialog dlg(m_scheduler->getSchedule(), m_controlWindow);
        if (dlg.exec() == QDialog::Accepted) {
            m_scheduler->setSchedule(dlg.getSchedule());
            m_scheduler->saveConfig();
        }
    });
    
    connect(m_syncCheckbox, &QCheckBox::stateChanged, [this](int state) {
        if (state == Qt::Checked && !m_monitorSliders.isEmpty()) {
            int firstVal = m_monitorSliders.first()->value();
            for (auto slider : m_monitorSliders) {
                if (slider->value() != firstVal) {
                    slider->setValue(firstVal);
                }
            }
        }
    });
}

void MonitorControlApp::populateMonitorSliders(QVBoxLayout *mainLayout) {
    QList<MonitorInfo> monitors = m_monitorController->discoverMonitors();
    
    if (monitors.isEmpty()) {
        QLabel *noMonitorsLabel = new QLabel("No DDC-capable monitors found");
        noMonitorsLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(noMonitorsLabel);
        return;
    }
    
    for (const auto &monitor : monitors) {
        // Get initial brightness
        int currentBrightness = m_monitorController->getBrightness(monitor.devicePath);
        if (currentBrightness < 0) currentBrightness = 50; // default if failed
        
        QLabel *nameLabel = new QLabel(monitor.name);
        nameLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
        
        QHBoxLayout *sliderLayout = new QHBoxLayout();
        
        QSlider *slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 100);
        slider->setValue(currentBrightness);
        slider->setMinimumWidth(200);
        
        m_monitorSliders.append(slider);
        m_monitorDevicePaths.append(monitor.devicePath);
        
        QLabel *valLabel = new QLabel(QString("%1%").arg(currentBrightness));
        valLabel->setFixedWidth(40);
        valLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        
        // Real-time label update & sync across other sliders
        connect(slider, &QSlider::valueChanged, [this, valLabel, slider](int val){
            valLabel->setText(QString("%1%").arg(val));
            
            if (m_syncCheckbox->isChecked()) {
                for (QSlider* otherSlider : m_monitorSliders) {
                    if (otherSlider != slider && otherSlider->value() != val) {
                        otherSlider->setValue(val);
                    }
                }
            }
        });
        
        // Set brightness when released (syncing all displays if checked)
        QString devPath = monitor.devicePath;
        connect(slider, &QSlider::sliderReleased, [this, slider, devPath]() {
            int val = slider->value();
            
            if (m_syncCheckbox->isChecked()) {
                for (const QString& path : m_monitorDevicePaths) {
                    QString busNum = path.section('-', -1);
                    QProcess::startDetached("ddcutil", {"--bus", busNum, "setvcp", "10", QString::number(val)});
                }
            } else {
                QString busNum = devPath.section('-', -1);
                QProcess::startDetached("ddcutil", {"--bus", busNum, "setvcp", "10", QString::number(val)});
            }
        });
        
        sliderLayout->addWidget(slider);
        sliderLayout->addWidget(valLabel);
        
        mainLayout->addWidget(nameLabel);
        mainLayout->addLayout(sliderLayout);
    }
}

int main(int argc, char *argv[]) {
    QSharedMemory sharedMemory(Config::SHARED_MEM_KEY);
    if (!sharedMemory.create(1)) {
        return 0; // Instance already running
    }
    
    MonitorControlApp app(argc, argv);
    return app.exec();
}
