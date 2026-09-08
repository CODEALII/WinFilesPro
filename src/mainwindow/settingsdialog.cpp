#include "settingsdialog.h"
#include <QGraphicsDropShadowEffect>
#include <QCheckBox>
#include <QFrame>
#include <QApplication>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent),
      selectedLang(I18n::language()),
      selectedTheme(AppStyle::currentTheme())
{
    const ThemeColors &c = AppStyle::colors();

    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(460, 560);

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
            if (onLanguageChanged) onLanguageChanged(selectedLang);
        }
        // Theme anwenden
        if (selectedTheme != AppStyle::currentTheme()) {
            AppStyle::setTheme(selectedTheme);
            if (onThemeChanged) onThemeChanged(selectedTheme);
        }
        accept();
    });

    footerLayout->addWidget(hint);
    footerLayout->addStretch();
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