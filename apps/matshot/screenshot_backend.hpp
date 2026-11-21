#pragma once

#include <QObject>
#include <QString>

namespace mat {
/// Real screen capture through wlr-screencopy-unstable-v1 against the running
/// compositor. The capture runs on its own Wayland connection in a worker
/// thread; pixels come from the compositor, never from a fake image.
class ScreenshotBackend final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString lastPath READ lastPath NOTIFY captured)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString status READ status NOTIFY captured)
public:
    explicit ScreenshotBackend(QObject *parent = nullptr);
    QString lastPath() const { return m_lastPath; }
    bool busy() const { return m_busy; }
    QString status() const { return m_status; }
    Q_INVOKABLE void capture();
    Q_INVOKABLE void openFolder();
signals:
    void captured(const QString &path, const QString &error);
    void busyChanged();
private:
    void finish(const QString &path, const QString &error);
    QString m_lastPath;
    QString m_status{QStringLiteral("Ready")};
    bool m_busy = false;
};
} // namespace mat
