#pragma once

#include <QObject>
#include <QFile>
#include <QTextStream>

namespace mat {
class TextDocument final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)
    Q_PROPERTY(QString title READ title NOTIFY filePathChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
public:
    explicit TextDocument(QObject *parent = nullptr);
    QString text() const { return m_text; }
    QString filePath() const { return m_path; }
    QString title() const { return m_title; }
    bool modified() const { return m_modified; }
    QString status() const { return m_status; }
    void setText(const QString &text);
    Q_INVOKABLE bool open(const QString &path);
    Q_INVOKABLE bool save();
    Q_INVOKABLE bool saveAs(const QString &path);
    Q_INVOKABLE void newDocument();
signals:
    void textChanged();
    void filePathChanged();
    void modifiedChanged();
    void statusChanged();
private:
    QString m_text;
    QString m_path;
    QString m_title{QStringLiteral("Untitled")};
    QString m_status{QStringLiteral("Plain text")};
    bool m_modified = false;
};
}
