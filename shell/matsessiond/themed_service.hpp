#pragma once

#include <QDBusContext>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include "m3/m3.hpp"

namespace mat {

struct Scheme {
    QVariantMap roles;
    QString sourceColor;
    QString mode;
    bool dark = false;
    double contrast = 0.0;
    int variant = static_cast<int>(m3::Variant::kTonalSpot);
};

class ThemeService : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.materialized.Theme1")
    Q_PROPERTY(QVariantMap Scheme READ schemeMap NOTIFY SchemeChanged)
    Q_PROPERTY(bool IsDark READ isDark NOTIFY SchemeChanged)
    Q_PROPERTY(QString Variant READ variantName NOTIFY SchemeChanged)
    Q_PROPERTY(double Contrast READ contrastLevel NOTIFY SchemeChanged)
    Q_PROPERTY(QString AccentMode READ accentMode NOTIFY SchemeChanged)
    Q_PROPERTY(uint AccentColor READ accentColor NOTIFY SchemeChanged)
    Q_PROPERTY(QString Mode READ mode NOTIFY SchemeChanged)
public:
    explicit ThemeService(QString runtimeDirectory, QObject *parent = nullptr);
    explicit ThemeService(QString runtimeDirectory, bool loadSettings, QObject *parent = nullptr);
    QVariantMap schemeMap() const;
    bool isDark() const;
    QString variantName() const;
    double contrastLevel() const;
    QString accentMode() const;
    uint accentColor() const;
    QString mode() const;
    QString outputDirectory() const;
    bool writeArtifacts();
    void setOptions(m3::Variant variant, double contrast, uint fixedAccent, bool wallpaper);

public slots:
    void SetMode(const QString &mode);
    void SetVariant(const QString &variant);
    void SetContrast(double level);
    void SetAccent(const QVariantMap &spec);
    void SetWallpaper(const QString &path, const QString &fit);
    void Preview(const QVariantMap &scheme);
    void Restore();
    void Reload();

signals:
    void SchemeChanged(const QVariantMap &scheme, const QString &themePath);
    void WallpaperChanged(const QString &uri, const QVariantMap &metadata);

private:
    void generate();
    void applySeed(uint32_t seed, bool persist = true);
    static QString variantName(m3::Variant variant);
    static m3::Variant parseVariant(const QString &name);
public:
    static m3::Variant variantFromName(const QString &name) { return parseVariant(name); }
private:
    QString m_directory;
    Scheme m_light;
    Scheme m_dark;
    bool m_darkMode = false;
    bool m_auto = false;
    bool m_wallpaperAccent = true;
    uint32_t m_fixedAccent = 0xff6750a4;
    uint32_t m_wallpaperAccentColor = m_fixedAccent;
    QString m_wallpaper;
    QString m_fit = QStringLiteral("crop");
    m3::Variant m_variant = m3::Variant::kTonalSpot;
    double m_contrast = 0.0;
};

QVariantMap generateArtifacts(const Scheme &light, const Scheme &dark,
                              const QString &directory, QString *error = nullptr);
Scheme schemeFor(uint32_t seed, bool dark, m3::Variant variant, double contrast);
m3::Variant variantFromName(const QString &name);
}
