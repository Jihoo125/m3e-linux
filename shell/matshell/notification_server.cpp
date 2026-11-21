#include "notification_server.hpp"

#include <QDateTime>
#include <QTimer>

namespace mat {

NotificationServer::NotificationServer(QObject *parent) : QObject(parent) {}

QVariantList NotificationServer::model() const
{
    QVariantList result;
    for (const auto &n : m_notifications) {
        result.append(QVariantMap{{"id", n.id}, {"application", n.application}, {"summary", n.summary},
            {"body", n.body}, {"icon", n.icon}, {"actions", n.actions}, {"time", n.time}, {"timeout", n.timeout}});
    }
    return result;
}

bool NotificationServer::doNotDisturb() const { return m_dnd; }
void NotificationServer::setDoNotDisturb(bool value)
{
    if (value == m_dnd) return;
    m_dnd = value;
    emit doNotDisturbChanged();
}

uint NotificationServer::notify(const QString &appName, uint replacesId, const QString &appIcon,
                                const QString &summary, const QString &body, const QStringList &actions,
                                const QVariantMap &hints, int expireTimeout)
{
    Notification n;
    n.id = replacesId == 0 ? m_nextId++ : replacesId;
    n.application = appName;
    n.summary = summary;
    n.body = body;
    n.icon = appIcon;
    n.actions = actions;
    n.hints = hints;
    n.timeout = expireTimeout;
    n.time = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm"));
    bool found = false;
    for (int i = 0; i < m_notifications.size(); ++i) {
        if (m_notifications[i].id == n.id) { m_notifications[i] = n; found = true; break; }
    }
    if (!found) m_notifications.prepend(n);
    if (m_notifications.size() > 100) m_notifications.resize(100);
    emit notificationsChanged();
    if (n.timeout > 0 && !m_dnd)
        QTimer::singleShot(n.timeout, this, [this, id = n.id]() { closeNotification(id); });
    return n.id;
}

void NotificationServer::closeNotification(uint id)
{
    for (int i = 0; i < m_notifications.size(); ++i) {
        if (m_notifications[i].id == id) {
            m_notifications.removeAt(i);
            emit notificationClosed(id, 2);
            emit notificationsChanged();
            return;
        }
    }
}

void NotificationServer::invokeAction(uint id, const QString &actionKey)
{
    emit actionInvoked(id, actionKey);
}

NotificationAdaptor::NotificationAdaptor(NotificationServer *server)
    : QDBusAbstractAdaptor(server), m_server(server)
{
    connect(server, &NotificationServer::notificationClosed,
            this, &NotificationAdaptor::NotificationClosed);
    connect(server, &NotificationServer::actionInvoked,
            this, &NotificationAdaptor::ActionInvoked);
}

uint NotificationAdaptor::Notify(const QString &appName, uint replacesId, const QString &appIcon,
                                 const QString &summary, const QString &body, const QStringList &actions,
                                 const QVariantMap &hints, int expireTimeout)
{
    return m_server->notify(appName, replacesId, appIcon, summary, body, actions, hints, expireTimeout);
}

void NotificationAdaptor::CloseNotification(uint id) { m_server->closeNotification(id); }

QStringList NotificationAdaptor::GetCapabilities()
{
    return {QStringLiteral("actions"), QStringLiteral("body"), QStringLiteral("body-markup"), QStringLiteral("persistence"), QStringLiteral("icon-static")};
}

void NotificationAdaptor::GetServerInformation(QString &name, QString &vendor, QString &version, QString &specVersion)
{
    name = QStringLiteral("Materialized Notifications");
    vendor = QStringLiteral("Materialized Linux");
    version = QStringLiteral("0.2.0");
    specVersion = QStringLiteral("1.2");
}

} // namespace mat
