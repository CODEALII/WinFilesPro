# WinFilesPro

A fast, modern, cross-platform file manager built with **Qt 6** and **C++17**.

WinFilesPro combines the familiar Windows-Explorer interaction model with a modern, minimal interface that is fully styled and themable. It is developed on Linux and runs on any desktop environment that supports Qt 6.

---

## Features

### File Management
- Copy, cut, paste, rename and delete files and folders (delete moves to the Trash; items inside the Trash are removed permanently).
- Create new files and folders with a modern, searchable template dialog that covers over 50 file types, each with its own icon and color.
- Compress and extract ZIP archives directly from the context menu.
- Integrated, modern text viewer with line numbers for source code, config and Markdown files (open is redirected to the external application only for file types that are not text or are too large).
- Drag & drop with cut/copy handling.

### Navigation
- Back, forward and up navigation with history, plus mouse side buttons.
- Editable address bar: breadcrumb navigation for direct browsing, or press `Ctrl+L` to type a path.
- Tabbed browsing with an always-visible tab bar, tab duplication and per-tab context menus.
- Sidebar with Quick Access, this computer, all mounted drives and network locations.
- "This PC" overview with all local disks, partitions, connected devices and user-defined network drives.

### Appearance & Control
- Fully custom Qt "Fusion" styling with light and dark themes. Desktop themes (Yaru, GTK, KDE) are deliberately blocked so the application looks identical everywhere.
- Adjustable visibility of the navigation, commands and tab bars, configured in the settings.
- Optional gray rendering of hidden files when they are shown.
- Pointer cursor over all interactive elements.
- Size column left-aligned; dates right-aligned; units configured as Windows-style (KB, MB, GB), binary (KiB, MiB, GiB) or SI (kB, MB, GB).

### Integration & Configuration
- Internationalization with German and English; switching the language applies immediately.
- Persistent settings for theme, language, window geometry, hidden-file visibility, sorting, toolbar layout and more.
- File-type associations: pick "Open with" from the context menu and remember the program per file type; manage all associations in a dedicated settings tab.
- Image actions: convert images to PNG from the context menu, optionally deleting the original file.
- Open a terminal in any folder; one-click elevation for write-protected folders.
- Settings are stored per user (QSettings) and survive restarts.

---

## Building from Source

Requirements:

- Qt 6 (Widgets module)
- CMake 3.16 or newer
- A C++17 compiler (GCC, Clang, MSVC)
- Optional: ImageMagick (`convert`/`magick`) for image conversion, `zip`/`unzip` for archives

```bash
git clone https://github.com/CODEALII/WinFilesPro.git
cd WinFilesPro
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
```

The executable is written to `build/WinFilesPro`.

## Running

```bash
./build/WinFilesPro
```

Or open a specific directory:

```bash
./build/WinFilesPro /path/to/directory
```

## Keyboard Shortcuts

| Shortcut               | Action                          |
| ---------------------- | ------------------------------- |
| `F5`                   | Refresh                         |
| `F2`                   | Rename                          |
| `Del`                  | Delete                          |
| `Ctrl+C` / `Ctrl+X`    | Copy / Cut                      |
| `Ctrl+V`               | Paste                           |
| `Ctrl+T`               | New tab                         |
| `Ctrl+W`               | Close tab                       |
| `Ctrl+H`               | Toggle hidden files             |
| `Ctrl+F`               | Show search bar                 |
| `Ctrl+L`               | Edit address bar                |
| `Alt+Left/Right/Up`    | Back / Forward / Up             |

## Resources

- [Source Code](https://github.com/CODEALII/WinFilesPro)
- [Issue Tracker](https://github.com/CODEALII/WinFilesPro/issues)

## Contributing

Contributions are welcome. Fork the repository, make your change, and submit a pull request. Please keep the code style consistent with the existing sources and avoid unrelated formatting changes.

## License

This project is licensed under the [GNU General Public License v3.0](LICENSE).