#pragma once

#include <QColor>
#include <QDateTime>
#include <QFileSystemWatcher>
#include <QFont>
#include <QObject>
#include <QQmlEngine>
#include <QQmlPropertyMap>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

namespace mat {

/// Live Material 3 design tokens for every QML component in the session.
///
/// The daemon writes a complete scheme before replacing the file atomically. The
/// watcher therefore never observes a partially-written palette. A 30 ms debounce
/// coalesces editor/live-session updates into one QML binding update.
class M3Theme final : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QQmlPropertyMap *colors READ colors CONSTANT)
    Q_PROPERTY(bool dark READ dark NOTIFY schemeChanged)
    Q_PROPERTY(QString sourceColor READ sourceColor NOTIFY schemeChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY schemeChanged)
    Q_PROPERTY(QString schemePath READ schemePath NOTIFY schemePathChanged)
    Q_PROPERTY(qreal contrast READ contrast NOTIFY schemeChanged)
    Q_PROPERTY(QString variant READ variant NOTIFY schemeChanged)
    Q_PROPERTY(int revision READ revision NOTIFY schemeChanged)
    Q_PROPERTY(bool highContrast READ highContrast NOTIFY schemeChanged)

public:
    explicit M3Theme(QObject *parent = nullptr);
    static M3Theme *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    QQmlPropertyMap *colors() { return &m_colors; }
    bool dark() const { return m_dark; }
    QString sourceColor() const { return m_sourceColor; }
    QString mode() const { return m_mode; }
    QString schemePath() const { return m_schemePath; }
    qreal contrast() const { return m_contrast; }
    QString variant() const { return m_variant; }
    int revision() const { return m_revision; }
    bool highContrast() const { return m_contrast >= 0.5; }

    Q_INVOKABLE QColor color(const QString &role) const;
    Q_INVOKABLE QFont font(const QString &scale) const;
    Q_INVOKABLE QFont fontEmphasized(const QString &scale) const;
    Q_INVOKABLE int radius(const QString &shape) const;
    Q_INVOKABLE int duration(const QString &name) const;
    Q_INVOKABLE QVariantList easing(const QString &name) const;
    /// Material 3 Expressive motion physics: a damped spring (mass = 1).
    /// Returns {spring, damping, epsilon} for QML SpringAnimation. Spatial
    /// springs are under-damped (bounce); effects springs are critical.
    Q_INVOKABLE QVariantMap spring(const QString &name) const;
    Q_INVOKABLE int spacing(const QString &name) const;
    Q_INVOKABLE bool reload();

signals:
    void schemeChanged();
    void schemePathChanged();
    void loadError(const QString &message);

private:
    void installDefaults();
    void loadScheme();
    void watchScheme();
    static QString defaultSchemePath();

    QQmlPropertyMap m_colors{this};
    QFileSystemWatcher m_watcher;
    QTimer m_reloadTimer;
    QString m_schemePath;
    QString m_sourceColor;
    QString m_mode{QStringLiteral("light")};
    bool m_dark = false;
    qreal m_contrast = 0.0;
    QString m_variant{QStringLiteral("tonal-spot")};
    int m_revision = 0;
    bool m_loading = false;
};

} // namespace mat
