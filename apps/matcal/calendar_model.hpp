#pragma once

#include <QAbstractListModel>
#include <QDate>
#include <QVariantList>

namespace mat {
/// Real calendar events persisted as JSON under XDG data home. No simulated
/// entries: everything shown was created by the user and survives restarts.
class CalendarModel final : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QDate month READ month WRITE setMonth NOTIFY monthChanged)
    Q_PROPERTY(QDate selected READ selected WRITE setSelected NOTIFY selectedChanged)
    Q_PROPERTY(QVariantList monthGrid READ monthGrid NOTIFY monthChanged)
    Q_PROPERTY(QVariantList dayEvents READ dayEvents NOTIFY eventsChanged)
public:
    enum Roles { TitleRole = Qt::UserRole + 1, TimeRole, IdRole };
    explicit CalendarModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QDate month() const { return m_month; }
    void setMonth(const QDate &date);
    QDate selected() const { return m_selected; }
    void setSelected(const QDate &date);
    QVariantList monthGrid() const;
    QVariantList dayEvents() const;
    Q_INVOKABLE void addEvent(const QString &title, const QString &time);
    Q_INVOKABLE void removeEvent(int id);
    Q_INVOKABLE void previousMonth();
    Q_INVOKABLE void nextMonth();
    Q_INVOKABLE bool hasEvents(const QDate &date) const;
signals:
    void monthChanged();
    void selectedChanged();
    void eventsChanged();
    void error(const QString &message);
private:
    struct Event { int id; QDate date; QString title; QString time; };
    void load();
    void save();
    QVector<Event> m_events;
    QDate m_month{QDate::currentDate().year(), QDate::currentDate().month(), 1};
    QDate m_selected{QDate::currentDate()};
    int m_nextId = 1;
    QString m_path;
};
} // namespace mat
