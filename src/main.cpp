#include "main.h"
#include "monitormanager.h"
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
#include "scheduledialog.h"

MonitorControlApp::MonitorControlApp(int &argc, char **argv) : QApplication(argc, argv) {
    // Create system tray icon
    m_trayIcon = new QSystemTrayIcon(this);
    
    QCoreApplication::setApplicationName("Dimwit");
    
    // m_monitorManager initialization only
    m_monitorManager = new MonitorManager();
    m_scheduler = new BrightnessScheduler(this);
    m_scheduler->loadConfig();
    
    // Pre-discover for background auto-updates without requiring window open
    QList<MonitorInfo> ms = m_monitorManager->discoverMonitors();
    QList<QString> dPaths;
    for (const auto& m : ms) dPaths.append(m.devicePath);
    m_scheduler->setDevicePaths(dPaths);
    
    // Create context menu
    m_contextMenu = new QMenu();

    QAction *exitAction = new QAction("Exit", this);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
    
    m_contextMenu->addAction(exitAction);
    
    QIcon icon(":/icons/dimwit-tray.png");
    m_trayIcon->setIcon(icon);
    m_trayIcon->setContextMenu(m_contextMenu);
    m_trayIcon->show();
    
    connect(m_trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            showControlWindow();
        }
    });
}

MonitorControlApp::~MonitorControlApp() {}

void MonitorControlApp::showControlWindow() {
    QWidget *window = new QWidget(nullptr);
    window->setWindowTitle("Dimwit");
    window->resize(350, -1);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(window);
    
    m_monitorSliders.clear();
    m_monitorDevicePaths.clear();
    
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
    
    connect(m_settingsButton, &QPushButton::clicked, [this, window]() {
        ScheduleDialog dlg(m_scheduler->getSchedule(), window);
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

    QList<MonitorInfo> monitors = m_monitorManager->discoverMonitors();
    
    if (monitors.isEmpty()) {
        QLabel *noMonitorsLabel = new QLabel("No DDC-capable monitors found");
        noMonitorsLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(noMonitorsLabel);
    } else {
        for (const auto &monitor : monitors) {
            // Get initial brightness
            int currentBrightness = m_monitorManager->getBrightness(monitor.devicePath);
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
    
    // Initial UI state setup for the sliders based on Automode toggle
    for (QSlider *slider : m_monitorSliders) {
        slider->setEnabled(!m_scheduler->isAutoModeEnabled());
    }
    
    // Refresh scheduler attached device paths as it might have connected/disconnected 
    m_scheduler->setDevicePaths(m_monitorDevicePaths);
    
    mainLayout->addStretch();
    window->show();
}

int main(int argc, char *argv[]) {
    QSharedMemory sharedMemory("DimwitAppInstance");
    if (!sharedMemory.create(1)) {
        return 0; // Instance already running
    }
    
    MonitorControlApp app(argc, argv);
    return app.exec();
}
