#pragma once

#include <QDBusInterface>
#include <QObject>
#include <QVariantMap>

namespace mat {
class SystemSettings final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString hostname READ hostname NOTIFY systemChanged)
    Q_PROPERTY(QString timezone READ timezone NOTIFY systemChanged)
    Q_PROPERTY(QString keyboardLayout READ keyboardLayout NOTIFY systemChanged)
    Q_PROPERTY(QString operatingSystem READ operatingSystem CONSTANT)
    Q_PROPERTY(QString kernel READ kernel CONSTANT)
    Q_PROPERTY(QStringList locales READ locales NOTIFY systemChanged)
    Q_PROPERTY(bool networkAvailable READ networkAvailable NOTIFY systemChanged)
    Q_PROPERTY(QString networkStatus READ networkStatus NOTIFY systemChanged)
    Q_PROPERTY(QString storageSummary READ storageSummary NOTIFY systemChanged)
public:
    explicit SystemSettings(QObject *parent = nullptr);
    QString hostname() const { return m_hostname; }
    QString timezone() const { return m_timezone; }
    QString keyboardLayout() const { return m_layout; }
    QString operatingSystem() const { return m_os; }
    QString kernel() const { return m_kernel; }
    QStringList locales() const { return m_locales; }
    bool networkAvailable() const { return m_networkAvailable; }
    QString networkStatus() const { return m_networkStatus; }
    QString storageSummary() const { return m_storage; }
    Q_INVOKABLE bool setHostname(const QString &hostname);
    Q_INVOKABLE bool setTimezone(const QString &timezone);
    Q_INVOKABLE bool setKeyboardLayout(const QString &layout);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setDarkMode(bool dark);
    Q_INVOKABLE void setVariant(const QString &variant);
    Q_INVOKABLE void setAccent(const QString &color);
    Q_INVOKABLE void setWallpaper(const QString &path);
signals:
    void systemChanged();
    void error(const QString &message);
private:
    QString m_hostname;
    QString m_timezone;
    QString m_layout;
    QString m_os;
    QString m_kernel;
    QStringList m_locales;
    bool m_networkAvailable = false;
    QString m_networkStatus;
    QString m_storage;
};
}
