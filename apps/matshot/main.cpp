#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "screenshot_backend.hpp"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("matshot"));
    mat::ScreenshotBackend shot;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("shot"), &shot);
    engine.load(QUrl(QStringLiteral("qrc:/matshot/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
