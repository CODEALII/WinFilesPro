#pragma once
#include <QFileIconProvider>
#include <QIcon>
#include <QFileInfo>

class WinIconProvider : public QFileIconProvider {
public:
    QIcon icon(const QFileInfo &info) const override;
    QIcon icon(IconType type) const override;

private:
    QIcon iconForDirectory(const QFileInfo &info) const;
    QIcon iconForFile(const QFileInfo &info) const;
};
