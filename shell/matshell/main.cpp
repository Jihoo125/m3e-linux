#include <QDBusConnection>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QColor>
#include <QTimer>
#include <fcntl.h>
#include <unistd.h>
#include "application_model.hpp"
#include "notification_server.hpp"
#include "theme_controller.hpp"
#include "device_backend.hpp"
#include "power_backend.hpp"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("matshell"));
    QObject shellServices;
    mat::NotificationServer notifications(&shellServices);
    mat::ApplicationModel applications(&shellServices);
    mat::NotificationAdaptor notificationAdaptor(&notifications);
    auto bus = QDBusConnection::sessionBus();
    if (bus.isConnected()) {
        bus.registerObject(QStringLiteral("/org/freedesktop/Notifications"), &notificationAdaptor,
                           QDBusConnection::ExportAdaptors);
        if (!bus.registerService(QStringLiteral("org.freedesktop.Notifications")))
            qWarning() << "Another notification daemon owns org.freedesktop.Notifications";
    }
    mat::ThemeController theme;
    mat::PowerBackend power;
    mat::DeviceBackend devices;
    QQuickView view;
    auto *context = view.engine()->rootContext();
    context->setContextProperty(QStringLiteral("themeInterface"), &theme);
    context->setContextProperty(QStringLiteral("devices"), &devices);
    context->setContextProperty(QStringLiteral("power"), &power);
    context->setContextProperty(QStringLiteral("applicationModel"), &applications);
    context->setContextProperty(QStringLiteral("notificationServer"), &notifications);
    if (qEnvironmentVariableIsSet("QML2_IMPORT_PATH"))
        view.engine()->addImportPath(qEnvironmentVariable("QML2_IMPORT_PATH"));
    if (qEnvironmentVariableIsSet("MAT_WALLPAPER")) applications.setWallpaper(qEnvironmentVariable("MAT_WALLPAPER"));
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(1280, 800);
    view.setSource(QUrl(QStringLiteral("qrc:/shell/Main.qml")));
    if (view.status() == QQuickView::Error) {
        qWarning() << view.errors();
        return 1;
    }
    view.show();
    // XDG autostart once the shell is on screen (real desktop mechanism).
    QTimer::singleShot(2000, &applications, [&applications]() { applications.launchAutostart(); });
    qInfo() << "Shell window exposed:" << view.isExposed() << "geometry:" << view.geometry() << "platform:" << QGuiApplication::platformName();
    // The compositor exposes the window asynchronously after its first frame.
    // Emit the acceptance marker only once the shell is genuinely on screen.
    auto *ready = new QTimer(&app);
    ready->setInterval(100);
    QObject::connect(ready, &QTimer::timeout, [&view, ready]() {
        if (!view.isExposed()) return;
        ready->stop();
        qInfo() << "Shell exposed by compositor";
        const int serial = ::open("/dev/ttyS0", O_WRONLY | O_NOCTTY);
        if (serial >= 0) {
            QByteArray payload = QStringLiteral("Shell visible=%1 exposed=%2 geometry=%3x%4\nMaterialized session started\n")
                .arg(view.isVisible()).arg(view.isExposed()).arg(view.width()).arg(view.height()).toUtf8();
            const char *data = payload.constData();
            int remaining = payload.size();
            while (remaining > 0) {
                const ssize_t written = ::write(serial, data, remaining);
                if (written <= 0) break;
                data += written;
                remaining -= static_cast<int>(written);
            }
            ::close(serial);
        }
    });
    ready->start();
    QObject::connect(&view, &QWindow::visibleChanged, [](bool visible) { qInfo() << "Shell visibility changed:" << visible; });
    if (qEnvironmentVariableIsSet("MAT_SCREENSHOT_PATH")) {
        QTimer::singleShot(1500, &view, [&view, &app]() {
            const QString path = qEnvironmentVariable("MAT_SCREENSHOT_PATH");
            if (!view.grabWindow().save(path)) {
                qWarning() << "Could not save shell screenshot" << path;
                app.exit(2);
                return;
            }
            qInfo() << "Saved" << path;
            app.quit();
        });
    }
    return app.exec();
}
