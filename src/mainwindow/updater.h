#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QProcess>

class Updater : public QObject {
    Q_OBJECT
public:
    enum class State {
        Idle,
        Checking,
        UpToDate,
        UpdateAvailable,
        Downloading,
        Building,
        Done,
        Failed
    };

    explicit Updater(QObject *parent = nullptr);

    static QString currentVersion();

    State state() const;
    QString latestVersion() const;
    QString errorMessage() const;

public slots:
    void checkForUpdates();
    void installUpdate();

signals:
    void stateChanged(Updater::State state);
    void progressChanged(int percent);
    void message(const QString &text);

private:
    void finishCheck(QNetworkReply *reply);
    void downloadRelease();
    void extractAndBuild();
    void buildStep(const QString &program, const QStringList &args);
    void onBuildFinished(int exitCode, QProcess::ExitStatus status);
    void installBinary();
    void cleanupTemp();
    void fail(const QString &text);
    void setState(State state);

    QNetworkAccessManager *m_nam;
    QProcess *m_proc;
    State m_state;
    QString m_latest;
    QString m_error;
    QString m_tmpDir;
    QString m_srcDir;
    QString m_buildDir;
    QString m_tarballPath;
    int m_step = 0; // 0=tar entpacken, 1=cmake konfigurieren, 2=cmake bauen
};