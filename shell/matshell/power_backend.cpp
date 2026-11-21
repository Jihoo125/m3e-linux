#include "power_backend.hpp"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTimer>

namespace mat {
PowerBackend::PowerBackend(QObject *parent) : QObject(parent)
{
    readBattery();
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &PowerBackend::readBattery);
    timer->start(30000);
}

void PowerBackend::readBattery()
{
    const QDir supply(QStringLiteral("/sys/class/power_supply"));
    int total = 0;
    int count = 0;
    bool charging = false;
    for (const QString &name : supply.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (!name.startsWith(QLatin1String("BAT"))) continue;
        QFile file(QStringLiteral("/sys/class/power_supply/%1/capacity").arg(name));
        if (!file.open(QIODevice::ReadOnly)) continue;
        total += file.readAll().trimmed().toInt();
        ++count;
        QFile status(QStringLiteral("/sys/class/power_supply/%1/status").arg(name));
        if (status.open(QIODevice::ReadOnly)) charging = status.readAll().trimmed() != QByteArray("Discharging");
    }
    if (count > 0) m_percentage = total / count;
    m_charging = charging;
    emit batteryChanged();
}
}
