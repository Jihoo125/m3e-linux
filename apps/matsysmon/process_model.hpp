#pragma once

#include <QAbstractListModel>
#include <QFileInfo>
#include <QProcess>
#include <QVector>

namespace mat {
struct ProcessSample {
    int pid = 0;
    QString name;
    QString user;
    QString state;
    double cpu = 0;
    qint64 rssBytes = 0;
    qint64 startTicks = 0;
    qint64 lastTicks = 0;
    qint64 lastSampleUptime = 0;
    bool hasPrevious = false;
};

class ProcessModel final : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(double totalCpu READ totalCpu NOTIFY sampleChanged)
    Q_PROPERTY(qint64 usedMemory READ usedMemory NOTIFY sampleChanged)
    Q_PROPERTY(qint64 totalMemory READ totalMemory NOTIFY sampleChanged)
    Q_PROPERTY(QString uptime READ uptime NOTIFY sampleChanged)
public:
    enum Role { PidRole = Qt::UserRole + 1, NameRole, UserRole, StateRole, CpuRole, MemoryRole };
    explicit ProcessModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QString filter() const { return m_filter; }
    void setFilter(const QString &filter);
    double totalCpu() const { return m_cpu; }
    qint64 usedMemory() const { return m_used; }
    qint64 totalMemory() const { return m_total; }
    QString uptime() const { return m_uptime; }
    Q_INVOKABLE void sample();
    Q_INVOKABLE bool terminate(int pid);
    Q_INVOKABLE bool kill(int pid);
    Q_INVOKABLE void sortByCpu();
signals:
    void filterChanged();
    void sampleChanged();
    void operationError(const QString &message);
private:
    QVector<ProcessSample> m_processes;
    QString m_filter;
    double m_cpu = 0;
    qint64 m_used = 0;
    qint64 m_total = 0;
    QString m_uptime;
    long m_ticksPerSecond = 100;
    long m_pageSize = 4096;
    bool m_sortCpu = true;
};
}
