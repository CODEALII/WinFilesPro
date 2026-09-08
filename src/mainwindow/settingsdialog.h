#pragma once
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QButtonGroup>
#include <functional>
#include "style.h"
#include "i18n.h"
#include "updater.h"

// Moderner Settings-Dialog im selben Stil wie ModernConfirmDialog
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    // Callbacks damit MainWindow direkt reagieren kann ohne Neustart
    std::function<void(Language)>  onLanguageChanged;
    std::function<void(AppTheme)>  onThemeChanged;

    explicit SettingsDialog(QWidget *parent = nullptr, Updater *updater = nullptr);

signals:
    void restartNeeded();

private:
    void buildUi();
    QWidget *makeSectionHeader(const QString &text);
    QWidget *makeLangSelector();
    QWidget *makeThemeSelector();
    QWidget *makeAboutSection();
    QWidget *makeUpdateSection();
    void updateUpdateUi();

    // Pill-style toggle button pair
    QPushButton *langDe = nullptr;
    QPushButton *langEn = nullptr;
    QPushButton *themeDark = nullptr;
    QPushButton *themeLight = nullptr;

    // Update-Sektion
    Updater *m_updater = nullptr;
    QLabel *updateStatusLabel = nullptr;
    QLabel *updateMessageLabel = nullptr;
    QPushButton *updateCheckBtn = nullptr;
    QPushButton *updateInstallBtn = nullptr;
    QProgressBar *updateProgress = nullptr;

    Language  selectedLang;
    AppTheme  selectedTheme;
};