#include "image_list_model.hpp"

#include <QDir>
#include <QFileInfo>
#include <QImageReader>

namespace mat {

ImageListModel::ImageListModel(QObject *parent) : QObject(parent)
{
    openFolder(QDir::homePath() + QStringLiteral("/Pictures"));
    if (m_files.isEmpty()) openFolder(QStringLiteral("/usr/share/backgrounds/materialized"));
}

QString ImageListModel::currentPath() const
{
    return m_index >= 0 && m_index < m_files.size() ? m_files[m_index] : QString();
}

QString ImageListModel::info() const
{
    const QString path = currentPath();
    if (path.isEmpty()) return QStringLiteral("No image");
    const QFileInfo fi(path);
    const QImageReader reader(path);
    const QSize size = reader.size();
    return QStringLiteral("%1 · %2×%3 · %4 KiB")
        .arg(fi.fileName())
        .arg(size.width()).arg(size.height())
        .arg(fi.size() / 1024);
}

bool ImageListModel::openFile(const QString &path)
{
    const QFileInfo fi(path);
    if (!fi.isFile()) { emit error(QStringLiteral("Not a file: %1").arg(path)); return false; }
    const QImageReader reader(path);
    if (!reader.canRead()) { emit error(QStringLiteral("Unsupported image: %1").arg(path)); return false; }
    m_folder = fi.absolutePath();
    openFolder(m_folder);
    m_index = m_files.indexOf(fi.absoluteFilePath());
    if (m_index < 0) { m_files.prepend(fi.absoluteFilePath()); m_index = 0; }
    emit currentChanged();
    return true;
}

void ImageListModel::openFolder(const QString &path)
{
    QDir dir(path);
    if (!dir.exists()) { emit error(QStringLiteral("Folder not found: %1").arg(path)); return; }
    const QByteArray formats = QImageReader::supportedImageFormats().join(',');
    QStringList filters;
    for (const QByteArray &fmt : QImageReader::supportedImageFormats())
        filters << QStringLiteral("*.%1").arg(QString::fromLatin1(fmt));
    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Name | QDir::IgnoreCase);
    m_files.clear();
    const auto entries = dir.entryInfoList();
    for (const auto &entry : entries) m_files.append(entry.absoluteFilePath());
    m_folder = dir.absolutePath();
    m_index = m_files.isEmpty() ? -1 : 0;
    emit folderChanged();
    emit currentChanged();
}

void ImageListModel::next()
{
    if (m_files.isEmpty()) return;
    m_index = (m_index + 1) % m_files.size();
    emit currentChanged();
}

void ImageListModel::previous()
{
    if (m_files.isEmpty()) return;
    m_index = (m_index - 1 + m_files.size()) % m_files.size();
    emit currentChanged();
}

} // namespace mat
