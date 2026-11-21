#pragma once

#include <QAbstractListModel>
#include <QFileInfoList>
#include <QVector>

namespace mat {
class FileManagerModel final : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString directory READ directory WRITE setDirectory NOTIFY directoryChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    enum Role { NameRole = Qt::UserRole + 1, PathRole, IsDirectoryRole, SizeRole, ModifiedRole, IconRole };
    explicit FileManagerModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QString directory() const { return m_directory; }
    void setDirectory(const QString &directory);
    QString filter() const { return m_filter; }
    void setFilter(const QString &filter);
    Q_INVOKABLE bool open(int index);
    Q_INVOKABLE void up();
    Q_INVOKABLE void home();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool remove(int index);
    Q_INVOKABLE bool rename(int index, const QString &name);
    Q_INVOKABLE bool createDirectory(const QString &name);
    Q_INVOKABLE bool copy(const QString &source, const QString &destination);
    Q_INVOKABLE void copyToClipboard(int index);
signals:
    void directoryChanged();
    void filterChanged();
    void countChanged();
    void error(const QString &message);
private:
    QFileInfoList m_entries;
    QString m_directory;
    QString m_filter;
};
}
