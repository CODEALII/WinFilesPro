#pragma once
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QButtonGroup>
#include <QCheckBox>
#include <QScrollArea>
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

    explicit SettingsDialog(QWidget *parent = nullptr, Updater *updater = nullptr);

signals:
    void restartNeeded();

private:
    void buildUi();
    QWidget *makeSectionHeader(const QString &text);
    QWidget *makeLangSelector();
    QWidget *makeAboutSection();
    QWidget *makeUpdateSection();
    QWidget *makeGeneralSection();
    void updateUpdateUi();

    QCheckBox *chkRestore = nullptr;
    QCheckBox *chkMouseNav = nullptr;
    QCheckBox *chkConfirmDelete = nullptr;
    QCheckBox *chkAnimations = nullptr;

    // Pill-style toggle button pair
    QPushButton *langDe = nullptr;
    QPushButton *langEn = nullptr;

    // Update-Sektion
    Updater *m_updater = nullptr;
    QLabel *updateStatusLabel = nullptr;
    QLabel *updateMessageLabel = nullptr;
    QPushButton *updateCheckBtn = nullptr;
    QPushButton *updateInstallBtn = nullptr;
    QProgressBar *updateProgress = nullptr;

    Language  selectedLang;
};