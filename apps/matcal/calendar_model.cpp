#include "calendar_model.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace mat {

CalendarModel::CalendarModel(QObject *parent) : QAbstractListModel(parent)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    m_path = dir + QStringLiteral("/events.json");
    load();
}

int CalendarModel::rowCount(const QModelIndex &) const
{
    int count = 0;
    for (const auto &event : m_events)
        if (event.date == m_selected) ++count;
    return count;
}

QVariant CalendarModel::data(const QModelIndex &index, int role) const
{
    int seen = 0;
    for (const auto &event : m_events) {
        if (event.date != m_selected) continue;
        if (seen++ != index.row()) continue;
        switch (role) {
        case TitleRole: return event.title;
        case TimeRole: return event.time;
        case IdRole: return event.id;
        default: return {};
        }
    }
    return {};
}

QHash<int, QByteArray> CalendarModel::roleNames() const
{
    return {{TitleRole, "title"}, {TimeRole, "time"}, {IdRole, "eventId"}};
}

void CalendarModel::setMonth(const QDate &date)
{
    const QDate first(date.year(), date.month(), 1);
    if (first == m_month) return;
    m_month = first;
    emit monthChanged();
}

void CalendarModel::setSelected(const QDate &date)
{
    if (date == m_selected) return;
    beginResetModel();
    m_selected = date;
    endResetModel();
    emit selectedChanged();
    emit eventsChanged();
    if (date.month() != m_month.month() || date.year() != m_month.year())
        setMonth(date);
}

QVariantList CalendarModel::monthGrid() const
{
    QVariantList cells;
    const int lead = (m_month.dayOfWeek() - 1 + 7) % 7; // Monday-first
    const QDate start = m_month.addDays(-lead);
    for (int i = 0; i < 42; ++i) {
        const QDate date = start.addDays(i);
        cells.append(QVariantMap{
            {QStringLiteral("day"), date.day()},
            {QStringLiteral("date"), date.toString(Qt::ISODate)},
            {QStringLiteral("inMonth"), date.month() == m_month.month()},
            {QStringLiteral("today"), date == QDate::currentDate()},
            {QStringLiteral("selected"), date == m_selected},
            {QStringLiteral("hasEvents"), hasEvents(date)},
        });
    }
    return cells;
}

QVariantList CalendarModel::dayEvents() const
{
    QVariantList out;
    for (const auto &event : m_events)
        if (event.date == m_selected)
            out.append(QVariantMap{{QStringLiteral("id"), event.id}, {QStringLiteral("title"), event.title}, {QStringLiteral("time"), event.time}});
    return out;
}

bool CalendarModel::hasEvents(const QDate &date) const
{
    for (const auto &event : m_events)
        if (event.date == date) return true;
    return false;
}

void CalendarModel::addEvent(const QString &title, const QString &time)
{
    if (title.trimmed().isEmpty()) { emit error(QStringLiteral("Event needs a title")); return; }
    beginResetModel();
    m_events.append(Event{m_nextId++, m_selected, title.trimmed(), time});
    endResetModel();
    save();
    emit eventsChanged();
    emit monthChanged();
}

void CalendarModel::removeEvent(int id)
{
    beginResetModel();
    m_events.erase(std::remove_if(m_events.begin(), m_events.end(), [id](const Event &e) { return e.id == id; }), m_events.end());
    endResetModel();
    save();
    emit eventsChanged();
    emit monthChanged();
}

void CalendarModel::previousMonth() { setMonth(m_month.addMonths(-1)); }
void CalendarModel::nextMonth() { setMonth(m_month.addMonths(1)); }

void CalendarModel::load()
{
    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly)) return;
    const auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) return;
    for (const auto &value : doc.array()) {
        const auto obj = value.toObject();
        const QDate date = QDate::fromString(obj.value(QStringLiteral("date")).toString(), Qt::ISODate);
        if (!date.isValid()) continue;
        const int id = obj.value(QStringLiteral("id")).toInt(m_nextId);
        m_events.append(Event{id, date, obj.value(QStringLiteral("title")).toString(), obj.value(QStringLiteral("time")).toString()});
        m_nextId = qMax(m_nextId, id + 1);
    }
}

void CalendarModel::save()
{
    QJsonArray array;
    for (const auto &event : m_events) {
        array.append(QJsonObject{
            {QStringLiteral("id"), event.id},
            {QStringLiteral("date"), event.date.toString(Qt::ISODate)},
            {QStringLiteral("title"), event.title},
            {QStringLiteral("time"), event.time},
        });
    }
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly)) { emit error(file.errorString()); return; }
    file.write(QJsonDocument(array).toJson());
    if (!file.commit()) emit error(file.errorString());
}

} // namespace mat
