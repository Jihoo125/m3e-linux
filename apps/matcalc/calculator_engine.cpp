#include "calculator_engine.hpp"

#include <QChar>
#include <QRegularExpression>
#include <QtMath>

namespace mat {

CalculatorEngine::CalculatorEngine(QObject *parent) : QObject(parent) {}

void CalculatorEngine::setExpression(const QString &expression)
{
    if (expression == m_expression) return;
    m_expression = expression;
    m_error.clear();
    emit expressionChanged();
    emit errorChanged();
}

void CalculatorEngine::input(const QString &token)
{
    if (m_evaluated) {
        if (token == QStringLiteral("+") || token == QStringLiteral("-") || token == QStringLiteral("×") || token == QStringLiteral("÷")) {
            m_expression = m_result;
        } else {
            m_expression.clear();
        }
        m_evaluated = false;
    }
    if (token == QStringLiteral(".")) {
        const QString tail = m_expression.section(QRegularExpression(QStringLiteral("[+×÷\\-]")), -1);
        if (tail.contains(QLatin1Char('.'))) return;
        if (tail.isEmpty()) m_expression += QLatin1Char('0');
    }
    if (token == QStringLiteral("0") && m_expression.isEmpty()) return;
    m_expression += token;
    m_error.clear();
    emit expressionChanged();
    emit errorChanged();
}

void CalculatorEngine::clear()
{
    m_expression.clear();
    m_result = QStringLiteral("0");
    m_error.clear();
    m_evaluated = false;
    emit expressionChanged();
    emit resultChanged();
    emit errorChanged();
}

void CalculatorEngine::backspace()
{
    m_expression.chop(1);
    m_evaluated = false;
    emit expressionChanged();
}

void CalculatorEngine::percent()
{
    bool ok = false;
    const QString value = evaluateExpression(m_expression, &ok);
    if (ok) m_result = QString::number(value.toDouble() / 100.0, 'g', 15);
    m_expression = m_result;
    m_evaluated = true;
    emit resultChanged();
    emit expressionChanged();
}

QString CalculatorEngine::evaluateExpression(const QString &expression, bool *ok) const
{
    // A deliberately small arithmetic grammar: + - * / % ^, parentheses and
    // unary minus. No eval(), arbitrary names, assignment or JS execution.
    struct Parser {
        const QString &text;
        int index = 0;
        bool valid = true;
        void spaces() { while (index < text.size() && text[index].isSpace()) ++index; }
        double expression() {
            double value = term();
            for (;;) {
                spaces();
                if (index >= text.size()) return value;
                const QChar op = text[index++];
                if (op != QLatin1Char('+') && op != QLatin1Char('-')) { --index; return value; }
                const double rhs = term();
                if (op == QLatin1Char('+')) value += rhs;
                else value -= rhs;
            }
        }
        double term() {
            double value = power();
            for (;;) {
                spaces();
                if (index >= text.size()) return value;
                const QChar op = text[index++];
                if (op != QLatin1Char('*') && op != QLatin1Char('/') && op != QLatin1Char('%')) { --index; return value; }
                const double rhs = power();
                if (op == QLatin1Char('*')) value *= rhs;
                else if (op == QLatin1Char('/')) { if (qFuzzyIsNull(rhs)) { valid = false; return 0; } value /= rhs; }
                else value = std::fmod(value, rhs);
            }
        }
        double power() {
            double value = unary();
            spaces();
            if (index < text.size() && text[index] == QLatin1Char('^')) {
                ++index;
                const double rhs = power();
                return qPow(value, rhs);
            }
            return value;
        }
        double unary() {
            spaces();
            if (index < text.size() && text[index] == QLatin1Char('-')) { ++index; return -unary(); }
            if (index < text.size() && text[index] == QLatin1Char('+')) { ++index; return unary(); }
            return primary();
        }
        double primary() {
            spaces();
            if (index >= text.size()) { valid = false; return 0; }
            if (text[index] == QLatin1Char('(')) {
                ++index;
                const double value = expression();
                spaces();
                if (index >= text.size() || text[index] != QLatin1Char(')')) { valid = false; return 0; }
                ++index;
                return value;
            }
            int start = index;
            while (index < text.size() && (text[index].isDigit() || text[index] == QLatin1Char('.'))) ++index;
            if (start == index) { valid = false; return 0; }
            bool ok = false;
            const double result = text.mid(start, index - start).toDouble(&ok);
            if (!ok) valid = false;
            return result;
        }
    } parser{expression};
    const double value = parser.expression();
    parser.spaces();
    if (!parser.valid || parser.index != expression.size() || !qIsFinite(value)) {
        if (ok) *ok = false;
        return {};
    }
    if (ok) *ok = true;
    return QString::number(value, 'g', 15);
}

void CalculatorEngine::evaluate()
{
    if (m_expression.isEmpty()) return;
    QString normalized = m_expression;
    normalized.replace(QStringLiteral("×"), QStringLiteral("*"));
    normalized.replace(QStringLiteral("÷"), QStringLiteral("/"));
    bool ok = false;
    const QString result = evaluateExpression(normalized, &ok);
    if (!ok) {
        m_error = QStringLiteral("Check your expression");
        emit errorChanged();
        return;
    }
    m_result = result;
    m_history.prepend(m_expression + QStringLiteral(" = ") + result);
    if (m_history.size() > 50) m_history.removeLast();
    m_expression = result;
    m_evaluated = true;
    m_error.clear();
    emit expressionChanged();
    emit resultChanged();
    emit historyChanged();
    emit errorChanged();
}

} // namespace mat
