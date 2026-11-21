#pragma once

#include <QObject>
#include <QStringList>
#include <QVector>

namespace mat {

class CalculatorEngine final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString expression READ expression WRITE setExpression NOTIFY expressionChanged)
    Q_PROPERTY(QString result READ result NOTIFY resultChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QStringList history READ history NOTIFY historyChanged)
public:
    explicit CalculatorEngine(QObject *parent = nullptr);
    QString expression() const { return m_expression; }
    QString result() const { return m_result; }
    QString error() const { return m_error; }
    QStringList history() const { return m_history; }
    void setExpression(const QString &expression);
    Q_INVOKABLE void input(const QString &token);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void backspace();
    Q_INVOKABLE void evaluate();
    Q_INVOKABLE void percent();

signals:
    void expressionChanged();
    void resultChanged();
    void errorChanged();
    void historyChanged();

private:
    QString evaluateExpression(const QString &expression, bool *ok) const;
    QString m_expression;
    QString m_result{QStringLiteral("0")};
    QString m_error;
    QStringList m_history;
    bool m_evaluated = false;
};

} // namespace mat
