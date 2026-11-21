#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "calculator_engine.hpp"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("matcalc"));
    mat::CalculatorEngine calculator;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("calculator"), &calculator);
    engine.load(QUrl(QStringLiteral("qrc:/matcalc/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
