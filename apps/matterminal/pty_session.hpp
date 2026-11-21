#pragma once

#include <QObject>
#include <QSocketNotifier>
#include <sys/types.h>

namespace mat {
class TerminalEmulator;

/// A real pseudo-terminal running the user's shell. Not a simulation: bytes
/// flow through forkpty(3) exactly like any terminal emulator.
class PtySession final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
public:
    explicit PtySession(TerminalEmulator *emulator, QObject *parent = nullptr);
    ~PtySession() override;
    bool running() const { return m_fd >= 0; }
    Q_INVOKABLE bool start(int cols, int rows);
    Q_INVOKABLE void send(const QByteArray &data);
    Q_INVOKABLE void resize(int cols, int rows);
    Q_INVOKABLE void stop();
signals:
    void runningChanged();
    void finished(int exitCode);
private:
    void onReadable();
    TerminalEmulator *m_emulator;
    int m_fd = -1;
    pid_t m_pid = -1;
    QSocketNotifier *m_notifier = nullptr;
};
} // namespace mat
