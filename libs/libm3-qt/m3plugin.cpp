#include <QQmlExtensionPlugin>
#include <QQmlEngine>
#include "m3theme.hpp"

class MaterializedPlugin final : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
    void registerTypes(const char *uri) override
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("Materialized"));
        qmlRegisterSingletonType<mat::M3Theme>("Materialized", 1, 0, "M3Theme", &mat::M3Theme::create);
        qmlRegisterModule("Materialized", 1, 0);
    }
};

#include "m3plugin.moc"
