#include "schedule_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTimeEdit>
#include <QSpinBox>
#include <QHeaderView>
#include <QKeyEvent>

ScheduleDialog::ScheduleDialog(const QMap<QTime, int>& currentSchedule, QWidget *parent)
    : QDialog(parent), m_schedule(currentSchedule) {
    setWindowTitle("Schedule Settings");
    resize(300, 400);
    setupUi();
}

void ScheduleDialog::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels({"Time", "Brightness", ""});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    
    layout->addWidget(m_table);
    
    for (auto it = m_schedule.begin(); it != m_schedule.end(); ++it) {
        addRow(it.key(), it.value());
    }
    
    QPushButton *addBtn = new QPushButton("Add Point", this);
    addBtn->setAutoDefault(false);
    connect(addBtn, &QPushButton::clicked, [this]() {
        addRow(QTime::currentTime(), 50);
        sortTable();
    });
    
    QPushButton *saveBtn = new QPushButton("Save", this);
    saveBtn->setDefault(true);
    connect(saveBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    QPushButton *cancelBtn = new QPushButton("Cancel", this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(addBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    layout->addLayout(btnLayout);
}

void ScheduleDialog::addRow(const QTime& time, int brightness) {
    int row = m_table->rowCount();
    m_table->insertRow(row);
    
    QTimeEdit *timeEdit = new QTimeEdit(time);
    timeEdit->setDisplayFormat("HH:mm");
    m_table->setCellWidget(row, 0, timeEdit);
    
    QSpinBox *spinBox = new QSpinBox();
    spinBox->setRange(0, 100);
    spinBox->setValue(brightness);
    spinBox->setSuffix("%");
    m_table->setCellWidget(row, 1, spinBox);
    
    timeEdit->installEventFilter(this);
    spinBox->installEventFilter(this);
    
    QPushButton *delBtn = new QPushButton("X");
    connect(delBtn, &QPushButton::clicked, [this, delBtn]() {
        for (int i = 0; i < m_table->rowCount(); ++i) {
            if (m_table->cellWidget(i, 2) == delBtn) {
                m_table->removeRow(i);
                break;
            }
        }
    });
    m_table->setCellWidget(row, 2, delBtn);
}

void ScheduleDialog::sortTable() {
    QList<QPair<QTime, int>> items;
    for (int i = 0; i < m_table->rowCount(); ++i) {
        QTimeEdit *timeEdit = qobject_cast<QTimeEdit*>(m_table->cellWidget(i, 0));
        QSpinBox *spinBox = qobject_cast<QSpinBox*>(m_table->cellWidget(i, 1));
        if (timeEdit && spinBox) {
            items.append({timeEdit->time(), spinBox->value()});
        }
    }
    
    std::sort(items.begin(), items.end(), [](const QPair<QTime, int>& a, const QPair<QTime, int>& b) {
        return a.first < b.first;
    });
    
    for (int i = 0; i < items.size(); ++i) {
        QTimeEdit *timeEdit = qobject_cast<QTimeEdit*>(m_table->cellWidget(i, 0));
        QSpinBox *spinBox = qobject_cast<QSpinBox*>(m_table->cellWidget(i, 1));
        if (timeEdit && spinBox) {
            timeEdit->setTime(items[i].first);
            spinBox->setValue(items[i].second);
        }
    }
}

bool ScheduleDialog::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            QWidget *w = qobject_cast<QWidget*>(obj);
            if (w) w->clearFocus();
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

QMap<QTime, int> ScheduleDialog::getSchedule() const {
    QMap<QTime, int> newSchedule;
    for (int i = 0; i < m_table->rowCount(); ++i) {
        QTimeEdit *timeEdit = qobject_cast<QTimeEdit*>(m_table->cellWidget(i, 0));
        QSpinBox *spinBox = qobject_cast<QSpinBox*>(m_table->cellWidget(i, 1));
        if (timeEdit && spinBox) {
            newSchedule.insert(timeEdit->time(), spinBox->value());
        }
    }
    return newSchedule;
}
