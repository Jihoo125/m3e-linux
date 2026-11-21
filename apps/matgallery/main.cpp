#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QColor>
#include <QDebug>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("matgallery"));
    QQuickView view;
    if (qEnvironmentVariableIsSet("QML2_IMPORT_PATH"))
        view.engine()->addImportPath(qEnvironmentVariable("QML2_IMPORT_PATH"));
    const QString schemePath = qEnvironmentVariable("MAT_SCHEME_PATH");
    if (!schemePath.isEmpty()) {
        QFile file(schemePath);
        if (file.open(QIODevice::ReadOnly)) {
            const auto scheme = QJsonDocument::fromJson(file.readAll()).object();
            const QColor color(scheme.value(QStringLiteral("roles")).toObject().value(QStringLiteral("background")).toString());
            if (color.isValid()) view.setColor(color);
        }
    }
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(1120, 840);
    view.setSource(QUrl(QStringLiteral("qrc:/Materialized/Gallery/Main.qml")));
    if (view.status() == QQuickView::Error) {
        qWarning() << view.errors();
        return 1;
    }
    view.show();
    const QString screenshot = qEnvironmentVariable("MAT_SCREENSHOT_PATH");
    if (!screenshot.isEmpty()) {
        auto *timer = new QTimer(&view);
        timer->setSingleShot(true);
        QObject::connect(timer, &QTimer::timeout, &view, [&view, screenshot, &app]() {
            const QImage image = view.grabWindow();
            if (image.isNull() || !image.save(screenshot)) {
                qCritical() << "Failed to save screenshot" << screenshot;
                app.exit(2);
                return;
            }
            qInfo() << "Saved" << screenshot << image.size();
            app.quit();
        });
        timer->start(1000);
    }
    return app.exec();
}
