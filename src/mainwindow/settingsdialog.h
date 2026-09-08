#pragma once
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QButtonGroup>
#include <functional>
#include "style.h"
#include "i18n.h"

// Moderner Settings-Dialog im selben Stil wie ModernConfirmDialog
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    // Callbacks damit MainWindow direkt reagieren kann ohne Neustart
    std::function<void(Language)>  onLanguageChanged;
    std::function<void(AppTheme)>  onThemeChanged;

    explicit SettingsDialog(QWidget *parent = nullptr);

private:
    void buildUi();
    QWidget *makeSectionHeader(const QString &text);
    QWidget *makeToggleRow(const QString &label, bool checked,
                           std::function<void(bool)> callback);
    QWidget *makeLangSelector();
    QWidget *makeThemeSelector();
    QWidget *makeAboutSection();

    // Pill-style toggle button pair
    QPushButton *langDe = nullptr;
    QPushButton *langEn = nullptr;
    QPushButton *themeDark = nullptr;
    QPushButton *themeLight = nullptr;

    Language  selectedLang;
    AppTheme  selectedTheme;
};