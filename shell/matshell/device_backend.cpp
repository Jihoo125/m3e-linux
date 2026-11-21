#include "device_backend.hpp"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTextStream>

namespace mat {
DeviceBackend::DeviceBackend(QObject *parent) : QObject(parent)
{
    const QDir backlight(QStringLiteral("/sys/class/backlight"));
    const auto entries = backlight.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (!entries.isEmpty()) {
        m_backlightPath = QStringLiteral("/sys/class/backlight/") + entries.front();
        m_maxBrightness = readPercent(m_backlightPath + QStringLiteral("/max_brightness"));
        m_brightness = readPercent(m_backlightPath + QStringLiteral("/brightness")) * 100 / qMax(1, m_maxBrightness);
        m_backend = QStringLiteral("Kernel backlight");
    } else {
        m_backend = QStringLiteral("No backlight device");
    }
}

int DeviceBackend::readPercent(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return 0;
    return file.readAll().trimmed().toInt();
}

bool DeviceBackend::writePercent(const QString &path, int value)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        emit error(QStringLiteral("Permission denied or unsupported device: %1").arg(path));
        return false;
    }
    file.write(QByteArray::number(value));
    return true;
}

void DeviceBackend::setVolume(int value)
{
    value = qBound(0, value, 100);
    m_volume = value;
    QProcess::startDetached(QStringLiteral("wpctl"), {QStringLiteral("set-volume"), QStringLiteral("@DEFAULT_AUDIO_SINK@"), QString::number(value / 100.0, 'f', 2)});
    emit volumeChanged();
}

void DeviceBackend::setMuted(bool value)
{
    m_muted = value;
    QProcess::startDetached(QStringLiteral("wpctl"), {QStringLiteral("set-mute"), QStringLiteral("@DEFAULT_AUDIO_SINK@"), value ? QStringLiteral("1") : QStringLiteral("0")});
    emit mutedChanged();
}

void DeviceBackend::setBrightness(int value)
{
    value = qBound(0, value, 100);
    m_brightness = value;
    if (!m_backlightPath.isEmpty())
        writePercent(m_backlightPath + QStringLiteral("/brightness"), value * m_maxBrightness / 100);
    emit brightnessChanged();
}
}
