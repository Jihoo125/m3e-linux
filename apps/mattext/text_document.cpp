#include "text_document.hpp"
#include <QFileInfo>
#include <QSaveFile>

namespace mat {
TextDocument::TextDocument(QObject *parent) : QObject(parent) {}

void TextDocument::setText(const QString &text)
{
    if (text == m_text) return;
    m_text = text;
    m_modified = true;
    emit textChanged();
    emit modifiedChanged();
}

bool TextDocument::open(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        m_status = file.errorString();
        emit statusChanged();
        return false;
    }
    if (file.size() > 16 * 1024 * 1024) {
        m_status = QStringLiteral("File exceeds 16 MiB editor limit");
        emit statusChanged();
        return false;
    }
    m_text = QString::fromUtf8(file.readAll());
    m_path = path;
    m_title = QFileInfo(path).fileName();
    m_modified = false;
    m_status = QStringLiteral("%1 bytes · UTF-8").arg(file.size());
    emit textChanged();
    emit filePathChanged();
    emit modifiedChanged();
    emit statusChanged();
    return true;
}

bool TextDocument::save()
{
    if (m_path.isEmpty()) return false;
    return saveAs(m_path);
}

bool TextDocument::saveAs(const QString &path)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        m_status = file.errorString();
        emit statusChanged();
        return false;
    }
    file.write(m_text.toUtf8());
    if (!file.commit()) {
        m_status = file.errorString();
        emit statusChanged();
        return false;
    }
    m_path = path;
    m_title = QFileInfo(path).fileName();
    m_modified = false;
    m_status = QStringLiteral("Saved · %1 bytes").arg(m_text.toUtf8().size());
    emit filePathChanged();
    emit modifiedChanged();
    emit statusChanged();
    return true;
}

void TextDocument::newDocument()
{
    m_text.clear();
    m_path.clear();
    m_title = QStringLiteral("Untitled");
    m_status = QStringLiteral("Plain text");
    m_modified = false;
    emit textChanged();
    emit filePathChanged();
    emit modifiedChanged();
    emit statusChanged();
}
}
