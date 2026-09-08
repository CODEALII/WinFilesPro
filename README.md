# WinFilesPro

WinFilesPro is a modern, cross-platform file manager built with Qt6 and C++. It aims to provide a fast, efficient, and user-friendly experience for managing your files.

## Features

*   **Standard File Operations:** Copy, cut, paste, delete, and rename files and folders.
*   **Navigation:** Intuitive navigation with back, forward, and up buttons, as well as a breadcrumb bar for quick directory access.
*   **Tabbed Interface:** Manage multiple directories simultaneously with a tabbed browsing experience.
*   **Sidebar:** Quick access to common locations and drives.
*   **Search Functionality:** Efficiently locate files and folders within the current directory.
*   **Customizable Themes:** Supports light and dark themes, with a custom "Fusion" style to ensure a consistent look across different desktop environments.
*   **Internationalization (i18n):** Language support for a global user base.
*   **Settings Management:** Persistent settings for window size, position, theme, and language.
*   **Hidden Files Toggle:** Option to show or hide hidden files.
*   **Context Menus:** Right-click context menus for quick access to file actions.
*   **Terminal Integration:** Option to open the current directory in a terminal.
*   **New File/Folder Creation:** Easily create new files and folders.

## Building from Source

To build WinFilesPro from source, you will need CMake and Qt6.

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/CODEALII/WinFilesPro.git
    cd WinFilesPro
    ```

2.  **Create a build directory and configure CMake:**
    ```bash
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    ```

3.  **Build the project using Ninja:**
    ```bash
    ninja
    ```

    Alternatively, you can use `make` if Ninja is not installed:
    ```bash
    make
    ```

## Usage

After a successful build, the executable `WinFilesPro` will be located in the `build` directory.

```bash
./WinFilesPro
```

The application will launch, allowing you to browse and manage your files.

## Dependencies

*   Qt 6 (Widgets module)
*   CMake (version 3.16 or higher)
*   C++17 compatible compiler (e.g., GCC, Clang, MSVC)

## Contributing

If you wish to contribute to WinFilesPro, please feel free to fork the repository and submit pull requests.

## License

This project is licensed under the GNU General Public License v3.0 - see the LICENSE file for details.
