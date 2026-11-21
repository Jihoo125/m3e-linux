#include <QCoreApplication>
#include <QDebug>
#include "terminal_emulator.hpp"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    mat::TerminalEmulator term;
    term.resize(20, 5);
    term.feed("hello\r\nworld");
    // Lines are padded to the terminal width, as on a real VT.
    const QString first = term.lines()[0];
    if (!term.plainText().section(QLatin1Char('\n'), 0, 0).trimmed().startsWith("hello")) { qCritical() << "basic output failed" << term.plainText(); return 1; }
    if (term.plainText().section(QLatin1Char('\n'), 1, 1).trimmed() != QLatin1String("world")) { qCritical() << "second line failed"; return 1; }
    if (term.cursorX() != 5 || term.cursorY() != 1) { qCritical() << "cursor tracking failed"; return 1; }
    term.feed("\x1b[2J\x1b[H");
    if (term.cursorX() != 0 || term.cursorY() != 0) { qCritical() << "CSI H failed"; return 1; }
    if (!term.plainText().trimmed().isEmpty()) { qCritical() << "erase display failed"; return 1; }
    term.feed("\x1b[31mred\x1b[0m plain");
    const QStringList lines = term.lines();
    if (!lines[0].contains("#cd3131") || !lines[0].contains("red")) { qCritical() << "SGR colour failed" << lines[0]; return 1; }
    if (lines[0].contains("color:#cd3131;\"> plain")) { qCritical() << "SGR reset failed"; return 1; }
    term.feed("\x1b[1;32mboldgreen");
    if (!term.lines()[0].contains("font-weight:bold")) { qCritical() << "bold failed"; return 1; }
    // Cursor addressing + erase line
    term.feed("\x1b[2J\x1b[3;5HXY\x1b[1K");
    if (term.cursorX() != 6 || term.cursorY() != 2) { qCritical() << "addressing failed"; return 1; }
    // Scroll: fill past the last row
    term.feed("\x1b[2J");
    for (int i = 0; i < 8; ++i) term.feed(QStringLiteral("line%1\r\n").arg(i).toUtf8());
    if (!term.plainText().contains("line7")) { qCritical() << "scroll failed"; return 1; }
    // OSC sequences must be swallowed, not printed
    term.feed("\x1b]0;title\x07ok");
    if (!term.plainText().contains("ok") || term.plainText().contains("title")) { qCritical() << "OSC handling failed"; return 1; }
    qInfo() << "Terminal emulator tests passed";
    return 0;
}
