#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "text_document.hpp"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("mattext"));
    mat::TextDocument document;
    if (argc > 1) document.open(QString::fromLocal8Bit(argv[1]));
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("textDocument"), &document);
    engine.load(QUrl(QStringLiteral("qrc:/mattext/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
