#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "terminal_emulator.hpp"
#include "pty_session.hpp"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("matterminal"));
    mat::TerminalEmulator emulator;
    mat::PtySession pty(&emulator);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("terminal"), &emulator);
    engine.rootContext()->setContextProperty(QStringLiteral("pty"), &pty);
    engine.load(QUrl(QStringLiteral("qrc:/matterminal/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
