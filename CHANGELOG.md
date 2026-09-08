# WinFilesPro Changelog

## [0.4.2] - 2026-09-08

### Fixes
- **Elevated (root) relaunch fixed for real**: the OS password dialog (pkexec) now passes the full display environment (DISPLAY/WAYLAND_DISPLAY/XDG_RUNTIME_DIR/DBUS_SESSION_BUS_ADDRESS/PATH) to the restarted instance, and the old window closes automatically only once the elevated instance has confirmed it is running. Cancel or failed auth keeps the old window open, so the permission-error loop is gone
- Fixed a startup crash caused by an uninitialized sidebar pointer

### Features
- New General settings section: restore last folder on start, mouse side-button navigation, delete confirmation and animations can be toggled
- Duplicate the current tab (context menu on tabs or Ctrl+Shift+D)
- Open a folder in a new tab from its context menu
- Create ZIP archives and extract ZIP files via context menu (Linux)
- Folder status bar now shows the free space of the current drive

### Smoother UI
- Fade-in on start, animated admin toast, fade transition between tabs

## [0.4.1] - 2026-09-08

### Fixes
- Settings now persist across restarts: hidden-files toggle, sort column/order and last visited folder are restored
- File list defaults to name sorting with folders first
- "Date modified" column got right-side padding
- Delete moves items to the Trash (with metadata), empty-trash action in the sidebar context menu
- Protected (write-only) folders now trigger the admin toast; failed create operations show the exact OS error reason and offer elevation
- Mouse side buttons (XButton1/XButton2) navigate back/forward

## [0.4.0] - 2026-09-08

### Features
- App icon set to Explorer icon for window, taskbar and Windows executable
- All disk partitions shown under "This PC" (not just root/media/mnt mounts)
- "Date modified" column now right-aligned in the file view
- Protected folders (requiring administrator rights) show a toast notification; clicking it reopens the app with elevated rights via the OS password dialog
- Folders can be pinned to Quick Access via the context menu or by dragging them onto the sidebar; pins persist across restarts and can be removed via context menu

## [0.3.0] - 2026-09-08

### Security
- **CRITICAL**: Fixed path traversal vulnerability in file/folder naming
- **CRITICAL**: Fixed unsafe path concatenation in copy/paste operations
- **HIGH**: Fixed TOCTOU race condition in cut operation
- **HIGH**: Fixed invalid trash path on non-Linux systems
- Added comprehensive input validation for all user-provided names
- Added file existence checks before all file operations

### Bug Fixes
- Fixed search filter corruption when clearing search text
- Fixed unbounded history stack growth (now capped at 100 entries)
- Fixed null pointer dereference risks in status label
- Fixed breadcrumb path construction with unusual path formats
- Improved error handling for failed file operations
- Fixed paste operation to check for existing files before overwriting

### Features
- Added file overwrite confirmation in paste operation
- Added detailed error messages for failed operations
- Improved cross-platform compatibility (Linux, macOS, Windows)

### Improvements
- Better error reporting to users for all file operations
- More robust path handling using Qt's safe methods
- Improved status bar updates with null checks

---

## [0.2.0] - Previous Release

### Added
- Multi-tab file browsing
- Settings dialog with theme and language options
- Internationalization (German and English)
- Custom Windows 11-style theming
- Drag and drop support

### Features
- File operations: copy, cut, paste, delete, rename
- File search with live filter
- Breadcrumb navigation
- Quick access sidebar
- Context menus
- Terminal integration

---

## [0.1.0] - Initial Release

### Initial Features
- Basic file browser
- File viewing and operations
- Simple UI

---

## Security Updates History

### v0.3.0 - Complete Security Audit
- Comprehensive security review performed
- 17 potential vulnerabilities identified
- 14+ vulnerabilities fixed and hardened
- Application considered stable for general use

---

## Known Issues

### Windows Platform
- Trash/Recycle Bin integration not yet implemented
- Limited to standard file operations

### Performance
- Large directory listings may experience slight lag
- Search is shallow (current directory only)

---

## Upcoming Features (Future)

- [ ] Recursive file search with async processing
- [ ] Undo/Redo functionality
- [ ] File preview pane
- [ ] Bookmarks system
- [ ] Batch file operations
- [ ] File compression support
- [ ] Integration with file archivers

---

**Current Version**: 0.4.2 (Stable Release)  
**Release Date**: 2026-09-08  
**Maintenance**: Active
