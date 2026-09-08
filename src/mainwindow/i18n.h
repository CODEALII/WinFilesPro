#pragma once
#include <QString>
#include <QMap>

enum class Language { German, English };

class I18n {
public:
    static void setLanguage(Language lang);
    static Language language();
    static QString tr(const QString &key);

private:
    static Language s_lang;
    static QMap<QString, QString> s_de;
    static QMap<QString, QString> s_en;
    static void init();
    static bool s_initialized;
};

// Kurz-Makro damit man nicht immer I18n::tr() schreiben muss
#define T(key) I18n::tr(key)