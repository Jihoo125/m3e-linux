#include "themed_service.hpp"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>
#include <QDir>
#include <QImage>
#include <QStandardPaths>
#include <iostream>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("matsessiond"));
    QCoreApplication::setOrganizationName(QStringLiteral("MaterializedLinux"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Materialized session theme and wallpaper service"));
    parser.addHelpOption();
    parser.addOption({{QStringLiteral("o"), QStringLiteral("output")}, QStringLiteral("Theme output directory"), QStringLiteral("directory")});
    parser.addOption({QStringLiteral("once"), QStringLiteral("Generate schemes and exit")});
    parser.addOption({QStringLiteral("wallpaper"), QStringLiteral("Extract a seed from this image"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("accent"), QStringLiteral("Fixed accent (#RRGGBB)"), QStringLiteral("color")});
    parser.addOption({QStringLiteral("mode"), QStringLiteral("light, dark or auto"), QStringLiteral("mode"), QStringLiteral("light")});
    parser.addOption({QStringLiteral("variant"), QStringLiteral("Material variant"), QStringLiteral("name"), QStringLiteral("tonal-spot")});
    parser.addOption({QStringLiteral("contrast"), QStringLiteral("Contrast in [-1, 1]"), QStringLiteral("number"), QStringLiteral("0")});
    parser.addOption({QStringLiteral("no-dbus"), QStringLiteral("Do not register on the session bus")});
    parser.process(app);

    const QString runtime = parser.value(QStringLiteral("output")).isEmpty()
            ? QDir(qEnvironmentVariable("XDG_RUNTIME_DIR", QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)))
                  .filePath(QStringLiteral("materialized/theme"))
            : parser.value(QStringLiteral("output"));

    mat::ThemeService service(runtime, !parser.isSet(QStringLiteral("accent")) && !parser.isSet(QStringLiteral("wallpaper")));
    const auto variant = mat::ThemeService::variantFromName(parser.value(QStringLiteral("variant")));
    uint accent = 0xff6750a4;
    if (!parser.value(QStringLiteral("accent")).isEmpty()) {
        bool ok = false;
        accent = 0xff000000u | parser.value(QStringLiteral("accent")).remove(QLatin1Char('#')).toUInt(&ok, 16);
        if (!ok) {
            std::cerr << "Invalid accent\n";
            return 2;
        }
    }
    const bool wallpaperRequested = parser.isSet(QStringLiteral("wallpaper"));
    service.setOptions(variant, parser.value(QStringLiteral("contrast")).toDouble(), accent, wallpaperRequested);
    if (wallpaperRequested) {
        QImage probe(parser.value(QStringLiteral("wallpaper")));
        if (probe.isNull()) {
            std::cerr << "Cannot decode wallpaper\n";
            return 2;
        }
        service.SetWallpaper(parser.value(QStringLiteral("wallpaper")), QStringLiteral("crop"));
    }
    if (parser.isSet(QStringLiteral("mode")) && parser.value(QStringLiteral("mode")) != QLatin1String("light"))
        service.SetMode(parser.value(QStringLiteral("mode")));

    if (!parser.isSet(QStringLiteral("no-dbus"))) {
        auto bus = QDBusConnection::sessionBus();
        if (!bus.isConnected()) {
            qWarning() << "No session D-Bus; continuing with file-based theming";
        } else if (!bus.registerObject(QStringLiteral("/org/materialized/Theme"), &service, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties | QDBusConnection::ExportAllSignals)
                   || !bus.registerService(QStringLiteral("org.materialized.Theme"))) {
            qWarning() << "Could not claim org.materialized.Theme on session bus";
            return 3;
        }
    }
    if (parser.isSet(QStringLiteral("once"))) {
        std::cout << runtime.toStdString() << '\n';
        return 0;
    }
    return app.exec();
}
