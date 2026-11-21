#pragma once

#include <QAbstractListModel>
#include <QDesktopServices>
#include <QHash>
#include <QProcess>
#include <QUrl>
#include <QVector>

namespace mat {

struct DesktopApplication {
    QString id;
    QString name;
    QString genericName;
    QString comment;
    QString exec;
    QString icon;
    QString desktopFile;
    QStringList categories;
    QStringList onlyShowIn;
    QStringList notShowIn;
    bool terminal = false;
    bool hidden = false;
};

class ApplicationModel final : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString wallpaper READ wallpaper NOTIFY wallpaperChanged)
public:
    enum Roles { NameRole = Qt::UserRole + 1, CommentRole, IconRole, ExecRole, IdRole, TerminalRole };
    explicit ApplicationModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QString query() const;
    void setQuery(const QString &query);
    Q_INVOKABLE bool launch(int index);
    Q_INVOKABLE void refresh();
    QString wallpaper() const { return m_wallpaper; }
    Q_INVOKABLE bool launchByName(const QString &name);
    Q_INVOKABLE void setWallpaper(const QString &path);
    /// XDG autostart: launch entries from ~/.config/autostart and
    /// $XDG_CONFIG_DIRS/autostart, honoring OnlyShowIn/NotShowIn/Hidden.
    void launchAutostart();

signals:
    void queryChanged();
    void countChanged();
    void launchError(const QString &message);
    void wallpaperChanged();

private:
    static DesktopApplication readDesktopFile(const QString &path);
    static QString unescapeExec(QString command);
    void rescan();
    void refilter();

    QVector<DesktopApplication> m_applications;
    QVector<int> m_visible;
    QString m_query;
    QString m_wallpaper;
};

} // namespace mat
