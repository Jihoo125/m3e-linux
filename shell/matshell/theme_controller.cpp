#include "theme_controller.hpp"
#include <QDBusConnection>
#include <QDBusReply>
#include <QDir>
#include <QStandardPaths>

namespace mat {
ThemeController::ThemeController(QObject *parent) : QObject(parent)
{
    auto bus = QDBusConnection::sessionBus();
    if (bus.isConnected()) {
        m_theme = new QDBusInterface(QStringLiteral("org.materialized.Theme"), QStringLiteral("/org/materialized/Theme"),
                                     QStringLiteral("org.materialized.Theme1"), bus, this);
        m_dark = m_theme->property("IsDark").toBool();
    }
}

void ThemeController::setMode(const QString &mode)
{
    m_dark = mode == QLatin1String("dark");
    if (m_theme && m_theme->isValid()) {
        QDBusReply<void> reply = m_theme->call(QStringLiteral("SetMode"), mode);
        if (!reply.isValid()) emit error(reply.error().message());
    }
    switchSchemeFile(m_dark);
    emit darkChanged();
}

void ThemeController::switchSchemeFile(bool dark)
{
    QString runtime = qEnvironmentVariable("XDG_RUNTIME_DIR");
    if (runtime.isEmpty()) runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    const QString path = QDir(runtime).filePath(QStringLiteral("materialized/theme/scheme-%1.json").arg(dark ? QStringLiteral("dark") : QStringLiteral("light")));
    qputenv("MAT_SCHEME_PATH", path.toUtf8());
}

void ThemeController::setVariant(const QString &variant)
{
    if (!m_theme || !m_theme->isValid()) {
        emit error(QStringLiteral("matsessiond is not running"));
        return;
    }
    QDBusReply<void> reply = m_theme->call(QStringLiteral("SetVariant"), variant);
    if (!reply.isValid()) emit error(reply.error().message());
}

void ThemeController::setWallpaper(const QString &path)
{
    if (!m_theme || !m_theme->isValid()) {
        emit error(QStringLiteral("matsessiond is not running"));
        return;
    }
    QDBusReply<void> reply = m_theme->call(QStringLiteral("SetWallpaper"), path, QStringLiteral("crop"));
    if (!reply.isValid()) emit error(reply.error().message());
}
}
