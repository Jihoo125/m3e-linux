#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QTemporaryDir>
#include <QFile>
#include <cmath>
#include "m3theme.hpp"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("scheme-light.json"));
    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.write("{\"schemaVersion\":1,\"mode\":\"light\",\"dark\":false,\"variant\":\"expressive\","
               "\"sourceColor\":\"#6750A4\",\"contrast\":0.0,"
               "\"roles\":{\"primary\":\"#7C4DFF\",\"background\":\"#FEF7FF\",\"onSurface\":\"#1D1B20\"}}");
    file.close();
    qputenv("MAT_SCHEME_PATH", path.toUtf8());
    mat::M3Theme theme;
    if (theme.variant() != QLatin1String("expressive")) { qCritical() << "variant not loaded:" << theme.variant(); return 1; }
    if (theme.color("primary") != QColor("#7C4DFF")) { qCritical() << "expressive roles not applied"; return 1; }
    const QVariantMap spatial = theme.spring("spatialDefault");
    const double k = spatial.value("spring").toDouble();
    const double damping = spatial.value("damping").toDouble();
    if (!qFuzzyCompare(k, 700.0) || std::abs(damping - 2.0 * 0.8 * std::sqrt(700.0)) > 1e-6) {
        qCritical() << "spatial spring physics wrong" << k << damping; return 1;
    }
    const QVariantMap effects = theme.spring("effectsDefault");
    if (std::abs(effects.value("damping").toDouble() - 2.0 * std::sqrt(1600.0)) > 1e-6) {
        qCritical() << "effects spring must be critically damped"; return 1;
    }
    if (theme.spring("spatialFast").value("damping").toDouble() >= 2.0 * std::sqrt(1400.0)) {
        qCritical() << "spatial springs must be under-damped (bounce)"; return 1;
    }
    const QFont base = theme.font("headlineMedium");
    const QFont emphasized = theme.fontEmphasized("headlineMedium");
    if (emphasized.weight() <= base.weight()) { qCritical() << "emphasized type must be heavier"; return 1; }
    qInfo() << "M3 Expressive theme tests passed";
    return 0;
}
