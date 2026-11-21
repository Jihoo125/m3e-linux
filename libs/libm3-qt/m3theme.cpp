#include "m3theme.hpp"

#include <QDebug>
#include <cmath>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace mat {
namespace {

const QHash<QString, QString> &lightDefaults()
{
    // Material baseline scheme. This is a startup fallback only: matsessiond
    // supplies a wallpaper-derived scheme during a real session.
    static const QHash<QString, QString> roles = {
        {"primary", "#6750A4"}, {"onPrimary", "#FFFFFF"},
        {"primaryContainer", "#EADDFF"}, {"onPrimaryContainer", "#21005D"},
        {"secondary", "#625B71"}, {"onSecondary", "#FFFFFF"},
        {"secondaryContainer", "#E8DEF8"}, {"onSecondaryContainer", "#1D192B"},
        {"tertiary", "#7D5260"}, {"onTertiary", "#FFFFFF"},
        {"tertiaryContainer", "#FFD8E4"}, {"onTertiaryContainer", "#31111D"},
        {"error", "#B3261E"}, {"onError", "#FFFFFF"},
        {"errorContainer", "#F9DEDC"}, {"onErrorContainer", "#410E0B"},
        {"background", "#FEF7FF"}, {"onBackground", "#1D1B20"},
        {"surface", "#FEF7FF"}, {"onSurface", "#1D1B20"},
        {"surfaceVariant", "#E7E0EC"}, {"onSurfaceVariant", "#49454F"},
        {"surfaceDim", "#DED8E1"}, {"surfaceBright", "#FEF7FF"},
        {"surfaceContainerLowest", "#FFFFFF"}, {"surfaceContainerLow", "#F7F2FA"},
        {"surfaceContainer", "#F3EDF7"}, {"surfaceContainerHigh", "#ECE6F0"},
        {"surfaceContainerHighest", "#E6E0E9"},
        {"outline", "#79747E"}, {"outlineVariant", "#CAC4D0"},
        {"shadow", "#000000"}, {"scrim", "#000000"},
        {"inverseSurface", "#322F35"}, {"inverseOnSurface", "#F5EFF7"},
        {"inversePrimary", "#D0BCFF"},
        {"primaryFixed", "#EADDFF"}, {"onPrimaryFixed", "#21005D"},
        {"primaryFixedDim", "#D0BCFF"}, {"onPrimaryFixedVariant", "#4F378B"},
        {"secondaryFixed", "#E8DEF8"}, {"onSecondaryFixed", "#1D192B"},
        {"secondaryFixedDim", "#CCC2DC"}, {"onSecondaryFixedVariant", "#4A4458"},
        {"tertiaryFixed", "#FFD8E4"}, {"onTertiaryFixed", "#31111D"},
        {"tertiaryFixedDim", "#EFB8C8"}, {"onTertiaryFixedVariant", "#633B48"},
        {"surfaceTint", "#6750A4"}
    };
    return roles;
}

QColor parseColor(const QJsonValue &value, const QColor &fallback)
{
    const QString text = value.toString();
    if (text.isEmpty())
        return fallback;
    QColor color(text);
    return color.isValid() ? color : fallback;
}

} // namespace

M3Theme::M3Theme(QObject *parent)
    : QObject(parent)
{
    m_reloadTimer.setSingleShot(true);
    m_reloadTimer.setInterval(30);
    connect(&m_reloadTimer, &QTimer::timeout, this, &M3Theme::loadScheme);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &) {
        // Atomic rename removes the watched inode. loadScheme() re-adds the path.
        m_reloadTimer.start();
    });
    installDefaults();
    watchScheme();
}

M3Theme *M3Theme::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    QJSEngine::setObjectOwnership(qmlEngine, QJSEngine::CppOwnership);
    return new M3Theme(qmlEngine);
}

QString M3Theme::defaultSchemePath()
{
    const auto env = QProcessEnvironment::systemEnvironment();
    const QString overridePath = env.value(QStringLiteral("MAT_SCHEME_PATH"));
    if (!overridePath.isEmpty())
        return overridePath;

    const QString mode = env.value(QStringLiteral("MAT_FORCE_DARK")) == QLatin1String("1")
            ? QStringLiteral("dark") : QStringLiteral("light");
    QString runtime = env.value(QStringLiteral("XDG_RUNTIME_DIR"));
    if (runtime.isEmpty())
        runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    return QDir(runtime).filePath(QStringLiteral("materialized/theme/scheme-%1.json").arg(mode));
}

void M3Theme::installDefaults()
{
    const auto &defaults = lightDefaults();
    for (auto it = defaults.cbegin(); it != defaults.cend(); ++it)
        m_colors.insert(it.key(), QColor(it.value()));
    m_colors.insert(QStringLiteral("transparent"), QColor(Qt::transparent));
}

void M3Theme::watchScheme()
{
    const QString next = defaultSchemePath();
    if (next != m_schemePath) {
        m_schemePath = next;
        emit schemePathChanged();
    }
    loadScheme();
}

QColor M3Theme::color(const QString &role) const
{
    const QVariant value = m_colors.value(role);
    if (value.canConvert<QColor>())
        return value.value<QColor>();
    qWarning() << "Unknown Material role" << role;
    // Magenta is deliberately conspicuous during development: a missing role
    // must never silently become black text or an invisible control.
    return QColor(QStringLiteral("#FF00FF"));
}

QFont M3Theme::font(const QString &scale) const
{
    struct Type { int px; int weight; qreal spacing; };
    static const QHash<QString, Type> types = {
        {"displayLarge", {57, QFont::Normal, -0.25}},
        {"displayMedium", {45, QFont::Normal, 0.0}},
        {"displaySmall", {36, QFont::Normal, 0.0}},
        {"headlineLarge", {32, QFont::Normal, 0.0}},
        {"headlineMedium", {28, QFont::Normal, 0.0}},
        {"headlineSmall", {24, QFont::Normal, 0.0}},
        {"titleLarge", {22, QFont::Normal, 0.0}},
        {"titleMedium", {16, QFont::Medium, 0.15}},
        {"titleSmall", {14, QFont::Medium, 0.1}},
        {"bodyLarge", {16, QFont::Normal, 0.5}},
        {"bodyMedium", {14, QFont::Normal, 0.25}},
        {"bodySmall", {12, QFont::Normal, 0.4}},
        {"labelLarge", {14, QFont::Medium, 0.1}},
        {"labelMedium", {12, QFont::Medium, 0.5}},
        {"labelSmall", {11, QFont::Medium, 0.5}}
    };
    const Type type = types.value(scale, types.value(QStringLiteral("bodyMedium")));
    QFont result(QStringLiteral("Inter"));
    result.setStyleHint(QFont::SansSerif);
    result.setPixelSize(type.px);
    result.setWeight(static_cast<QFont::Weight>(type.weight));
    result.setLetterSpacing(QFont::AbsoluteSpacing, type.spacing);
    return result;
}

int M3Theme::radius(const QString &shape) const
{
    static const QHash<QString, int> values = {
        {"none", 0}, {"extraSmall", 4}, {"small", 8}, {"medium", 12},
        {"large", 16}, {"extraLarge", 28}, {"full", 9999}
    };
    return values.value(shape, values.value(QStringLiteral("medium")));
}

int M3Theme::duration(const QString &name) const
{
    static const QHash<QString, int> values = {
        {"short1", 50}, {"short2", 100}, {"short3", 150}, {"short4", 200},
        {"medium1", 250}, {"medium2", 300}, {"medium3", 350}, {"medium4", 400},
        {"long1", 450}, {"long2", 500}, {"long3", 550}, {"long4", 600},
        {"extraLong1", 700}, {"extraLong2", 800}, {"extraLong3", 900}, {"extraLong4", 1000}
    };
    return values.value(name, 200);
}

QVariantList M3Theme::easing(const QString &name) const
{
    if (name == QLatin1String("emphasized") || name == QLatin1String("emphasizedAccelerate"))
        return {0.3, 0.0, 0.8, 0.15};
    if (name == QLatin1String("emphasizedDecelerate"))
        return {0.05, 0.7, 0.1, 1.0};
    if (name == QLatin1String("standardDecelerate"))
        return {0.0, 0.0, 0.0, 1.0};
    if (name == QLatin1String("standardAccelerate"))
        return {0.3, 0.0, 1.0, 1.0};
    return {0.2, 0.0, 0.0, 1.0}; // standard
}

int M3Theme::spacing(const QString &name) const
{
    static const QHash<QString, int> values = {
        {"none", 0}, {"xs", 4}, {"sm", 8}, {"md", 12}, {"lg", 16},
        {"xl", 24}, {"xxl", 32}, {"xxxl", 48}
    };
    return values.value(name, 12);
}

QVariantMap M3Theme::spring(const QString &name) const
{
    // Material 3 Expressive motion scheme (public motion-physics spec):
    // damped harmonic oscillator with unit mass. Spatial motion uses
    // under-damped springs (visible bounce); effects (colour/opacity) use
    // critically damped springs (no overshoot). damping = 2*zeta*sqrt(k).
    struct Spring { double k; double zeta; };
    static const QHash<QString, Spring> springs = {
        {"spatialFast", {1400.0, 0.6}},
        {"spatialDefault", {700.0, 0.8}},
        {"spatialSlow", {300.0, 0.8}},
        {"effectsFast", {3800.0, 1.0}},
        {"effectsDefault", {1600.0, 1.0}},
        {"effectsSlow", {800.0, 1.0}},
    };
    const Spring s = springs.value(name, springs.value(QStringLiteral("spatialDefault")));
    return {
        {QStringLiteral("spring"), s.k},
        {QStringLiteral("damping"), 2.0 * s.zeta * std::sqrt(s.k)},
        {QStringLiteral("epsilon"), 0.01},
    };
}

QFont M3Theme::fontEmphasized(const QString &scale) const
{
    // Expressive typography: the emphasized styles render heavier than the
    // baseline styles. Inter Variable is packaged in the live image; QFont
    // weight interpolation applies when the variable font is resolved.
    QFont f = font(scale);
    const bool display = scale.startsWith(QLatin1String("display")) || scale.startsWith(QLatin1String("headline"));
    f.setWeight(display ? QFont::Bold : QFont::DemiBold);
    return f;
}

bool M3Theme::reload()
{
    loadScheme();
    return true;
}

void M3Theme::loadScheme()
{
    if (m_loading)
        return;
    m_loading = true;

    if (!m_watcher.files().isEmpty())
        m_watcher.removePaths(m_watcher.files());
    if (QFileInfo::exists(m_schemePath))
        m_watcher.addPath(m_schemePath);
    const QString parent = QFileInfo(m_schemePath).absolutePath();
    if (!QFileInfo::exists(m_schemePath) && QDir(parent).exists())
        m_watcher.addPath(parent);

    QFile file(m_schemePath);
    if (!file.exists()) {
        if (m_revision == 0)
            qInfo() << "M3Theme: using baseline palette until" << m_schemePath << "appears";
        m_loading = false;
        return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        emit loadError(file.errorString());
        m_loading = false;
        return;
    }
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        emit loadError(QStringLiteral("Invalid scheme %1: %2").arg(m_schemePath, error.errorString()));
        m_loading = false;
        return;
    }

    const QJsonObject root = document.object();
    const QJsonObject roles = root.value(QStringLiteral("roles")).toObject();
    if (roles.isEmpty()) {
        emit loadError(QStringLiteral("Scheme contains no roles: %1").arg(m_schemePath));
        m_loading = false;
        return;
    }

    // Validate every value before changing a single live binding.
    QHash<QString, QColor> resolved;
    for (auto it = roles.begin(); it != roles.end(); ++it) {
        const QColor fallback = m_colors.value(it.key()).value<QColor>();
        const QColor parsed = parseColor(it.value(), fallback);
        if (!parsed.isValid()) {
            emit loadError(QStringLiteral("Invalid colour for role %1").arg(it.key()));
            m_loading = false;
            return;
        }
        resolved.insert(it.key(), parsed);
    }
    for (auto it = resolved.cbegin(); it != resolved.cend(); ++it)
        m_colors.insert(it.key(), it.value());

    m_dark = root.value(QStringLiteral("dark")).toBool(root.value(QStringLiteral("mode")).toString() == QLatin1String("dark"));
    m_mode = m_dark ? QStringLiteral("dark") : QStringLiteral("light");
    m_sourceColor = root.value(QStringLiteral("sourceColor")).toString();
    m_contrast = root.value(QStringLiteral("contrast")).toDouble(0.0);
    m_variant = root.value(QStringLiteral("variant")).toString(QStringLiteral("tonal-spot"));
    ++m_revision;
    emit schemeChanged();
    m_loading = false;
}

} // namespace mat
