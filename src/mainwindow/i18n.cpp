#include "i18n.h"

Language I18n::s_lang = Language::German;
QMap<QString, QString> I18n::s_de;
QMap<QString, QString> I18n::s_en;
bool I18n::s_initialized = false;

void I18n::setLanguage(Language lang) {
    s_lang = lang;
    if (!s_initialized) init();
}

Language I18n::language() { return s_lang; }

QString I18n::tr(const QString &key) {
    if (!s_initialized) init();
    const auto &map = (s_lang == Language::German) ? s_de : s_en;
    return map.value(key, key); // fallback: key selbst
}

void I18n::init() {
    s_initialized = true;

    // ---- Deutsch ----
    s_de["back"]           = "Zurück";
    s_de["forward"]        = "Vorwärts";
    s_de["up"]             = "Nach oben";
    s_de["refresh"]        = "Aktualisieren";
    s_de["search"]         = "Suchen";
    s_de["new_folder"]     = "Neuer Ordner";
    s_de["new_file"]       = "Neue Datei";
    s_de["cut"]            = "Ausschneiden";
    s_de["copy"]           = "Kopieren";
    s_de["paste"]          = "Einfügen";
    s_de["rename"]         = "Umbenennen";
    s_de["delete"]         = "Löschen";
    s_de["sort"]           = "Sortieren";
    s_de["view"]           = "Anzeigen";
    s_de["hidden_files"]   = "Versteckte Dateien";
    s_de["terminal"]       = "Terminal hier öffnen";
    s_de["properties"]     = "Eigenschaften";
    s_de["new_tab"]        = "Neuer Tab";
    s_de["close_tab"]      = "Tab schließen";
    s_de["settings"]       = "Einstellungen";
    s_de["quick_access"]   = "Schnellzugriff";
    s_de["desktop"]        = "Desktop";
    s_de["downloads"]      = "Downloads";
    s_de["documents"]      = "Dokumente";
    s_de["pictures"]       = "Bilder";
    s_de["music"]          = "Musik";
    s_de["videos"]         = "Videos";
    s_de["trash"]          = "Papierkorb";
    s_de["this_pc"]        = "Dieser PC";
    s_de["open"]           = "Öffnen";
    s_de["copy_path"]      = "Pfad kopieren";
    s_de["pin_quick"]      = "An Schnellzugriff anheften";
    s_de["new_folder_ctx"] = "Neuer Ordner";
    s_de["new_file_ctx"]   = "Neue Datei";
    s_de["term_ctx"]       = "Terminal hier öffnen";
    s_de["ready"]          = "Bereit";
    s_de["items"]          = "Elemente";
    s_de["selected"]       = "ausgewählt";
    s_de["drives_title"]   = "Geräte und Laufwerke";
    s_de["drives_sub"]     = "Alle verbundenen Laufwerke im Überblick";
    s_de["local_disk"]     = "Lokaler Datenträger";
    s_de["system"]         = "System";
    s_de["free_of"]        = "GB frei von";
    s_de["gb"]             = "GB";
    s_de["delete_confirm"] = "Elemente löschen";
    s_de["delete_msg"]     = "Möchtest du %1 Element(e) wirklich unwiderruflich löschen?";
    s_de["ok"]             = "OK";
    s_de["cancel"]         = "Abbrechen";
    s_de["error"]          = "Fehler";
    s_de["paste_fail"]     = "Einfügen fehlgeschlagen.";
    s_de["no_terminal"]    = "Kein unterstütztes Terminal gefunden.";
    s_de["rename_dialog"]  = "Umbenennen";
    s_de["rename_label"]   = "Neuer Name:";
    s_de["folder_dialog"]  = "Neuer Ordner";
    s_de["folder_label"]   = "Name:";
    s_de["file_dialog"]    = "Neue Datei";
    s_de["file_label"]     = "Dateiname:";
    s_de["file_exists"]    = "Eine Datei mit diesem Namen existiert bereits.";
    s_de["file_fail"]      = "Datei konnte nicht erstellt werden.";
    s_de["home"]           = "Start";

    // Settings Dialog
    s_de["settings_title"]    = "Einstellungen";
    s_de["settings_lang"]     = "Sprache / Language";
    s_de["settings_lang_de"]  = "Deutsch";
    s_de["settings_lang_en"]  = "English";
    s_de["settings_theme"]    = "Design";
    s_de["settings_dark"]     = "Dunkel";
    s_de["settings_light"]    = "Hell";
    s_de["settings_about"]    = "Über WinFilesPro";
    s_de["settings_version"]  = "Version 0.3.0 · Qt6 · Open Source";
    s_de["settings_restart"]  = "Neustart erforderlich für Sprachänderung";
    s_de["settings_save"]     = "Speichern";
    s_de["settings_hidden"]   = "Versteckte Dateien anzeigen";
    s_de["settings_section_appearance"] = "Erscheinungsbild";
    s_de["settings_section_language"]   = "Sprache";
    s_de["settings_section_info"]       = "Info";

    // ---- English ----
    s_en["back"]           = "Back";
    s_en["forward"]        = "Forward";
    s_en["up"]             = "Up";
    s_en["refresh"]        = "Refresh";
    s_en["search"]         = "Search";
    s_en["new_folder"]     = "New Folder";
    s_en["new_file"]       = "New File";
    s_en["cut"]            = "Cut";
    s_en["copy"]           = "Copy";
    s_en["paste"]          = "Paste";
    s_en["rename"]         = "Rename";
    s_en["delete"]         = "Delete";
    s_en["sort"]           = "Sort";
    s_en["view"]           = "View";
    s_en["hidden_files"]   = "Hidden Files";
    s_en["terminal"]       = "Open Terminal Here";
    s_en["properties"]     = "Properties";
    s_en["new_tab"]        = "New Tab";
    s_en["close_tab"]      = "Close Tab";
    s_en["settings"]       = "Settings";
    s_en["quick_access"]   = "Quick Access";
    s_en["desktop"]        = "Desktop";
    s_en["downloads"]      = "Downloads";
    s_en["documents"]      = "Documents";
    s_en["pictures"]       = "Pictures";
    s_en["music"]          = "Music";
    s_en["videos"]         = "Videos";
    s_en["trash"]          = "Recycle Bin";
    s_en["this_pc"]        = "This PC";
    s_en["open"]           = "Open";
    s_en["copy_path"]      = "Copy Path";
    s_en["pin_quick"]      = "Pin to Quick Access";
    s_en["new_folder_ctx"] = "New Folder";
    s_en["new_file_ctx"]   = "New File";
    s_en["term_ctx"]       = "Open Terminal Here";
    s_en["ready"]          = "Ready";
    s_en["items"]          = "Items";
    s_en["selected"]       = "selected";
    s_en["drives_title"]   = "Devices and Drives";
    s_en["drives_sub"]     = "All connected drives at a glance";
    s_en["local_disk"]     = "Local Disk";
    s_en["system"]         = "System";
    s_en["free_of"]        = "GB free of";
    s_en["gb"]             = "GB";
    s_en["delete_confirm"] = "Delete Items";
    s_en["delete_msg"]     = "Are you sure you want to permanently delete %1 item(s)?";
    s_en["ok"]             = "OK";
    s_en["cancel"]         = "Cancel";
    s_en["error"]          = "Error";
    s_en["paste_fail"]     = "Paste failed.";
    s_en["no_terminal"]    = "No supported terminal found.";
    s_en["rename_dialog"]  = "Rename";
    s_en["rename_label"]   = "New name:";
    s_en["folder_dialog"]  = "New Folder";
    s_en["folder_label"]   = "Name:";
    s_en["file_dialog"]    = "New File";
    s_en["file_label"]     = "File name:";
    s_en["file_exists"]    = "A file with this name already exists.";
    s_en["file_fail"]      = "Could not create file.";
    s_en["home"]           = "Home";

    // Settings Dialog
    s_en["settings_title"]    = "Settings";
    s_en["settings_lang"]     = "Sprache / Language";
    s_en["settings_lang_de"]  = "Deutsch";
    s_en["settings_lang_en"]  = "English";
    s_en["settings_theme"]    = "Theme";
    s_en["settings_dark"]     = "Dark";
    s_en["settings_light"]    = "Light";
    s_en["settings_about"]    = "About WinFilesPro";
    s_en["settings_version"]  = "Version 0.3.0 · Qt6 · Open Source";
    s_en["settings_restart"]  = "Restart required for language change";
    s_en["settings_save"]     = "Save";
    s_en["settings_hidden"]   = "Show hidden files";
    s_en["settings_section_appearance"] = "Appearance";
    s_en["settings_section_language"]   = "Language";
    s_en["settings_section_info"]       = "Info";
}