#include "pty_session.hpp"
#include "terminal_emulator.hpp"

#include <QProcessEnvironment>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <pty.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

namespace mat {

PtySession::PtySession(TerminalEmulator *emulator, QObject *parent)
    : QObject(parent), m_emulator(emulator) {}

PtySession::~PtySession() { stop(); }

bool PtySession::start(int cols, int rows)
{
    if (running()) return true;
    winsize ws{};
    ws.ws_col = static_cast<unsigned short>(qMax(2, cols));
    ws.ws_row = static_cast<unsigned short>(qMax(2, rows));
    m_pid = forkpty(&m_fd, nullptr, nullptr, &ws);
    if (m_pid < 0) return false;
    if (m_pid == 0) {
        // Child: become the shell on the new controlling terminal.
        setenv("TERM", "xterm-256color", 1);
        const QString shell = QProcessEnvironment::systemEnvironment().value(QStringLiteral("SHELL"), QStringLiteral("/bin/bash"));
        execl(shell.toUtf8().constData(), shell.toUtf8().constData(), static_cast<char *>(nullptr));
        execl("/bin/bash", "bash", static_cast<char *>(nullptr));
        _exit(127);
    }
    m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &PtySession::onReadable);
    m_emulator->resize(cols, rows);
    emit runningChanged();
    return true;
}

void PtySession::onReadable()
{
    char buffer[8192];
    const ssize_t n = ::read(m_fd, buffer, sizeof(buffer));
    if (n > 0) {
        m_emulator->feed(QByteArray(buffer, static_cast<int>(n)));
        return;
    }
    // EOF or error: the shell exited.
    int status = 0;
    if (m_pid > 0) waitpid(m_pid, &status, WNOHANG);
    const int code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    stop();
    emit finished(code);
}

void PtySession::send(const QByteArray &data)
{
    if (!running() || data.isEmpty()) return;
    if (qEnvironmentVariableIsSet("MAT_DEBUG_INPUT"))
        fprintf(stderr, "matterminal: pty send %s\n", data.toHex().constData());
    ssize_t offset = 0;
    while (offset < data.size()) {
        const ssize_t n = ::write(m_fd, data.constData() + offset, static_cast<size_t>(data.size() - offset));
        if (n <= 0) {
            if (errno == EINTR) continue;
            break;
        }
        offset += n;
    }
}

void PtySession::resize(int cols, int rows)
{
    m_emulator->resize(cols, rows);
    if (!running()) return;
    winsize ws{};
    ws.ws_col = static_cast<unsigned short>(qMax(2, cols));
    ws.ws_row = static_cast<unsigned short>(qMax(2, rows));
    ::ioctl(m_fd, TIOCSWINSZ, &ws);
    if (m_pid > 0) ::kill(m_pid, SIGWINCH);
}

void PtySession::stop()
{
    if (m_notifier) { m_notifier->setEnabled(false); m_notifier->deleteLater(); m_notifier = nullptr; }
    if (m_fd >= 0) { ::close(m_fd); m_fd = -1; }
    if (m_pid > 0) {
        ::kill(m_pid, SIGHUP);
        waitpid(m_pid, nullptr, WNOHANG);
        m_pid = -1;
    }
    emit runningChanged();
}

} // namespace mat
