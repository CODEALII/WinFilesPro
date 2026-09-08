#!/usr/bin/env bash
# Baut einen Release mit ausführbarer Datei:
#   dist/WinFilesPro                  - rohes Linux-x86_64-Binary (vom Updater genutzt)
#   dist/WinFilesPro-<v>-x86_64.AppImage  - portables Einzeldatei-Programm
#   dist/WinFilesPro.AppDir/          - AppDir-Backup (falls AppImage nicht gebaut werden konnte)
set -euo pipefail
cd "$(dirname "$0")/.."

VERSION="$(grep -m1 'project(WinFilesPro' CMakeLists.txt | sed -n 's/.*VERSION \([0-9.]*\).*/\1/p')"
DIST="dist"
rm -rf "$DIST" build-release
mkdir -p "$DIST"

echo "==> Release-Build (0.4.x)…"
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build-release -j >/dev/null

echo "==> Roh-Binary für den Updater…"
cp build-release/WinFilesPro "$DIST/WinFilesPro"
chmod +x "$DIST/WinFilesPro"

echo "==> AppDir zusammenstellen…"
APPDIR="$DIST/WinFilesPro.AppDir"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/lib" "$APPDIR/usr/plugins" "$APPDIR/usr/share/applications" "$APPDIR/usr/share/icons/hicolor/512x512/apps"

cp build-release/WinFilesPro "$APPDIR/usr/bin/WinFilesPro"

# Benötigte Qt-Bibliotheken kopieren
for lib in $(ldd build-release/WinFilesPro | awk '/=> \/usr/ {print $3}' | sort -u); do
    cp -L "$lib" "$APPDIR/usr/lib/" 2>/dev/null || true
done

# Icon im PNG-Format erzeugen (für .desktop + AppImage).
# Hinweis: Vom ICO nur eine Einzel-Frame-Ebene verwenden, sonst schreibt
# ImageMagick bei diesem Multi-Frame-ICO erwartungswidrig keine Datei.
if command -v convert >/dev/null 2>&1; then
    (convert 'assets/icons/Explorer.ico[7]' "$APPDIR/usr/share/icons/hicolor/512x512/apps/WinFilesPro.png" ||
     convert 'assets/icons/Explorer.ico[0]' "$APPDIR/usr/share/icons/hicolor/512x512/apps/WinFilesPro.png") 2>/dev/null
fi
[ -f "$APPDIR/usr/share/icons/hicolor/512x512/apps/WinFilesPro.png" ] || cp assets/icons/Explorer.ico "$APPDIR/usr/share/icons/hicolor/512x512/apps/WinFilesPro.png"

# Qt-Plugins (Fenster-Plattform, Icons, Formate, TLS)
PLUGDIR="$(qmake6 -query QT_INSTALL_PLUGINS 2>/dev/null || echo /usr/lib/x86_64-linux-gnu/qt6/plugins)"
for d in platforms iconengines imageformats platforminputcontexts tls styles; do
    [ -d "$PLUGDIR/$d" ] && cp -r "$PLUGDIR/$d" "$APPDIR/usr/plugins/" || true
done

cat > "$APPDIR/WinFilesPro.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=WinFilesPro
Comment=File Manager
Exec=WinFilesPro
Icon=WinFilesPro
Categories=Utility;FileTools;
EOF
cp "$APPDIR/usr/share/icons/hicolor/512x512/apps/WinFilesPro.png" "$APPDIR/WinFilesPro.png" 2>/dev/null || true
cp "$APPDIR/WinFilesPro.desktop" "$APPDIR/usr/share/applications/"

cat > "$APPDIR/AppRun" <<'EOF'
#!/usr/bin/env bash
HERE="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$HERE/usr/plugins"
exec "$HERE/usr/bin/WinFilesPro" "$@"
EOF
chmod +x "$APPDIR/AppRun"

echo "==> AppImage bauen…"
APPIMAGETOOL="$(command -v appimagetool || echo "$HOME/.local/bin/appimagetool")"
if [ ! -x "$APPIMAGETOOL" ]; then
    echo "    lade appimagetool herunter…"
    mkdir -p "$HOME/.local/bin"
    if ! curl -fsSL -o "$HOME/.local/bin/appimagetool.tmp" \
        "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"; then
        echo "!! appimagetool-Download fehlgeschlagen -> nur portables AppDir/Tar"
        APPIMAGETOOL=""
    else
        chmod +x "$HOME/.local/bin/appimagetool.tmp"
        mv "$HOME/.local/bin/appimagetool.tmp" "$HOME/.local/bin/appimagetool"
    fi
fi

if [ -n "$APPIMAGETOOL" ]; then
    if "$APPIMAGETOOL" --appimage-extract-and-run "$APPDIR" "$DIST/WinFilesPro-${VERSION}-x86_64.AppImage" 2>/dev/null; then
        rm -rf "$APPDIR"
        echo "==> AppImage erstellt."
    else
        echo "!! AppImage fehlgeschlagen, portables Paket wird erstellt."
    fi
fi

if [ -d "$APPDIR" ]; then
    tar -C "$DIST" -czf "$DIST/WinFilesPro-${VERSION}-Linux-x86_64.tar.gz" WinFilesPro.AppDir
    echo "==> Portables Paket erstellt (entpacken, dann ./WinFilesPro.AppDir/AppRun)."
fi

echo "==> Fertig in $DIST:"
ls -lh "$DIST" | grep -v build-release