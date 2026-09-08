#include "settingsdialog.h"
#include "settingsmanager.h"
#include <QGraphicsDropShadowEffect>
#include <QCheckBox>
#include <QFrame>
#include <QApplication>
#include <QProgressBar>

SettingsDialog::SettingsDialog(QWidget *parent, Updater *updater)
    : QDialog(parent),
      m_updater(updater),
      selectedLang(I18n::language()),
      selectedTheme(AppStyle::currentTheme())
{
    const ThemeColors &c = AppStyle::colors();

    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(460, 640);

    buildUi();
}

void SettingsDialog::buildUi() {
    const ThemeColors &c = AppStyle::colors();

    // Äußeres transparentes Layout für Schatten
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(16, 16, 16, 16);

    // Haupt-Container (der sichtbare Dialog)
    QWidget *container = new QWidget;
    container->setObjectName("settingsContainer");
    container->setStyleSheet(QString(R"(
        QWidget#settingsContainer {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 12px;
        }
    )").arg(c.elevatedBg, c.border));

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(40);
    shadow->setColor(QColor(0, 0, 0, 110));
    shadow->setOffset(0, 8);
    container->setGraphicsEffect(shadow);

    outerLayout->addWidget(container);

    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ---- Titelleiste ----
    QWidget *titleBar = new QWidget;
    titleBar->setStyleSheet(QString(
        "background-color: %1; border-radius: 12px 12px 0 0; border-bottom: 1px solid %2;"
    ).arg(c.chromeBg, c.border));
    titleBar->setFixedHeight(52);

    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(20, 0, 16, 0);

    QLabel *titleIcon = new QLabel("⚙");
    titleIcon->setStyleSheet(QString("color: %1; font-size: 18px; background: transparent;").arg(c.accent));
    QLabel *titleText = new QLabel(T("settings_title"));
    titleText->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: 700; background: transparent;").arg(c.textPrimary));

    QPushButton *closeBtn = new QPushButton("✕");
    closeBtn->setFixedSize(30, 30);
    closeBtn->setStyleSheet(QString(R"(
        QPushButton {
            background: transparent;
            border: none;
            border-radius: 6px;
            color: %1;
            font-size: 13px;
        }
        QPushButton:hover { background-color: %2; }
    )").arg(c.textSecondary, c.danger));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);

    titleLayout->addWidget(titleIcon);
    titleLayout->addSpacing(8);
    titleLayout->addWidget(titleText);
    titleLayout->addStretch();
    titleLayout->addWidget(closeBtn);
    layout->addWidget(titleBar);

    // ---- Scrollbarer Inhalt ----
    QWidget *content = new QWidget;
    content->setStyleSheet("background: transparent;");
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(20, 16, 20, 16);
    contentLayout->setSpacing(8);

    // --- Sektion: Sprache ---
    contentLayout->addWidget(makeSectionHeader(T("settings_section_language")));
    contentLayout->addWidget(makeLangSelector());
    contentLayout->addSpacing(8);

    // Trennlinie
    QFrame *sep1 = new QFrame;
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet(QString("color: %1;").arg(c.border));
    contentLayout->addWidget(sep1);
    contentLayout->addSpacing(8);

    // --- Sektion: Erscheinungsbild ---
    contentLayout->addWidget(makeSectionHeader(T("settings_section_appearance")));
    contentLayout->addWidget(makeThemeSelector());
    contentLayout->addSpacing(8);

    QFrame *sep2 = new QFrame;
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet(QString("color: %1;").arg(c.border));
    contentLayout->addWidget(sep2);
    contentLayout->addSpacing(8);

    // --- Sektion: Updates ---
    contentLayout->addWidget(makeSectionHeader(T("settings_section_updates")));
    contentLayout->addWidget(makeUpdateSection());

    QFrame *sep3 = new QFrame;
    sep3->setFrameShape(QFrame::HLine);
    sep3->setStyleSheet(QString("color: %1;").arg(c.border));
    contentLayout->addWidget(sep3);
    contentLayout->addSpacing(8);

    // --- Sektion: Info ---
    contentLayout->addWidget(makeSectionHeader(T("settings_section_info")));
    contentLayout->addWidget(makeAboutSection());

    contentLayout->addStretch();
    layout->addWidget(content);

    // ---- Footer mit Hinweis + Speichern ----
    QWidget *footer = new QWidget;
    footer->setStyleSheet(QString(
        "background-color: %1; border-radius: 0 0 12px 12px; border-top: 1px solid %2;"
    ).arg(c.chromeBg, c.border));
    footer->setFixedHeight(60);

    QHBoxLayout *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(20, 0, 20, 0);

    QLabel *hint = new QLabel(T("settings_restart"));
    hint->setStyleSheet(QString("color: %1; font-size: 11px; background: transparent;").arg(c.textSecondary));

    QPushButton *saveBtn = new QPushButton(T("settings_save"));
    saveBtn->setObjectName("primaryBtn");
    saveBtn->setFixedHeight(34);
    saveBtn->setFixedWidth(110);
    saveBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            border: none;
            border-radius: 6px;
            color: #0b0b0b;
            font-size: 13px;
            font-weight: 600;
            padding: 0 16px;
        }
        QPushButton:hover { background-color: %2; }
        QPushButton:pressed { background-color: %3; }
    )").arg(c.accent, c.accentHover, c.accentPressed));

    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        // Sprache anwenden
        if (selectedLang != I18n::language()) {
            I18n::setLanguage(selectedLang);
            SettingsManager::setLanguage(selectedLang);
            if (onLanguageChanged) onLanguageChanged(selectedLang);
        }
        // Theme anwenden
        if (selectedTheme != AppStyle::currentTheme()) {
            AppStyle::setTheme(selectedTheme);
            SettingsManager::setTheme(selectedTheme);
            if (onThemeChanged) onThemeChanged(selectedTheme);
        }
        // Einstellungen speichern
        SettingsManager::save();
        accept();
    });

    footerLayout->addWidget(hint);
    footerLayout->addStretch();

    QPushButton *cancelBtn = new QPushButton(T("settings_cancel"));
    cancelBtn->setFixedHeight(34);
    cancelBtn->setFixedWidth(100);
    cancelBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: transparent;
            border: 1px solid %1;
            border-radius: 6px;
            color: %2;
            font-size: 13px;
            font-weight: 600;
            padding: 0 16px;
        }
        QPushButton:hover { background-color: %3; }
    )").arg(c.border, c.textPrimary, c.hoverBg));
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    footerLayout->addWidget(cancelBtn);
    footerLayout->addSpacing(8);
    footerLayout->addWidget(saveBtn);
    layout->addWidget(footer);
}

QWidget *SettingsDialog::makeSectionHeader(const QString &text) {
    const ThemeColors &c = AppStyle::colors();
    QLabel *label = new QLabel(text);
    label->setStyleSheet(QString(
        "color: %1; font-size: 11px; font-weight: 700; "
        "text-transform: uppercase; letter-spacing: 1px; "
        "background: transparent; padding-bottom: 4px;"
    ).arg(c.textSecondary));
    return label;
}

QWidget *SettingsDialog::makeLangSelector() {
    const ThemeColors &c = AppStyle::colors();

    QWidget *row = new QWidget;
    row->setStyleSheet("background: transparent;");
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 4, 0, 4);
    layout->setSpacing(0);

    // Pill-Container
    QWidget *pill = new QWidget;
    pill->setStyleSheet(QString(R"(
        QWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
        }
    )").arg(c.chromeBg, c.border));

    QHBoxLayout *pillLayout = new QHBoxLayout(pill);
    pillLayout->setContentsMargins(3, 3, 3, 3);
    pillLayout->setSpacing(3);

    auto makePillBtn = [&](const QString &text, bool active) -> QPushButton* {
        QPushButton *btn = new QPushButton(text);
        btn->setCheckable(true);
        btn->setChecked(active);
        btn->setFixedHeight(32);
        btn->setMinimumWidth(90);
        QString activeSS = QString(R"(
            QPushButton {
                background-color: %1;
                color: %2;
                border: none;
                border-radius: 6px;
                font-size: 13px;
                font-weight: 600;
                padding: 0 14px;
            }
        )").arg(c.accent, "#0b0b0b");
        QString inactiveSS = QString(R"(
            QPushButton {
                background-color: transparent;
                color: %1;
                border: none;
                border-radius: 6px;
                font-size: 13px;
                padding: 0 14px;
            }
            QPushButton:hover { background-color: %2; }
        )").arg(c.textPrimary, c.hoverBg);
        btn->setStyleSheet(active ? activeSS : inactiveSS);

        // Speichern für späteren Zugriff
        btn->setProperty("activeSS", activeSS);
        btn->setProperty("inactiveSS", inactiveSS);
        return btn;
    };

    bool deSel = (selectedLang == Language::German);
    langDe = makePillBtn(T("settings_lang_de"), deSel);
    langEn = makePillBtn(T("settings_lang_en"), !deSel);

    connect(langDe, &QPushButton::clicked, this, [this]() {
        selectedLang = Language::German;
        langDe->setChecked(true);
        langEn->setChecked(false);
        langDe->setStyleSheet(langDe->property("activeSS").toString());
        langEn->setStyleSheet(langEn->property("inactiveSS").toString());
    });
    connect(langEn, &QPushButton::clicked, this, [this]() {
        selectedLang = Language::English;
        langEn->setChecked(true);
        langDe->setChecked(false);
        langEn->setStyleSheet(langEn->property("activeSS").toString());
        langDe->setStyleSheet(langDe->property("inactiveSS").toString());
    });

    pillLayout->addWidget(langDe);
    pillLayout->addWidget(langEn);

    layout->addWidget(pill);
    layout->addStretch();
    return row;
}

QWidget *SettingsDialog::makeThemeSelector() {
    const ThemeColors &c = AppStyle::colors();

    QWidget *row = new QWidget;
    row->setStyleSheet("background: transparent;");
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 4, 0, 4);
    layout->setSpacing(0);

    QWidget *pill = new QWidget;
    pill->setStyleSheet(QString(R"(
        QWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
        }
    )").arg(c.chromeBg, c.border));

    QHBoxLayout *pillLayout = new QHBoxLayout(pill);
    pillLayout->setContentsMargins(3, 3, 3, 3);
    pillLayout->setSpacing(3);

    auto makePillBtn = [&](const QString &text, bool active) -> QPushButton* {
        QPushButton *btn = new QPushButton(text);
        btn->setCheckable(true);
        btn->setChecked(active);
        btn->setFixedHeight(32);
        btn->setMinimumWidth(90);
        QString activeSS = QString(R"(
            QPushButton {
                background-color: %1;
                color: %2;
                border: none;
                border-radius: 6px;
                font-size: 13px;
                font-weight: 600;
                padding: 0 14px;
            }
        )").arg(c.accent, "#0b0b0b");
        QString inactiveSS = QString(R"(
            QPushButton {
                background-color: transparent;
                color: %1;
                border: none;
                border-radius: 6px;
                font-size: 13px;
                padding: 0 14px;
            }
            QPushButton:hover { background-color: %2; }
        )").arg(c.textPrimary, c.hoverBg);
        btn->setStyleSheet(active ? activeSS : inactiveSS);
        btn->setProperty("activeSS", activeSS);
        btn->setProperty("inactiveSS", inactiveSS);
        return btn;
    };

    bool darkSel = (selectedTheme == AppTheme::Dark);
    themeDark  = makePillBtn("🌙  " + T("settings_dark"),  darkSel);
    themeLight = makePillBtn("☀️  " + T("settings_light"), !darkSel);

    connect(themeDark, &QPushButton::clicked, this, [this]() {
        selectedTheme = AppTheme::Dark;
        themeDark->setChecked(true);
        themeLight->setChecked(false);
        themeDark->setStyleSheet(themeDark->property("activeSS").toString());
        themeLight->setStyleSheet(themeLight->property("inactiveSS").toString());
    });
    connect(themeLight, &QPushButton::clicked, this, [this]() {
        selectedTheme = AppTheme::Light;
        themeLight->setChecked(true);
        themeDark->setChecked(false);
        themeLight->setStyleSheet(themeLight->property("activeSS").toString());
        themeDark->setStyleSheet(themeDark->property("inactiveSS").toString());
    });

    pillLayout->addWidget(themeDark);
    pillLayout->addWidget(themeLight);

    layout->addWidget(pill);
    layout->addStretch();
    return row;
}

QWidget *SettingsDialog::makeAboutSection() {
    const ThemeColors &c = AppStyle::colors();

    QWidget *box = new QWidget;
    box->setStyleSheet(QString(R"(
        QWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
        }
    )").arg(c.chromeBg, c.border));

    QHBoxLayout *layout = new QHBoxLayout(box);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(14);

    QLabel *icon = new QLabel("🗂");
    icon->setStyleSheet("font-size: 28px; background: transparent; border: none;");

    QVBoxLayout *textLayout = new QVBoxLayout;
    textLayout->setSpacing(2);

    QLabel *name = new QLabel("WinFilesPro");
    name->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: 700; background: transparent; border: none;").arg(c.textPrimary));

    QLabel *version = new QLabel(T("settings_version"));
    version->setStyleSheet(QString("color: %1; font-size: 12px; background: transparent; border: none;").arg(c.textSecondary));

    textLayout->addWidget(name);
    textLayout->addWidget(version);

    layout->addWidget(icon);
    layout->addLayout(textLayout);
    layout->addStretch();

    return box;
}

QWidget *SettingsDialog::makeUpdateSection() {
    const ThemeColors &c = AppStyle::colors();

    QWidget *box = new QWidget;
    box->setStyleSheet(QString(R"(
        QWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
        }
    )").arg(c.chromeBg, c.border));

    QVBoxLayout *layout = new QVBoxLayout(box);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(10);

    updateStatusLabel = new QLabel(T("settings_version_now").arg(Updater::currentVersion()));
    updateStatusLabel->setStyleSheet(QString("color: %1; font-size: 13px; background: transparent;").arg(c.textPrimary));

    updateMessageLabel = new QLabel;
    updateMessageLabel->setWordWrap(true);
    updateMessageLabel->setStyleSheet(QString("color: %1; font-size: 12px; background: transparent;").arg(c.textSecondary));
    updateMessageLabel->hide();

    updateProgress = new QProgressBar;
    updateProgress->setRange(0, 100);
    updateProgress->setValue(0);
    updateProgress->setTextVisible(false);
    updateProgress->setFixedHeight(6);
    updateProgress->setStyleSheet(QString(R"(
        QProgressBar {
            background-color: %1;
            border: none;
            border-radius: 3px;
        }
        QProgressBar::chunk {
            background-color: %2;
            border-radius: 3px;
        }
    )").arg(c.hoverBg, c.accent));
    updateProgress->hide();

    updateCheckBtn = new QPushButton(T("settings_check_updates"));
    updateCheckBtn->setFixedHeight(30);
    updateCheckBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: transparent;
            border: 1px solid %1;
            border-radius: 6px;
            color: %2;
            font-size: 12px;
            font-weight: 600;
            padding: 0 12px;
        }
        QPushButton:hover { background-color: %3; }
    )").arg(c.border, c.textPrimary, c.hoverBg));

    updateInstallBtn = new QPushButton(T("settings_install_update"));
    updateInstallBtn->setObjectName("primaryBtn");
    updateInstallBtn->setFixedHeight(30);
    updateInstallBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            border: none;
            border-radius: 6px;
            color: #0b0b0b;
            font-size: 12px;
            font-weight: 600;
            padding: 0 12px;
        }
        QPushButton:hover { background-color: %2; }
    )").arg(c.accent, c.accentHover));
    updateInstallBtn->hide();

    if (m_updater) {
        connect(updateCheckBtn, &QPushButton::clicked, m_updater, &Updater::checkForUpdates);
        connect(updateInstallBtn, &QPushButton::clicked, m_updater, &Updater::installUpdate);
        connect(m_updater, &Updater::stateChanged, this, &SettingsDialog::updateUpdateUi);
        connect(m_updater, &Updater::message, this, [this](const QString &text) {
            updateMessageLabel->setText(text);
            updateMessageLabel->show();
        });
        connect(m_updater, &Updater::progressChanged, this, [this](int percent) {
            if (auto *bar = qobject_cast<QProgressBar *>(updateProgress)) {
                bar->setRange(0, 100);
                bar->setValue(percent);
            }
        });
        updateUpdateUi();
    } else {
        updateCheckBtn->setEnabled(false);
    }

    QWidget *row = new QWidget;
    row->setStyleSheet("background: transparent;");
    QHBoxLayout *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->addWidget(updateCheckBtn);
    rowLayout->addStretch();
    rowLayout->addWidget(updateInstallBtn);

    layout->addWidget(updateStatusLabel);
    layout->addWidget(updateMessageLabel);
    layout->addWidget(updateProgress);
    layout->addWidget(row);

    return box;
}

void SettingsDialog::updateUpdateUi() {
    if (!m_updater) return;

    QProgressBar *bar = qobject_cast<QProgressBar *>(updateProgress);
    Updater::State st = m_updater->state();

    switch (st) {
    case Updater::State::Checking:
        updateStatusLabel->setText(T("settings_checking"));
        updateMessageLabel->hide();
        updateCheckBtn->setEnabled(false);
        updateInstallBtn->hide();
        if (bar) bar->setRange(0, 0);
        updateProgress->show();
        break;
    case Updater::State::UpToDate:
        updateStatusLabel->setText(T("settings_no_updates").arg(Updater::currentVersion()));
        updateMessageLabel->hide();
        updateCheckBtn->setEnabled(true);
        updateInstallBtn->hide();
        updateProgress->hide();
        break;
    case Updater::State::UpdateAvailable:
        updateStatusLabel->setText(T("settings_update_available").arg(m_updater->latestVersion()));
        updateMessageLabel->hide();
        updateCheckBtn->setEnabled(true);
        updateInstallBtn->show();
        updateProgress->hide();
        break;
    case Updater::State::Downloading:
        updateStatusLabel->setText(T("settings_downloading").arg(m_updater->latestVersion()));
        updateCheckBtn->setEnabled(false);
        updateInstallBtn->hide();
        if (bar) bar->setRange(0, 100);
        updateProgress->show();
        break;
    case Updater::State::Building:
        updateStatusLabel->setText(T("settings_building").arg(m_updater->latestVersion()));
        updateCheckBtn->setEnabled(false);
        updateInstallBtn->hide();
        if (bar) bar->setRange(0, 0);
        updateProgress->show();
        break;
    case Updater::State::Done:
        updateStatusLabel->setText(T("settings_done"));
        updateMessageLabel->hide();
        updateCheckBtn->setEnabled(false);
        updateInstallBtn->hide();
        updateProgress->hide();
        emit restartNeeded();
        break;
    case Updater::State::Failed:
        updateStatusLabel->setText(T("settings_update_error").arg(m_updater->errorMessage()));
        updateMessageLabel->hide();
        updateCheckBtn->setEnabled(true);
        updateInstallBtn->hide();
        updateProgress->hide();
        break;
    case Updater::State::Idle:
    default:
        updateStatusLabel->setText(T("settings_version_now").arg(Updater::currentVersion()));
        updateMessageLabel->hide();
        updateCheckBtn->setEnabled(true);
        updateInstallBtn->hide();
        updateProgress->hide();
        break;
    }
}