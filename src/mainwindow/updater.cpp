#include "updater.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVersionNumber>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryDir>
#include <QCoreApplication>

Updater::Updater(QObject *parent)
    : QObject(parent),
      m_nam(new QNetworkAccessManager(this)),
      m_proc(nullptr),
      m_state(State::Idle),
      m_step(0) {
}

QString Updater::currentVersion() {
#ifdef WINFILESPRO_VERSION
    return QStringLiteral(WINFILESPRO_VERSION);
#else
    return QStringLiteral("0.3.0");
#endif
}

Updater::State Updater::state() const {
    return m_state;
}

QString Updater::latestVersion() const {
    return m_latest;
}

QString Updater::errorMessage() const {
    return m_error;
}

void Updater::setState(State state) {
    if (m_state == state) return;
    m_state = state;
    emit stateChanged(state);
}

void Updater::fail(const QString &text) {
    m_error = text;
    setState(State::Failed);
    cleanupTemp();
}

void Updater::checkForUpdates() {
    if (m_state == State::Checking || m_state == State::Downloading ||
        m_state == State::Building) {
        return;
    }
    m_error.clear();
    setState(State::Checking);

    QNetworkRequest request(QUrl("https://api.github.com/repos/CODEALII/WinFilesPro/releases/latest"));
    request.setRawHeader("User-Agent", "WinFilesPro-Updater");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        finishCheck(reply);
    });
}

void Updater::finishCheck(QNetworkReply *reply) {
    reply->deleteLater();
    if (m_state != State::Checking) return;

    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() == QNetworkReply::ContentNotFoundError) {
            setState(State::UpToDate);
        } else {
            fail(reply->errorString());
        }
        return;
    }

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        fail(QStringLiteral("Ungültige Antwort von GitHub."));
        return;
    }

    QJsonObject obj = doc.object();
    QString tag = obj.value("tag_name").toString();
    if (tag.isEmpty()) {
        setState(State::UpToDate);
        return;
    }
    m_latest = tag;
    if (m_latest.startsWith('v')) m_latest.remove(0, 1);

    QVersionNumber latest = QVersionNumber::fromString(m_latest);
    if (latest.isNull()) latest = QVersionNumber(0, 0, 0);
    QVersionNumber current = QVersionNumber::fromString(currentVersion());
    if (current.isNull()) current = QVersionNumber(0, 0, 0);

    setState(latest > current ? State::UpdateAvailable : State::UpToDate);
}

void Updater::installUpdate() {
    if (m_state != State::UpdateAvailable) return;
    downloadRelease();
}

void Updater::downloadRelease() {
    setState(State::Downloading);
    emit progressChanged(0);

    QTemporaryDir tmp;
    if (!tmp.isValid()) {
        fail(QStringLiteral("Temporäres Verzeichnis konnte nicht erstellt werden."));
        return;
    }
    m_tmpDir = tmp.path();
    tmp.setAutoRemove(false);

    m_tarballPath = m_tmpDir + "/WinFilesPro-" + m_latest + ".tar.gz";

    QUrl url(QString("https://api.github.com/repos/CODEALII/WinFilesPro/tarball/v%1").arg(m_latest));
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "WinFilesPro-Updater");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        int pct = total > 0 ? int(received * 100 / total) : 0;
        emit progressChanged(pct);
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (m_state != State::Downloading) return;

        if (reply->error() != QNetworkReply::NoError) {
            fail(reply->errorString());
            return;
        }

        QFile file(m_tarballPath);
        if (!file.open(QIODevice::WriteOnly)) {
            fail(QStringLiteral("Download konnte nicht gespeichert werden."));
            return;
        }
        file.write(reply->readAll());
        file.close();
        emit progressChanged(100);

        extractAndBuild();
    });
}

void Updater::extractAndBuild() {
    setState(State::Building);
    emit message(QStringLiteral("Extrahiere Quelle…"));

    m_buildDir = m_tmpDir + "/build";
    m_srcDir = m_tmpDir + "/src";
    QDir root(m_tmpDir);
    root.mkdir("build");
    root.mkdir("src");

    m_step = 0;
    buildStep("tar", { "-xzf", m_tarballPath, "-C", m_srcDir });
}

void Updater::buildStep(const QString &program, const QStringList &args) {
    m_proc = new QProcess(this);
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Updater::onBuildFinished);
    m_proc->start(program, args);
}

void Updater::onBuildFinished(int exitCode, QProcess::ExitStatus status) {
    if (m_state != State::Building) {
        if (m_proc) m_proc->deleteLater();
        m_proc = nullptr;
        return;
    }

    bool ok = (status == QProcess::NormalExit && exitCode == 0);
    if (!ok) {
        fail(QStringLiteral("Update-Build fehlgeschlagen (Exit-Code %1).").arg(exitCode));
        m_proc->deleteLater();
        m_proc = nullptr;
        return;
    }

    m_proc->deleteLater();
    m_proc = nullptr;

    if (m_step == 0) {
        // Tar erfolgreich entpackt – Quellverzeichnis lokalisieren und konfigurieren.
        QString sourceDir;
        const QFileInfoList candidates =
            QDir(m_srcDir).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &fi : candidates) {
            if (QFileInfo::exists(fi.absoluteFilePath() + "/CMakeLists.txt")) {
                sourceDir = fi.absoluteFilePath();
                break;
            }
        }
        if (sourceDir.isEmpty()) {
            fail(QStringLiteral("Quellcode nicht gefunden."));
            return;
        }
        m_srcDir = sourceDir;
        m_step = 1;
        emit message(QStringLiteral("Konfiguriere Build…"));
        buildStep("cmake",
                  { "-S", m_srcDir, "-B", m_buildDir, "-DCMAKE_BUILD_TYPE=Release" });
    } else if (m_step == 1) {
        // Konfiguration erfolgreich – jetzt den eigentlichen Build starten.
        m_step = 2;
        emit message(QStringLiteral("Baue WinFilesPro…"));
        buildStep("cmake", { "--build", m_buildDir, "-j" });
    } else if (m_step == 2) {
        installBinary();
    } else {
        fail(QStringLiteral("Ungültiger Build-Schritt."));
    }
}

void Updater::installBinary() {
    QString executable = m_buildDir + "/WinFilesPro";
    if (!QFileInfo::exists(executable)) {
        fail(QStringLiteral("Binärdatei wurde nicht erzeugt."));
        return;
    }

    QString target = QCoreApplication::applicationFilePath();
    QString backup = target + ".old";

    QFile::remove(backup);
    if (QFile::exists(target) && !QFile::rename(target, backup)) {
        fail(QStringLiteral("Aktuelle Binärdatei konnte nicht gesichert werden."));
        return;
    }

    if (!QFile::copy(executable, target)) {
        fail(QStringLiteral("Neue Binärdatei konnte nicht installiert werden."));
        return;
    }
    QFile::setPermissions(target, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    cleanupTemp();
    setState(State::Done);
}

void Updater::cleanupTemp() {
    if (m_tmpDir.isEmpty()) return;
    QDir(m_tmpDir).removeRecursively();
    m_tmpDir.clear();
}