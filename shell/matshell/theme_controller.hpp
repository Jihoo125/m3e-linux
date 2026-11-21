#pragma once

#include <QDBusInterface>
#include <QObject>
#include <QVariantMap>

namespace mat {
class ThemeController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool dark READ dark NOTIFY darkChanged)
public:
    explicit ThemeController(QObject *parent = nullptr);
    bool dark() const { return m_dark; }
    Q_INVOKABLE void setMode(const QString &mode);
    Q_INVOKABLE void setVariant(const QString &variant);
    Q_INVOKABLE void setWallpaper(const QString &path);
signals:
    void darkChanged();
    void error(const QString &message);
private:
    void switchSchemeFile(bool dark);
    QDBusInterface *m_theme = nullptr;
    bool m_dark = false;
};
}
