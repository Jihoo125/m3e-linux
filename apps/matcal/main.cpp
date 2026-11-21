#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "calendar_model.hpp"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("matcal"));
    mat::CalendarModel calendar;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("calendar"), &calendar);
    engine.load(QUrl(QStringLiteral("qrc:/matcal/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
