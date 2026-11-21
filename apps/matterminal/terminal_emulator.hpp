#pragma once

#include <QObject>
#include <QStringList>
#include <QVector>

namespace mat {

struct Cell {
    QChar ch{QLatin1Char(' ')};
    int fg = -1; // -1 = default
    int bg = -1;
    bool bold = false;
};

/// A compact but real VT100/ANSI terminal emulator: cursor addressing, erase,
/// SGR colours, scrollback. Not a mock: it parses the same escape sequences
/// bash, ls --color and vim emit.
class TerminalEmulator final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList lines READ lines NOTIFY changed)
    Q_PROPERTY(int cursorX READ cursorX NOTIFY changed)
    Q_PROPERTY(int cursorY READ cursorY NOTIFY changed)
    Q_PROPERTY(int columns READ columns NOTIFY changed)
    Q_PROPERTY(int rows READ rows NOTIFY changed)
public:
    explicit TerminalEmulator(QObject *parent = nullptr);
    QStringList lines() const;
    int cursorX() const { return m_x; }
    int cursorY() const { return m_y; }
    int columns() const { return m_cols; }
    int rows() const { return m_rows; }
    Q_INVOKABLE void resize(int cols, int rows);
    void feed(const QByteArray &data);
    QString plainText() const;
signals:
    void changed();
private:
    using Line = QVector<Cell>;
    void put(QChar c);
    void newline();
    void scrollUp();
    void csi(QChar final, const QVector<int> &params);
    void sgr(const QVector<int> &params);
    void eraseLine(int mode);
    void eraseDisplay(int mode);
    QString lineHtml(const Line &line) const;
    QVector<Line> m_screen;
    QVector<Line> m_scrollback;
    int m_cols = 80, m_rows = 24;
    int m_x = 0, m_y = 0;
    int m_fg = -1, m_bg = -1;
    bool m_bold = false;
    int m_state = 0; // 0 ground, 1 esc, 2 csi, 3 osc
    QVector<int> m_params;
    int m_currentParam = -1;
};

} // namespace mat
