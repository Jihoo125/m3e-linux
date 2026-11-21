#pragma once

#include <QObject>
#include <QVariantMap>

namespace mat {
class PowerBackend final : public QObject {
    Q_OBJECT
    Q_PROPERTY(int batteryPercentage READ batteryPercentage NOTIFY batteryChanged)
    Q_PROPERTY(bool charging READ charging NOTIFY batteryChanged)
public:
    explicit PowerBackend(QObject *parent = nullptr);
    int batteryPercentage() const { return m_percentage; }
    bool charging() const { return m_charging; }
signals:
    void batteryChanged();
private:
    void readBattery();
    int m_percentage = 100;
    bool m_charging = false;
};
}
