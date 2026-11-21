#include "process_model.hpp"
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <QTimer>
#include <algorithm>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <pwd.h>

namespace mat {
ProcessModel::ProcessModel(QObject *parent) : QAbstractListModel(parent)
{
    m_ticksPerSecond = sysconf(_SC_CLK_TCK);
    m_pageSize = sysconf(_SC_PAGESIZE);
    sample();
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &ProcessModel::sample);
    timer->start(2000);
}

int ProcessModel::rowCount(const QModelIndex &) const { return m_processes.size(); }

QVariant ProcessModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_processes.size()) return {};
    const auto &p = m_processes[index.row()];
    switch (role) {
    case PidRole: return p.pid;
    case NameRole: return p.name;
    case UserRole: return p.user;
    case StateRole: return p.state;
    case CpuRole: return p.cpu;
    case MemoryRole: return p.rssBytes;
    default: return {};
    }
}

QHash<int, QByteArray> ProcessModel::roleNames() const
{
    return {{PidRole, "pid"}, {NameRole, "name"}, {UserRole, "user"}, {StateRole, "state"}, {CpuRole, "cpu"}, {MemoryRole, "memory"}};
}

void ProcessModel::setFilter(const QString &filter)
{
    if (filter == m_filter) return;
    m_filter = filter;
    emit filterChanged();
    sample();
}

void ProcessModel::sample()
{
    QFile meminfo(QStringLiteral("/proc/meminfo"));
    if (meminfo.open(QIODevice::ReadOnly)) {
        const auto lines = meminfo.readAll().split('\n');
        qint64 available = 0;
        for (const auto &line : lines) {
            const auto parts = QString::fromLatin1(line).split(' ', Qt::SkipEmptyParts);
            if (parts.size() < 2) continue;
            const qint64 value = parts[1].toLongLong() * 1024;
            if (parts[0] == "MemTotal:") m_total = value;
            else if (parts[0] == "MemAvailable:") available = value;
        }
        m_used = m_total - available;
    }
    QFile uptime(QStringLiteral("/proc/uptime"));
    qint64 uptimeSeconds = 0;
    if (uptime.open(QIODevice::ReadOnly)) uptimeSeconds = static_cast<qint64>(uptime.readAll().split(' ')[0].toDouble());
    m_uptime = QStringLiteral("%1h %2m").arg(uptimeSeconds / 3600).arg((uptimeSeconds % 3600) / 60);

    QVector<ProcessSample> next;
    double cpu = 0;
    const QDir proc(QStringLiteral("/proc"));
    const auto entries = proc.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &entry : entries) {
        bool numeric = false;
        const int pid = entry.toInt(&numeric);
        if (!numeric) continue;
        QFile stat(QStringLiteral("/proc/%1/stat").arg(pid));
        if (!stat.open(QIODevice::ReadOnly)) continue;
        const QString data = QString::fromLatin1(stat.readAll());
        const int openParen = data.indexOf(QLatin1Char('('));
        const int closeParen = data.lastIndexOf(QLatin1Char(')'));
        if (openParen < 0 || closeParen <= openParen) continue;
        const QString name = data.mid(openParen + 1, closeParen - openParen - 1);
        if (!m_filter.isEmpty() && !name.contains(m_filter, Qt::CaseInsensitive)) continue;
        const auto fields = data.mid(closeParen + 2).split(' ', Qt::SkipEmptyParts);
        if (fields.size() < 22) continue;
        ProcessSample p;
        p.pid = pid;
        p.name = name;
        p.state = fields[0];
        const qint64 utime = fields[11].toLongLong();
        const qint64 stime = fields[12].toLongLong();
        const qint64 total = utime + stime;
        p.rssBytes = fields[21].toLongLong() * m_pageSize;
        p.startTicks = fields[19].toLongLong();
        QFile status(QStringLiteral("/proc/%1/status").arg(pid));
        if (status.open(QIODevice::ReadOnly)) {
            const auto statusLines = status.readAll().split('\n');
            for (const auto &line : statusLines) {
                if (line.startsWith("Uid:")) {
                    // /proc status fields are tab-separated; normalize first.
                    const auto uidParts = QString::fromLatin1(line).simplified().split(' ', Qt::SkipEmptyParts);
                    if (uidParts.size() < 2) break;
                    const uid_t uid = uidParts[1].toUInt();
                    if (const passwd *pw = getpwuid(uid)) p.user = QString::fromLocal8Bit(pw->pw_name);
                    break;
                }
            }
        }
        for (const auto &old : m_processes) {
            if (old.pid == pid && old.hasPrevious && uptimeSeconds >= old.lastSampleUptime) {
                const double elapsed = double(uptimeSeconds - old.lastSampleUptime);
                if (elapsed > 0) p.cpu = std::max(0.0, double(total - old.lastTicks) / (m_ticksPerSecond * elapsed) * 100.0);
                break;
            }
        }
        p.lastTicks = total;
        p.lastSampleUptime = uptimeSeconds;
        p.hasPrevious = true;
        cpu += p.cpu;
        next.push_back(std::move(p));
    }
    if (m_sortCpu) std::sort(next.begin(), next.end(), [](const auto &a, const auto &b) { return a.cpu > b.cpu; });
    beginResetModel();
    m_processes = std::move(next);
    m_cpu = cpu;
    endResetModel();
    emit sampleChanged();
}

bool ProcessModel::terminate(int pid)
{
    if (pid <= 0 || ::kill(pid, SIGTERM) != 0) {
        emit operationError(QStringLiteral("Cannot terminate PID %1: %2").arg(pid).arg(QString::fromLocal8Bit(strerror(errno))));
        return false;
    }
    return true;
}

bool ProcessModel::kill(int pid)
{
    if (pid <= 0 || ::kill(pid, SIGKILL) != 0) {
        emit operationError(QStringLiteral("Cannot kill PID %1: %2").arg(pid).arg(QString::fromLocal8Bit(strerror(errno))));
        return false;
    }
    return true;
}

void ProcessModel::sortByCpu()
{
    m_sortCpu = !m_sortCpu;
    sample();
}
}
