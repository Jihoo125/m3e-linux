#include <QCoreApplication>
#include <QPluginLoader>
#include <QDebug>
#include <QQmlExtensionPlugin>
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QPluginLoader loader(QStringLiteral("/mnt/c/Users/iris8/Downloads/yes/build/qml/qml/Materialized/libm3plugin.so"));
    const auto metadata = loader.metaData();
    qDebug() << metadata;
    if (!loader.load()) { qCritical() << loader.errorString(); return 1; }
    auto *object = qobject_cast<QQmlExtensionPlugin *>(loader.instance());
    if (!object) { qCritical() << loader.errorString(); return 1; }
    object->registerTypes("Materialized");
    qInfo() << "loaded";
    return 0;
}
