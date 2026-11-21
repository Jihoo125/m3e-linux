#pragma once

#include <QDBusInterface>
#include <QDBusVariant>
#include <QObject>
#include <QVariantMap>

namespace mat {

/// Small typed D-Bus facade with consistent logging and invalid-call handling.
/// Apps should map user-facing failures at the page/view-model layer; system
/// service names and object paths are kept here instead of duplicated in QML.
class DBus {
public:
    static QVariantMap call(const QString &service, const QString &path,
                            const QString &interface, const QString &method,
                            const QVariantList &arguments = {});
    static bool callWithReply(const QString &service, const QString &path,
                              const QString &interface, const QString &method,
                              const QVariantList &arguments, QVariantMap *reply,
                              QString *error);
    static QVariant getProperty(const QString &service, const QString &path,
                                const QString &interface, const QString &property);
    static bool setProperty(const QString &service, const QString &path,
                            const QString &interface, const QString &property,
                            const QVariant &value, QString *error = nullptr);
    static std::shared_ptr<QDBusInterface> interface(const QString &service, const QString &path,
                                                     const QString &name);
};

} // namespace mat
