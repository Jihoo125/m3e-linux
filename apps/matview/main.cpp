#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "image_list_model.hpp"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("matview"));
    mat::ImageListModel images;
    if (argc > 1 && !images.openFile(QString::fromLocal8Bit(argv[1])))
        images.openFolder(QString::fromLocal8Bit(argv[1]));
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("images"), &images);
    engine.load(QUrl(QStringLiteral("qrc:/matview/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
