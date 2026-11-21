#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDebug>
#include "text_document.hpp"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir directory;
    const QString path = directory.filePath(QStringLiteral("test.txt"));
    mat::TextDocument document;
    document.setText(QStringLiteral("Materialized\nLinux\n"));
    if (!document.modified() || !document.saveAs(path)) { qCritical() << "save failed"; return 1; }
    if (document.modified()) { qCritical() << "modified flag not cleared"; return 1; }
    mat::TextDocument other;
    if (!other.open(path) || other.text() != document.text()) { qCritical() << "open failed"; return 1; }
    if (!other.open(directory.filePath(QStringLiteral("missing")))) qInfo() << "Missing-file error handled";
    qInfo() << "Text-document tests passed";
    return 0;
}
