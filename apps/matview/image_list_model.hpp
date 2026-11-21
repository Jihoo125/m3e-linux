#pragma once

#include <QObject>
#include <QStringList>

namespace mat {
/// Real directory-backed image browsing: scans a folder for images Qt can
/// decode, navigates with wrap-around, and reports actual file metadata.
class ImageListModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString folder READ folder NOTIFY folderChanged)
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentChanged)
    Q_PROPERTY(int index READ index NOTIFY currentChanged)
    Q_PROPERTY(int count READ count NOTIFY folderChanged)
    Q_PROPERTY(QString info READ info NOTIFY currentChanged)
public:
    explicit ImageListModel(QObject *parent = nullptr);
    QString folder() const { return m_folder; }
    QString currentPath() const;
    int index() const { return m_index; }
    int count() const { return m_files.size(); }
    QString info() const;
    Q_INVOKABLE bool openFile(const QString &path);
    Q_INVOKABLE void openFolder(const QString &path);
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
signals:
    void folderChanged();
    void currentChanged();
    void error(const QString &message);
private:
    QStringList m_files;
    QString m_folder;
    int m_index = -1;
};
} // namespace mat
