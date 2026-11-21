#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "process_model.hpp"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("matsysmon"));
    mat::ProcessModel processes;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("processes"), &processes);
    engine.load(QUrl(QStringLiteral("qrc:/matsysmon/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
