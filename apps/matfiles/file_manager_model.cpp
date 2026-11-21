#include "file_manager_model.hpp"
#include <QDir>
#include <QDirIterator>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QClipboard>
#include <QUrl>
#include <QDateTime>

namespace mat {
FileManagerModel::FileManagerModel(QObject *parent) : QAbstractListModel(parent)
{
    home();
}

int FileManagerModel::rowCount(const QModelIndex &) const { return m_entries.size(); }

QVariant FileManagerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) return {};
    const auto &entry = m_entries[index.row()];
    switch (role) {
    case NameRole: return entry.fileName();
    case PathRole: return entry.absoluteFilePath();
    case IsDirectoryRole: return entry.isDir();
    case SizeRole: return entry.isDir() ? QStringLiteral("Folder") : QStringLiteral("%1 B").arg(entry.size());
    case ModifiedRole: return entry.lastModified().toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    case IconRole: return entry.isDir() ? QStringLiteral("folder") : QStringLiteral("file");
    default: return {};
    }
}

QHash<int, QByteArray> FileManagerModel::roleNames() const
{
    return {{NameRole, "name"}, {PathRole, "path"}, {IsDirectoryRole, "isDirectory"}, {SizeRole, "size"}, {ModifiedRole, "modified"}, {IconRole, "icon"}};
}

void FileManagerModel::setDirectory(const QString &directory)
{
    const QFileInfo info(directory);
    if (!info.exists() || !info.isDir()) {
        emit error(QStringLiteral("Not a directory: %1").arg(directory));
        return;
    }
    m_directory = info.absoluteFilePath();
    emit directoryChanged();
    refresh();
}

void FileManagerModel::setFilter(const QString &filter)
{
    if (filter == m_filter) return;
    m_filter = filter;
    emit filterChanged();
    refresh();
}

void FileManagerModel::refresh()
{
    beginResetModel();
    QDir dir(m_directory);
    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    dir.setSorting(QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
    if (!m_filter.isEmpty()) dir.setNameFilters({QStringLiteral("*%1*").arg(m_filter)});
    m_entries = dir.entryInfoList();
    endResetModel();
    emit countChanged();
}

bool FileManagerModel::open(int index)
{
    if (index < 0 || index >= m_entries.size()) return false;
    const auto &entry = m_entries[index];
    if (entry.isDir()) { setDirectory(entry.absoluteFilePath()); return true; }
    return QDesktopServices::openUrl(QUrl::fromLocalFile(entry.absoluteFilePath()));
}

void FileManagerModel::up() { setDirectory(QDir(m_directory).absolutePath() == QLatin1String("/") ? QStringLiteral("/") : QDir(m_directory).absoluteFilePath(QStringLiteral(".."))); }
void FileManagerModel::home() { setDirectory(QDir::homePath()); }

bool FileManagerModel::remove(int index)
{
    if (index < 0 || index >= m_entries.size()) return false;
    const auto &entry = m_entries[index];
    bool ok = false;
    if (entry.isDir()) ok = QDir(entry.absoluteFilePath()).removeRecursively();
    else ok = QFile::remove(entry.absoluteFilePath());
    if (!ok) emit error(QStringLiteral("Could not remove %1").arg(entry.fileName()));
    refresh();
    return ok;
}

bool FileManagerModel::rename(int index, const QString &name)
{
    if (index < 0 || index >= m_entries.size() || name.isEmpty() || name.contains(QLatin1Char('/'))) return false;
    const auto &entry = m_entries[index];
    const bool ok = QFile::rename(entry.absoluteFilePath(), QDir(entry.absolutePath()).filePath(name));
    if (!ok) emit error(QStringLiteral("Could not rename %1").arg(entry.fileName()));
    refresh();
    return ok;
}

bool FileManagerModel::createDirectory(const QString &name)
{
    if (name.isEmpty() || name.contains(QLatin1Char('/'))) return false;
    const bool ok = QDir(m_directory).mkdir(name);
    if (!ok) emit error(QStringLiteral("Could not create directory %1").arg(name));
    refresh();
    return ok;
}

bool FileManagerModel::copy(const QString &source, const QString &destination)
{
    const bool ok = QFile::copy(source, destination);
    if (!ok) emit error(QStringLiteral("Could not copy %1").arg(source));
    refresh();
    return ok;
}

void FileManagerModel::copyToClipboard(int index)
{
    if (index < 0 || index >= m_entries.size()) return;
    QGuiApplication::clipboard()->setText(QUrl::fromLocalFile(m_entries[index].absoluteFilePath()).toString());
}
}
