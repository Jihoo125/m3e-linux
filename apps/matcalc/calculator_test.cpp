#include <QCoreApplication>
#include <QDebug>
#include "calculator_engine.hpp"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    mat::CalculatorEngine calculator;
    for (const QString &token : {QStringLiteral("2"), QStringLiteral("+"), QStringLiteral("3"), QStringLiteral("×"), QStringLiteral("4")})
        calculator.input(token);
    calculator.evaluate();
    if (calculator.result() != QLatin1String("14")) { qCritical() << "precedence failed" << calculator.result(); return 1; }
    calculator.clear();
    calculator.setExpression(QStringLiteral("10÷4"));
    calculator.evaluate();
    if (calculator.result() != QLatin1String("2.5")) { qCritical() << "division failed" << calculator.result(); return 1; }
    calculator.setExpression(QStringLiteral("2^10"));
    calculator.evaluate();
    if (calculator.result() != QLatin1String("1024")) { qCritical() << "power failed" << calculator.result(); return 1; }
    calculator.setExpression(QStringLiteral("1÷0"));
    calculator.evaluate();
    if (calculator.error().isEmpty()) { qCritical() << "divide by zero accepted"; return 1; }
    calculator.setExpression(QStringLiteral("notAFunction(2)"));
    calculator.evaluate();
    if (calculator.error().isEmpty()) { qCritical() << "unsafe expression accepted"; return 1; }
    qInfo() << "Calculator parser tests passed";
    return 0;
}
