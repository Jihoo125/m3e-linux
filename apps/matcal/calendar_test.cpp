#include <QCoreApplication>
#include <QDebug>
#include <QStandardPaths>
#include "calendar_model.hpp"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setApplicationName(QStringLiteral("matcal-test"));
    {
        mat::CalendarModel calendar;
        const QDate day(2026, 11, 19);
        calendar.setSelected(day);
        calendar.addEvent(QStringLiteral("Ship alpha"), QStringLiteral("09:00"));
        calendar.addEvent(QStringLiteral(""), QString()); // must be rejected
        if (calendar.rowCount() != 1) { qCritical() << "event add failed" << calendar.rowCount(); return 1; }
        if (!calendar.hasEvents(day)) { qCritical() << "hasEvents failed"; return 1; }
        const QVariantList grid = calendar.monthGrid();
        if (grid.size() != 42) { qCritical() << "month grid must be 6 weeks"; return 1; }
    }
    {
        // Persistence: a fresh model must load the saved event.
        mat::CalendarModel calendar;
        calendar.setSelected(QDate(2026, 11, 19));
        if (calendar.rowCount() != 1) { qCritical() << "persistence failed"; return 1; }
        const QVariantList events = calendar.dayEvents();
        if (events[0].toMap().value("title").toString() != QLatin1String("Ship alpha")) { qCritical() << "event content lost"; return 1; }
        calendar.removeEvent(events[0].toMap().value("id").toInt());
        if (calendar.rowCount() != 0) { qCritical() << "remove failed"; return 1; }
    }
    {
        mat::CalendarModel calendar;
        calendar.setSelected(QDate(2026, 11, 19));
        if (calendar.rowCount() != 0) { qCritical() << "remove not persisted"; return 1; }
    }
    qInfo() << "Calendar model tests passed";
    return 0;
}
