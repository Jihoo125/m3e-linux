#include "terminal_emulator.hpp"

#include <QChar>

namespace mat {

TerminalEmulator::TerminalEmulator(QObject *parent) : QObject(parent)
{
    m_screen.resize(m_rows);
    for (auto &line : m_screen) line.resize(m_cols);
}

QStringList TerminalEmulator::lines() const
{
    QStringList out;
    out.reserve(m_screen.size());
    for (const auto &line : m_screen) out.append(lineHtml(line));
    return out;
}

QString TerminalEmulator::plainText() const
{
    QString out;
    for (const auto &line : m_screen) {
        for (const auto &cell : line) out.append(cell.ch);
        out.append(QLatin1Char('\n'));
    }
    return out;
}

QString TerminalEmulator::lineHtml(const Line &line) const
{
    static const char *palette[] = {"#000000", "#cd3131", "#0dbc79", "#e5e510", "#2472c8", "#bc3fbc", "#11a8cd", "#e5e5e5",
                                    "#666666", "#f14c4c", "#23d18b", "#f5f543", "#3b8eea", "#d670d6", "#29b8db", "#ffffff"};
    QString html;
    int runFg = -2, runBg = -2;
    bool runBold = false;
    bool open = false;
    for (const auto &cell : line) {
        if (cell.fg != runFg || cell.bg != runBg || cell.bold != runBold) {
            if (open) html += QLatin1String("</span>");
            QString style;
            const int fg = cell.fg >= 0 ? cell.fg : 7;
            style += QStringLiteral("color:%1;").arg(QLatin1String(palette[fg & 15]));
            if (cell.bg >= 0) style += QStringLiteral("background:%1;").arg(QLatin1String(palette[cell.bg & 15]));
            if (cell.bold) style += QLatin1String("font-weight:bold;");
            html += QStringLiteral("<span style=\"%1\">").arg(style);
            open = true;
            runFg = cell.fg; runBg = cell.bg; runBold = cell.bold;
        }
        const QChar c = cell.ch;
        if (c == QLatin1Char('&')) html += QLatin1String("&amp;");
        else if (c == QLatin1Char('<')) html += QLatin1String("&lt;");
        else if (c == QLatin1Char('>')) html += QLatin1String("&gt;");
        else html.append(c);
    }
    if (open) html += QLatin1String("</span>");
    return html;
}

void TerminalEmulator::resize(int cols, int rows)
{
    if (cols < 2 || rows < 2 || (cols == m_cols && rows == m_rows)) return;
    m_cols = cols; m_rows = rows;
    m_screen.resize(rows);
    for (auto &line : m_screen) line.resize(cols);
    m_x = qMin(m_x, cols - 1);
    m_y = qMin(m_y, rows - 1);
    emit changed();
}

void TerminalEmulator::scrollUp()
{
    m_scrollback.append(m_screen.takeFirst());
    if (m_scrollback.size() > 1000) m_scrollback.removeFirst();
    Line blank;
    blank.resize(m_cols);
    m_screen.append(blank);
}

void TerminalEmulator::newline()
{
    if (m_y + 1 >= m_rows) scrollUp();
    else ++m_y;
}

void TerminalEmulator::put(QChar c)
{
    if (m_x >= m_cols) { m_x = 0; newline(); }
    Cell &cell = m_screen[m_y][m_x];
    cell.ch = c; cell.fg = m_fg; cell.bg = m_bg; cell.bold = m_bold;
    ++m_x;
}

void TerminalEmulator::eraseLine(int mode)
{
    auto &line = m_screen[m_y];
    const int from = mode == 1 ? 0 : m_x;
    const int to = mode == 0 ? m_cols : (mode == 1 ? m_x + 1 : m_cols);
    for (int i = from; i < to && i < m_cols; ++i) line[i] = Cell{};
}

void TerminalEmulator::eraseDisplay(int mode)
{
    if (mode == 2 || mode == 3) {
        for (auto &line : m_screen) line.fill(Cell{});
    } else if (mode == 0) {
        eraseLine(0);
        for (int y = m_y + 1; y < m_rows; ++y) m_screen[y].fill(Cell{});
    } else {
        eraseLine(1);
        for (int y = 0; y < m_y; ++y) m_screen[y].fill(Cell{});
    }
}

void TerminalEmulator::sgr(const QVector<int> &params)
{
    if (params.isEmpty()) { m_fg = m_bg = -1; m_bold = false; return; }
    for (int i = 0; i < params.size(); ++i) {
        const int p = params[i];
        if (p == 0) { m_fg = m_bg = -1; m_bold = false; }
        else if (p == 1) m_bold = true;
        else if (p == 22) m_bold = false;
        else if (p >= 30 && p <= 37) m_fg = p - 30;
        else if (p == 39) m_fg = -1;
        else if (p >= 40 && p <= 47) m_bg = p - 40;
        else if (p == 49) m_bg = -1;
        else if (p >= 90 && p <= 97) m_fg = p - 90 + 8;
        else if (p >= 100 && p <= 107) m_bg = p - 100 + 8;
        else if (p == 38 || p == 48) {
            // Extended colour: 5;n (256) or 2;r;g;b (truecolor -> nearest base).
            if (i + 1 < params.size() && params[i + 1] == 5 && i + 2 < params.size()) {
                const int idx = params[i + 2] & 15;
                if (p == 38) m_fg = idx; else m_bg = idx;
                i += 2;
            } else if (i + 1 < params.size() && params[i + 1] == 2 && i + 4 < params.size()) {
                i += 4; // truecolor accepted; base palette approximation
            }
        }
    }
}

void TerminalEmulator::csi(QChar final, const QVector<int> &params)
{
    const int p0 = params.isEmpty() ? 0 : params[0];
    const int p1 = params.size() > 1 ? params[1] : 0;
    switch (final.unicode()) {
    case 'A': m_y = qMax(0, m_y - qMax(1, p0)); break;
    case 'B': m_y = qMin(m_rows - 1, m_y + qMax(1, p0)); break;
    case 'C': m_x = qMin(m_cols - 1, m_x + qMax(1, p0)); break;
    case 'D': m_x = qMax(0, m_x - qMax(1, p0)); break;
    case 'H': case 'f':
        m_y = qBound(0, (p0 ? p0 : 1) - 1, m_rows - 1);
        m_x = qBound(0, (p1 ? p1 : 1) - 1, m_cols - 1);
        break;
    case 'J': eraseDisplay(p0); break;
    case 'K': eraseLine(p0); break;
    case 'm': sgr(params); break;
    case 'G': m_x = qBound(0, (p0 ? p0 : 1) - 1, m_cols - 1); break;
    case 'd': m_y = qBound(0, (p0 ? p0 : 1) - 1, m_rows - 1); break;
    default: break; // unsupported modes are ignored, never crash
    }
}

void TerminalEmulator::feed(const QByteArray &data)
{
    const QString text = QString::fromUtf8(data);
    for (const QChar c : text) {
        const ushort u = c.unicode();
        if (m_state == 3) { // OSC: skip until BEL or ST
            if (u == 0x07) m_state = 0;
            else if (u == 0x1b) m_state = 4;
            continue;
        }
        if (m_state == 4) { m_state = (u == '\\') ? 0 : 3; continue; }
        if (m_state == 1) {
            if (c == QLatin1Char('[')) { m_state = 2; m_params.clear(); m_currentParam = -1; }
            else if (c == QLatin1Char(']')) m_state = 3;
            else if (c == QLatin1Char('(') || c == QLatin1Char(')')) m_state = 5;
            else m_state = 0;
            continue;
        }
        if (m_state == 5) { m_state = 0; continue; }
        if (m_state == 2) {
            if (c.isDigit()) { m_currentParam = (m_currentParam < 0 ? 0 : m_currentParam) * 10 + c.digitValue(); }
            else if (c == QLatin1Char(';')) { m_params.append(m_currentParam < 0 ? 0 : m_currentParam); m_currentParam = -1; }
            else if (c == QLatin1Char('?') || c == QLatin1Char('>')) { /* private prefix */ }
            else {
                m_params.append(m_currentParam < 0 ? 0 : m_currentParam);
                csi(c, m_params);
                m_state = 0;
            }
            continue;
        }
        // ground
        if (u == 0x1b) { m_state = 1; continue; }
        if (c == QLatin1Char('\n')) { newline(); continue; }
        if (c == QLatin1Char('\r')) { m_x = 0; continue; }
        if (c == QLatin1Char('\b')) { m_x = qMax(0, m_x - 1); continue; }
        if (c == QLatin1Char('\t')) { m_x = qMin(m_cols - 1, ((m_x / 8) + 1) * 8); continue; }
        if (u == 0x07) continue;
        if (u < 0x20) continue;
        put(c);
    }
    emit changed();
}

} // namespace mat
