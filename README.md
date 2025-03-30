# Codebase Exporter

A simple GTK application designed to help you easily select and export the content of multiple code files from a project directory into a single markdown file.

This tool is particularly useful when you need to consolidate code from various files into one block, for example, when providing context to AI language models or for documentation purposes.

## Features

*   Graphical interface (GTK) for selecting a project directory.
*   Automatically detects common project types (Java, Python, C/C++, etc.) to pre-select relevant file extensions.
*   Allows manual selection/deselection of individual files.
*   Exports the content of selected files into a single markdown file (`custom-codebase.md` in the directory specified in the source - *Note: This should ideally be changed to a save dialog in future versions*).
*   Copies the generated markdown content to the clipboard for convenience.
*   Remembers the last used directory for quicker access.

## Installation

This application is installed from source. You need `git`, a C compiler (like `gcc`), `make`, and the GTK3 development libraries.

**1. Install Dependencies:**

*   **Arch Linux:**
    ```bash
    sudo pacman -S base-devel git gtk3
    ```
*   **Debian/Ubuntu:**
    ```bash
    sudo apt update
    sudo apt install build-essential git libgtk-3-dev
    ```
*   **Fedora:**
    ```bash
    sudo dnf groupinstall "Development Tools"
    sudo dnf install git gtk3-devel
    ```

**2. Clone the Repository:**

```bash
git clone https://github.com/kedimuzafer/codebase-exporter
cd codebase-exporter
```

**3. Run the Installation Script:**

This script compiles the application and installs it locally for your user (`~/.local/bin`).

```bash
bash install.sh
```

The script will also add `~/.local/bin` to your PATH in `.bashrc` and `.zshrc` if it's not already present.

## Usage

After installation, you can typically launch the application from your desktop environment's application menu (search for "Codebase Exporter").

Alternatively, you can run it from the terminal:

```bash
codebase-exporter
```

1.  Use the "Browse" button to select your project directory.
2.  The application will load recognized code files.
3.  Select/deselect files using the checkboxes.
4.  Click "Save to Markdown & Copy" to generate the output file and copy the content to your clipboard.

## Uninstallation

To remove the application, run the following commands (as shown by the install script):

```bash
rm -f "$HOME/.local/bin/codebase-exporter"
rm -f "$HOME/.local/share/applications/codebase-exporter.desktop"
rm -f "$HOME/.local/share/icons/hicolor/scalable/apps/codebase-exporter.svg"
# Optionally remove the config directory if you don't need the last used path saved
rm -rf "$HOME/.config/codebase-exporter"
```

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues.

## License

This project is licensed under the MIT License.