#include "system_settings.hpp"
#include <QDBusConnection>
#include <QDBusReply>
#include <QColor>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStorageInfo>
#include <QDateTime>
#include <QSysInfo>

namespace mat {
SystemSettings::SystemSettings(QObject *parent) : QObject(parent)
{
    m_os = QSysInfo::prettyProductName();
    m_kernel = QSysInfo::kernelVersion();
    refresh();
}

void SystemSettings::refresh()
{
    m_hostname = QSysInfo::machineHostName();
    m_timezone = QDateTime::currentDateTime().timeZoneAbbreviation();
    m_layout = QStringLiteral("us");
    QFile layout(QStringLiteral("/etc/default/keyboard"));
    if (layout.open(QIODevice::ReadOnly)) {
        for (const auto &line : layout.readAll().split('\n')) {
            if (line.startsWith("XKBLAYOUT=")) m_layout = QString::fromUtf8(line).section('=', 1).remove('"');
        }
    }
    m_locales = {QStringLiteral("en_US.UTF-8"), QStringLiteral("C.UTF-8")};
    m_storage = QStringLiteral("%1 GiB free of %2 GiB")
        .arg(QStorageInfo::root().bytesAvailable() / 1073741824.0, 0, 'f', 1)
        .arg(QStorageInfo::root().bytesTotal() / 1073741824.0, 0, 'f', 1);
    QDBusInterface network(QStringLiteral("org.freedesktop.NetworkManager"), QStringLiteral("/org/freedesktop/NetworkManager"),
                           QStringLiteral("org.freedesktop.NetworkManager"), QDBusConnection::systemBus());
    m_networkAvailable = network.isValid();
    m_networkStatus = m_networkAvailable ? QStringLiteral("NetworkManager running") : QStringLiteral("NetworkManager unavailable");
    emit systemChanged();
}

bool SystemSettings::setHostname(const QString &hostname)
{
    if (hostname.isEmpty() || hostname.length() > 63) {
        emit error(QStringLiteral("Hostname must be 1–63 characters"));
        return false;
    }
    QDBusInterface hostnamed(QStringLiteral("org.freedesktop.hostname1"), QStringLiteral("/org/freedesktop/hostname1"),
                             QStringLiteral("org.freedesktop.hostname1"), QDBusConnection::systemBus());
    QDBusReply<void> reply = hostnamed.call(QStringLiteral("SetStaticHostname"), hostname, false);
    if (!reply.isValid()) { emit error(reply.error().message()); return false; }
    m_hostname = hostname;
    emit systemChanged();
    return true;
}

bool SystemSettings::setTimezone(const QString &timezone)
{
    if (!QFileInfo::exists(QStringLiteral("/usr/share/zoneinfo/") + timezone)) {
        emit error(QStringLiteral("Unknown timezone: %1").arg(timezone));
        return false;
    }
    QDBusInterface timedated(QStringLiteral("org.freedesktop.timedate1"), QStringLiteral("/org/freedesktop/timedate1"),
                             QStringLiteral("org.freedesktop.timedate1"), QDBusConnection::systemBus());
    QDBusReply<void> reply = timedated.call(QStringLiteral("SetTimezone"), timezone, false);
    if (!reply.isValid()) { emit error(reply.error().message()); return false; }
    m_timezone = timezone;
    emit systemChanged();
    return true;
}

bool SystemSettings::setKeyboardLayout(const QString &layout)
{
    QDBusInterface localed(QStringLiteral("org.freedesktop.locale1"), QStringLiteral("/org/freedesktop/locale1"),
                           QStringLiteral("org.freedesktop.locale1"), QDBusConnection::systemBus());
    QDBusReply<void> reply = localed.call(QStringLiteral("SetX11Keyboard"), layout, QString(), QString(), QString(), false, false);
    if (!reply.isValid()) { emit error(reply.error().message()); return false; }
    m_layout = layout;
    emit systemChanged();
    return true;
}

void SystemSettings::setDarkMode(bool dark)
{
    QDBusInterface theme(QStringLiteral("org.materialized.Theme"), QStringLiteral("/org/materialized/Theme"),
                         QStringLiteral("org.materialized.Theme1"), QDBusConnection::sessionBus());
    if (!theme.isValid()) {
        const QString mode = dark ? QStringLiteral("dark") : QStringLiteral("light");
        QProcess::startDetached(QStringLiteral("matsessiond"), {QStringLiteral("--mode"), mode});
        qputenv("MAT_FORCE_DARK", dark ? "1" : "0");
        return;
    }
    QDBusReply<void> reply = theme.call(QStringLiteral("SetMode"), dark ? QStringLiteral("dark") : QStringLiteral("light"));
    if (!reply.isValid()) emit error(reply.error().message());
}

void SystemSettings::setVariant(const QString &variant)
{
    QDBusInterface theme(QStringLiteral("org.materialized.Theme"), QStringLiteral("/org/materialized/Theme"),
                         QStringLiteral("org.materialized.Theme1"), QDBusConnection::sessionBus());
    if (!theme.isValid()) { emit error(QStringLiteral("matsessiond is not running")); return; }
    QDBusReply<void> reply = theme.call(QStringLiteral("SetVariant"), variant);
    if (!reply.isValid()) emit error(reply.error().message());
}

void SystemSettings::setAccent(const QString &color)
{
    if (!QColor(color).isValid()) { emit error(QStringLiteral("Invalid colour")); return; }
    QDBusInterface theme(QStringLiteral("org.materialized.Theme"), QStringLiteral("/org/materialized/Theme"),
                         QStringLiteral("org.materialized.Theme1"), QDBusConnection::sessionBus());
    QDBusReply<void> reply = theme.call(QStringLiteral("SetAccent"), QVariantMap{{QStringLiteral("mode"), QStringLiteral("fixed")}, {QStringLiteral("argb"), color}});
    if (!reply.isValid()) emit error(reply.error().message());
}

void SystemSettings::setWallpaper(const QString &path)
{
    if (!QFileInfo::exists(path)) { emit error(QStringLiteral("Wallpaper not found")); return; }
    QDBusInterface theme(QStringLiteral("org.materialized.Theme"), QStringLiteral("/org/materialized/Theme"),
                         QStringLiteral("org.materialized.Theme1"), QDBusConnection::sessionBus());
    QDBusReply<void> reply = theme.call(QStringLiteral("SetWallpaper"), path, QStringLiteral("crop"));
    if (!reply.isValid()) emit error(reply.error().message());
}
}
