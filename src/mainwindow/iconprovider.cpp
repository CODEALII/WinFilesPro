#include "iconprovider.h"
#include <QStandardPaths>
#include <QDir>

QIcon WinIconProvider::icon(const QFileInfo &info) const {
    if (info.isDir()) return iconForDirectory(info);
    return iconForFile(info);
}

QIcon WinIconProvider::icon(IconType type) const {
    switch (type) {
        case Computer: return QIcon(":/icons/computer.ico");
        case Folder:   return QIcon(":/icons/folder.ico");
        case Drive:    return QIcon(":/icons/hardware.ico");
        default:       return QFileIconProvider::icon(type);
    }
}

QIcon WinIconProvider::iconForDirectory(const QFileInfo &info) const {
    const QString path = QDir::cleanPath(info.absoluteFilePath());

    static const QString desktop   = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    static const QString downloads = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    static const QString documents = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    static const QString pictures  = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    static const QString music     = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
    static const QString videos    = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation));

    if (path == desktop)   return QIcon(":/icons/folder_desktop.ico");
    if (path == downloads) return QIcon(":/icons/folder_downloads.ico");
    if (path == documents) return QIcon(":/icons/folder_documents.ico");
    if (path == pictures)  return QIcon(":/icons/folder_pictures.ico");
    if (path == music)     return QIcon(":/icons/folder_music.ico");
    if (path == videos)    return QIcon(":/icons/folder_videos.ico");

    const QString name = info.fileName();
    if (name.compare("OneDrive", Qt::CaseInsensitive) == 0)
        return QIcon(":/icons/folder_onedrive.ico");
    if (name.compare(".git", Qt::CaseInsensitive) == 0)
        return QIcon(":/icons/folder.ico");

    return QIcon(":/icons/folder.ico");
}

QIcon WinIconProvider::iconForFile(const QFileInfo &info) const {
    const QString ext = info.suffix().toLower();

    static const QStringList audioExts   = {"mp3","wav","flac","ogg","m4a","aac","wma","opus"};
    static const QStringList videoExts   = {"mp4","mkv","avi","mov","wmv","flv","webm","m4v"};
    static const QStringList archiveExts = {"zip","rar","7z","tar","gz","bz2","xz","tgz"};
    static const QStringList imageExts   = {"png","jpg","jpeg","gif","bmp","webp","svg","ico","tiff"};
    static const QStringList docExts     = {"doc","docx","odt","rtf"};
    static const QStringList sheetExts   = {"xls","xlsx","ods","csv"};
    static const QStringList pdfExts     = {"pdf"};
    static const QStringList codeExts    = {"cpp","h","hpp","c","cc","py","js","ts","java","cs","go","rs","php","rb","json","xml","html","css","sh"};
    static const QStringList textExts    = {"txt","md","log","ini","cfg","conf"};

    if (ext == "exe") return QIcon(":/icons/exe.ico");
    if (ext == "dll") return QIcon(":/icons/dll.ico");
    if (archiveExts.contains(ext)) return QIcon(":/icons/zip.ico");
    if (audioExts.contains(ext)) return QIcon(":/icons/audio.ico");
    if (videoExts.contains(ext)) return QIcon(":/icons/video_file.ico");
    if (pdfExts.contains(ext)) return QIcon(":/icons/pdf.ico");
    if (docExts.contains(ext)) return QIcon(":/icons/doc.ico");
    if (sheetExts.contains(ext)) return QIcon(":/icons/sheet.ico");
    if (imageExts.contains(ext)) return QIcon(":/icons/image.ico");
    if (codeExts.contains(ext)) return QIcon(":/icons/code.ico");
    if (textExts.contains(ext)) return QIcon(":/icons/text.ico");

    return QIcon(":/icons/blank.ico");
}
