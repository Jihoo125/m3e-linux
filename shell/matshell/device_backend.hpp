#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace mat {
class DeviceBackend final : public QObject {
    Q_OBJECT
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(QString backendName READ backendName CONSTANT)
public:
    explicit DeviceBackend(QObject *parent = nullptr);
    int volume() const { return m_volume; }
    void setVolume(int value);
    bool muted() const { return m_muted; }
    void setMuted(bool value);
    int brightness() const { return m_brightness; }
    void setBrightness(int value);
    QString backendName() const { return m_backend; }
signals:
    void volumeChanged();
    void mutedChanged();
    void brightnessChanged();
    void error(const QString &message);
private:
    int readPercent(const QString &path) const;
    bool writePercent(const QString &path, int value);
    int m_volume = 65;
    bool m_muted = false;
    int m_brightness = 80;
    int m_maxBrightness = 100;
    QString m_backlightPath;
    QString m_backend;
};
}
