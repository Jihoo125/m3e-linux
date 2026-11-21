#include "dbus.hpp"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDebug>
#include <memory>

namespace mat {
namespace {
std::shared_ptr<QDBusInterface> makeInterface(const QString &service, const QString &path, const QString &name)
{
    auto result = std::make_shared<QDBusInterface>(service, path, name, QDBusConnection::systemBus());
    result->setTimeout(20000);
    return result;
}
}

std::shared_ptr<QDBusInterface> DBus::interface(const QString &service, const QString &path, const QString &name)
{
    return std::make_shared<QDBusInterface>(service, path, name, QDBusConnection::systemBus());
}

bool DBus::callWithReply(const QString &service, const QString &path, const QString &name,
                         const QString &method, const QVariantList &arguments,
                         QVariantMap *reply, QString *error)
{
    auto iface = makeInterface(service, path, name);
    if (!iface->isValid()) {
        if (error) *error = iface->lastError().message();
        return false;
    }
    QDBusMessage message = iface->callWithArgumentList(QDBus::Block, method, arguments);
    if (message.type() == QDBusMessage::ErrorMessage) {
        if (error) *error = message.errorMessage();
        qWarning() << service << path << name << method << message.errorMessage();
        return false;
    }
    if (reply) {
        reply->clear();
        const QList<QVariant> values = message.arguments();
        for (int i = 0; i < values.size(); ++i)
            reply->insert(QString::number(i), values.at(i));
    }
    return true;
}

QVariantMap DBus::call(const QString &service, const QString &path, const QString &name,
                       const QString &method, const QVariantList &arguments)
{
    QVariantMap reply;
    callWithReply(service, path, name, method, arguments, &reply, nullptr);
    return reply;
}

QVariant DBus::getProperty(const QString &service, const QString &path, const QString &name,
                           const QString &property)
{
    QDBusInterface iface(service, path, QStringLiteral("org.freedesktop.DBus.Properties"),
                               QDBusConnection::systemBus());
    QDBusReply<QDBusVariant> reply = iface.call(QDBus::Block, QStringLiteral("Get"), name, property);
    if (!reply.isValid()) {
        qWarning() << "D-Bus property read failed:" << service << path << name << property << reply.error().message();
        return {};
    }
    return reply.value().variant();
}

bool DBus::setProperty(const QString &service, const QString &path, const QString &name,
                       const QString &property, const QVariant &value, QString *error)
{
    QDBusInterface iface(service, path, QStringLiteral("org.freedesktop.DBus.Properties"),
                               QDBusConnection::systemBus());
    QDBusReply<void> reply = iface.call(QDBus::Block, QStringLiteral("Set"), name, property, QVariant::fromValue(QDBusVariant(value)));
    if (!reply.isValid()) {
        if (error) *error = reply.error().message();
        return false;
    }
    return true;
}

} // namespace mat
