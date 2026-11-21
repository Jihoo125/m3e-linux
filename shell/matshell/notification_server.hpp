#pragma once

#include <QDBusAbstractAdaptor>
#include <QObject>
#include <QVariantMap>
#include <QVector>

namespace mat {

struct Notification {
    uint id = 0;
    QString application;
    QString summary;
    QString body;
    QString icon;
    QStringList actions;
    QVariantMap hints;
    int timeout = 5000;
    QString time;
};

class NotificationServer final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList model READ model NOTIFY notificationsChanged)
    Q_PROPERTY(bool doNotDisturb READ doNotDisturb WRITE setDoNotDisturb NOTIFY doNotDisturbChanged)
public:
    explicit NotificationServer(QObject *parent = nullptr);
    QVariantList model() const;
    bool doNotDisturb() const;
    Q_INVOKABLE void setDoNotDisturb(bool value);

public slots:
    uint notify(const QString &appName, uint replacesId, const QString &appIcon,
                const QString &summary, const QString &body, const QStringList &actions,
                const QVariantMap &hints, int expireTimeout);
    Q_INVOKABLE void closeNotification(uint id);
    void invokeAction(uint id, const QString &actionKey);

signals:
    void notificationClosed(uint id, uint reason);
    void actionInvoked(uint id, const QString &actionKey);
    void notificationsChanged();
    void doNotDisturbChanged();

private:
    QVector<Notification> m_notifications;
    uint m_nextId = 1;
    bool m_dnd = false;
};

class NotificationAdaptor final : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")
    Q_CLASSINFO("D-Bus Introspection", "<interface name=\"org.freedesktop.Notifications\">\n"
        "<method name=\"Notify\"><arg type=\"s\" direction=\"in\"/><arg type=\"u\" direction=\"in\"/><arg type=\"s\" direction=\"in\"/><arg type=\"s\" direction=\"in\"/><arg type=\"s\" direction=\"in\"/><arg type=\"as\" direction=\"in\"/><arg type=\"a{sv}\" direction=\"in\"/><arg type=\"i\" direction=\"in\"/><arg type=\"u\" direction=\"out\"/></method>\n"
        "<method name=\"CloseNotification\"><arg type=\"u\" direction=\"in\"/></method>\n"
        "<method name=\"GetCapabilities\"><arg type=\"as\" direction=\"out\"/></method>\n"
        "<method name=\"GetServerInformation\"><arg type=\"s\" direction=\"out\"/><arg type=\"s\" direction=\"out\"/><arg type=\"s\" direction=\"out\"/><arg type=\"s\" direction=\"out\"/></method>\n"
        "<signal name=\"NotificationClosed\"><arg type=\"u\"/><arg type=\"u\"/></signal>\n"
        "<signal name=\"ActionInvoked\"><arg type=\"u\"/><arg type=\"s\"/></signal>\n"
        "</interface>")
public:
    explicit NotificationAdaptor(NotificationServer *server);
public slots:
    uint Notify(const QString &appName, uint replacesId, const QString &appIcon,
                const QString &summary, const QString &body, const QStringList &actions,
                const QVariantMap &hints, int expireTimeout);
    void CloseNotification(uint id);
    QStringList GetCapabilities();
    void GetServerInformation(QString &name, QString &vendor, QString &version, QString &specVersion);
signals:
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString &actionKey);
private:
    NotificationServer *m_server;
};

} // namespace mat
