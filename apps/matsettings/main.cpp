#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "system_settings.hpp"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("matsettings"));
    mat::SystemSettings settings;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("settings"), &settings);
    engine.load(QUrl(QStringLiteral("qrc:/matsettings/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
