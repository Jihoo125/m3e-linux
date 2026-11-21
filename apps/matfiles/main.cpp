#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "file_manager_model.hpp"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("matfiles"));
    mat::FileManagerModel files;
    if (argc > 1) files.setDirectory(QString::fromLocal8Bit(argv[1]));
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("files"), &files);
    engine.load(QUrl(QStringLiteral("qrc:/matfiles/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
