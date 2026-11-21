#include "application_model.hpp"

#include <algorithm>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>

namespace mat {

ApplicationModel::ApplicationModel(QObject *parent) : QAbstractListModel(parent)
{
    refresh();
}

int ApplicationModel::rowCount(const QModelIndex &) const { return m_visible.size(); }

QVariant ApplicationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visible.size()) return {};
    const auto &app = m_applications[m_visible[index.row()]];
    switch (role) {
    case NameRole: return app.name;
    case CommentRole: return app.comment.isEmpty() ? app.genericName : app.comment;
    case IconRole: return app.icon;
    case ExecRole: return app.exec;
    case IdRole: return app.id;
    case TerminalRole: return app.terminal;
    default: return {};
    }
}

QHash<int, QByteArray> ApplicationModel::roleNames() const
{
    return {{NameRole, "name"}, {CommentRole, "comment"}, {IconRole, "icon"}, {ExecRole, "exec"}, {IdRole, "appId"}, {TerminalRole, "terminal"}};
}

QString ApplicationModel::query() const { return m_query; }
void ApplicationModel::setQuery(const QString &query)
{
    if (query == m_query) return;
    m_query = query;
    emit queryChanged();
    refilter();
}

void ApplicationModel::refresh()
{
    beginResetModel();
    m_applications.clear();
    m_visible.clear();
    endResetModel();
    rescan();
    refilter();
}

DesktopApplication ApplicationModel::readDesktopFile(const QString &path)
{
    DesktopApplication app;
    app.desktopFile = path;
    app.id = QFileInfo(path).completeBaseName();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return app;
    bool mainGroup = false;
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.startsWith(QLatin1Char('['))) {
            mainGroup = line == QLatin1String("[Desktop Entry]");
            continue;
        }
        if (!mainGroup) continue;
        const auto separator = line.indexOf(QLatin1Char('='));
        if (separator <= 0) continue;
        const QString key = line.left(separator);
        const QString value = line.mid(separator + 1);
        if (key == QLatin1String("Name")) app.name = value;
        else if (key == QLatin1String("GenericName")) app.genericName = value;
        else if (key == QLatin1String("Comment")) app.comment = value;
        else if (key == QLatin1String("Exec")) app.exec = value;
        else if (key == QLatin1String("Icon")) app.icon = value;
        else if (key == QLatin1String("Terminal")) app.terminal = value.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
        else if (key == QLatin1String("NoDisplay") || key == QLatin1String("Hidden")) app.hidden = value == QLatin1String("true");
        else if (key == QLatin1String("Categories")) app.categories = value.split(QLatin1Char(';'), Qt::SkipEmptyParts);
        else if (key == QLatin1String("OnlyShowIn")) app.onlyShowIn = value.split(QLatin1Char(';'), Qt::SkipEmptyParts);
        else if (key == QLatin1String("NotShowIn")) app.notShowIn = value.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    }
    return app;
}

QString ApplicationModel::unescapeExec(QString command)
{
    command.remove(QRegularExpression(QStringLiteral("%[fFuUdDnNickvm]")));
    command.remove(QRegularExpression(QStringLiteral("%[a-zA-Z]")));
    command.replace(QStringLiteral("\\\\"), QStringLiteral("\\"));
    return command.trimmed();
}

void ApplicationModel::rescan()
{
    QVector<DesktopApplication> found;
    const auto paths = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    for (const QString &path : paths) {
        QDir directory(path);
        const auto files = directory.entryInfoList({QStringLiteral("*.desktop")}, QDir::Files, QDir::Name);
        for (const QFileInfo &file : files) {
            auto app = readDesktopFile(file.absoluteFilePath());
            if (app.hidden || app.name.isEmpty() || app.exec.isEmpty()) continue;
            bool duplicate = false;
            for (const auto &existing : found) if (existing.id == app.id) { duplicate = true; break; }
            if (!duplicate) found.push_back(std::move(app));
        }
    }
    std::sort(found.begin(), found.end(), [](const auto &a, const auto &b) {
        return a.name.localeAwareCompare(b.name) < 0;
    });
    beginResetModel();
    m_applications = std::move(found);
    endResetModel();
}

void ApplicationModel::refilter()
{
    beginResetModel();
    m_visible.clear();
    for (int i = 0; i < m_applications.size(); ++i) {
        const auto &app = m_applications[i];
        const QString text = app.name + QLatin1Char(' ') + app.genericName + QLatin1Char(' ') + app.comment;
        if (m_query.isEmpty() || text.contains(m_query, Qt::CaseInsensitive)) m_visible.push_back(i);
    }
    endResetModel();
    emit countChanged();
}

bool ApplicationModel::launch(int index)
{
    if (index < 0 || index >= m_visible.size()) return false;
    const auto &app = m_applications[m_visible[index]];
    QString command = unescapeExec(app.exec);
    if (command.isEmpty()) return false;
    if (app.terminal) command = QStringLiteral("matterminal --command ") + command;
    return QProcess::startDetached(command, {});
}

bool ApplicationModel::launchByName(const QString &name)
{
    for (int i = 0; i < m_applications.size(); ++i) {
        const auto &app = m_applications[i];
        if (app.id == name || app.exec == name || app.name.compare(name, Qt::CaseInsensitive) == 0) {
            QString command = unescapeExec(app.exec);
            return QProcess::startDetached(command, {});
        }
    }
    if (name.contains(QRegularExpression(QStringLiteral("^[A-Za-z0-9_./-]+$"))))
        return QProcess::startDetached(name, {});
    emit launchError(QStringLiteral("Application not installed: %1").arg(name));
    return false;
}

void ApplicationModel::launchAutostart()
{
    const auto env = QProcessEnvironment::systemEnvironment();
    const QString desktop = env.value(QStringLiteral("XDG_CURRENT_DESKTOP"));
    QStringList dirs;
    const QString config = env.value(QStringLiteral("XDG_CONFIG_HOME"), QDir::homePath() + QStringLiteral("/.config"));
    dirs << config + QStringLiteral("/autostart");
    const QString xdgDirs = env.value(QStringLiteral("XDG_CONFIG_DIRS"), QStringLiteral("/etc/xdg"));
    for (const QString &dir : xdgDirs.split(QLatin1Char(':'), Qt::SkipEmptyParts))
        dirs << dir + QStringLiteral("/autostart");
    QStringList seen;
    for (const QString &dir : dirs) {
        const auto files = QDir(dir).entryInfoList({QStringLiteral("*.desktop")}, QDir::Files, QDir::Name);
        for (const QFileInfo &file : files) {
            const auto app = readDesktopFile(file.absoluteFilePath());
            if (app.id.isEmpty() || seen.contains(app.id)) continue; // user dir overrides system
            seen << app.id;
            if (app.hidden || app.exec.isEmpty()) continue;
            if (!app.onlyShowIn.isEmpty() && !app.onlyShowIn.contains(desktop)) continue;
            if (app.notShowIn.contains(desktop)) continue;
            const QString command = unescapeExec(app.exec);
            if (command.isEmpty()) continue;
            if (!QProcess::startDetached(command, {}))
                emit launchError(QStringLiteral("Autostart failed: %1").arg(app.id));
        }
    }
}

void ApplicationModel::setWallpaper(const QString &path)
{
    const QString url = path.startsWith(QLatin1String("file://")) ? path : QUrl::fromLocalFile(path).toString();
    if (url == m_wallpaper) return;
    m_wallpaper = url;
    emit wallpaperChanged();
}

} // namespace mat
