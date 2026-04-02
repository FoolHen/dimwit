#ifndef SCHEDULEDIALOG_H
#define SCHEDULEDIALOG_H

#include <QDialog>
#include <QTime>
#include <QMap>
#include <QTableWidget>

class ScheduleDialog : public QDialog {
public:
    explicit ScheduleDialog(const QMap<QTime, int>& currentSchedule, QWidget *parent = nullptr);
    QMap<QTime, int> getSchedule() const;

private:
    void setupUi();
    void addRow(const QTime& time, int brightness);
    void sortTable();
    
    QTableWidget *m_table;
    QMap<QTime, int> m_schedule;
    
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // SCHEDULEDIALOG_H
