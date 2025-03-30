#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}Starting CodebaseExporter installation...${NC}"

# Check for required packages
echo "Checking for GTK3 development libraries..."
if ! pkg-config --exists gtk+-3.0; then
    echo -e "${RED}GTK3 development libraries not found.${NC}"
    echo "Please run the following command:"
    echo "  sudo pacman -S gtk3"
    exit 1
fi

# Compile the application
echo "Compiling the application..."
make clean && make
if [ $? -ne 0 ]; then
    echo -e "${RED}Error during compilation.${NC}"
    exit 1
fi

# Create installation directories
echo "Preparing installation directories..."
INSTALL_DIR="$HOME/.local/bin"
DESKTOP_DIR="$HOME/.local/share/applications"
ICON_DIR="$HOME/.local/share/icons/hicolor/scalable/apps"

mkdir -p "$INSTALL_DIR"
mkdir -p "$DESKTOP_DIR"
mkdir -p "$ICON_DIR"

# Copy the application
echo "Copying the application to $INSTALL_DIR..."
cp codebase-exporter "$INSTALL_DIR/codebase-exporter"
chmod +x "$INSTALL_DIR/codebase-exporter"

# Create the SVG icon
echo "Creating the icon..."
cat > "$ICON_DIR/codebase-exporter.svg" << EOF
<svg width="256px" height="256px" viewBox="-60 -60 520.00 520.00" fill="none" xmlns="http://www.w3.org/2000/svg" transform="rotate(0)matrix(1, 0, 0, 1, 0, 0)"><g id="SVGRepo_bgCarrier" stroke-width="0" transform="translate(0,0), scale(1)"><path transform="translate(-60, -60), scale(32.5)" fill="#fec958" d="M9.166.33a2.25 2.25 0 00-2.332 0l-5.25 3.182A2.25 2.25 0 00.5 5.436v5.128a2.25 2.25 0 001.084 1.924l5.25 3.182a2.25 2.25 0 002.332 0l5.25-3.182a2.25 2.25 0 001.084-1.924V5.436a2.25 2.25 0 00-1.084-1.924L9.166.33z" strokewidth="0"></path></g><g id="SVGRepo_tracerCarrier" stroke-linecap="round" stroke-linejoin="round" stroke="#CCCCCC" stroke-width="24"></g><g id="SVGRepo_iconCarrier"> <path d="M97.8357 54.6682C177.199 59.5311 213.038 52.9891 238.043 52.9891C261.298 52.9891 272.24 129.465 262.683 152.048C253.672 173.341 100.331 174.196 93.1919 165.763C84.9363 156.008 89.7095 115.275 89.7095 101.301" stroke="#000000" stroke-opacity="0.9" stroke-width="12.8" stroke-linecap="round" stroke-linejoin="round"></path> <path d="M98.3318 190.694C-10.6597 291.485 121.25 273.498 148.233 295.083" stroke="#000000" stroke-opacity="0.9" stroke-width="12.8" stroke-linecap="round" stroke-linejoin="round"></path> <path d="M98.3301 190.694C99.7917 213.702 101.164 265.697 100.263 272.898" stroke="#000000" stroke-opacity="0.9" stroke-width="12.8" stroke-linecap="round" stroke-linejoin="round"></path> <path d="M208.308 136.239C208.308 131.959 208.308 127.678 208.308 123.396" stroke="#000000" stroke-opacity="0.9" stroke-width="12.8" stroke-linecap="round" stroke-linejoin="round"></path> <path d="M177.299 137.271C177.035 133.883 177.3 126.121 177.3 123.396" stroke="#000000" stroke-opacity="0.9" stroke-width="12.8" stroke-linecap="round" stroke-linejoin="round"></path> <path d="M203.398 241.72C352.097 239.921 374.881 226.73 312.524 341.851" stroke="#000000" stroke-opacity="0.9" stroke-width="12.8" stroke-linecap="round" stroke-linejoin="round"></path> <path d="M285.55 345.448C196.81 341.85 136.851 374.229 178.223 264.504" stroke="#000000" stroke-opacity="0.9" stroke-width="12.8" stroke-linecap="round" stroke-linejoin="round"></path> <path d="M180.018 345.448C160.77 331.385 139.302 320.213 120.658 304.675" stroke="#000000" stroke-opacity="0.9" stroke-width="12.8" stroke-linecap="round" stroke-linejoin="round"></path> <path d="M218.395 190.156C219.024 205.562 219.594 220.898 219.594 236.324" stroke="#000000" stroke-opacity="0.9" stroke-width="12.8" stroke-linecap="round" stroke-linejoin="round"></path> <path d="M218.395 190.156C225.896 202.037 232.97 209.77 241.777 230.327" stroke="#000000" stroke-opacity="0.9" stroke-width="12.8" stroke-linecap="round" stroke-linejoin="round"></path> <path d="M80.1174 119.041C75.5996 120.222 71.0489 119.99 66.4414 120.41" stroke="#000000" stroke-opacity="0.9" stroke-width="12.8" stroke-linecap="round" stroke-linejoin="round"></path> <path d="M59.5935 109.469C59.6539 117.756 59.5918 125.915 58.9102 134.086" stroke="#000000" stroke-opacity="0.9" stroke-width="12.8" stroke-linecap="round" stroke-linejoin="round"></path> <path d="M277.741 115.622C281.155 115.268 284.589 114.823 287.997 114.255" stroke="#000000" stroke-opacity="0.9" stroke-width="12.8" stroke-linecap="round" stroke-linejoin="round"></path> <path d="M291.412 104.682C292.382 110.109 292.095 115.612 292.095 121.093" stroke="#000000" stroke-opacity="0.9" stroke-width="12.8" stroke-linecap="round" stroke-linejoin="round"></path> <path d="M225.768 116.466C203.362 113.993 181.657 115.175 160.124 118.568" stroke="#000000" stroke-opacity="0.9" stroke-width="12.8" stroke-linecap="round" stroke-linejoin="round"></path> </g></svg>
EOF

# Create the .desktop file
echo "Creating the .desktop file..."
cat > "$DESKTOP_DIR/codebase-exporter.desktop" << EOF
[Desktop Entry]
Name=Codebase Exporter
Comment=Export all code files in a folder to a single text file
Exec=$INSTALL_DIR/codebase-exporter
Icon=$ICON_DIR/codebase-exporter.svg
Terminal=false
Type=Application
Categories=Utility;Development;
EOF

# Check if installation directory is in PATH
if [[ ":$PATH:" != *":$INSTALL_DIR:"* ]]; then
    echo -e "${GREEN}Adding $INSTALL_DIR to PATH...${NC}"
    echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.bashrc"
    echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.zshrc" 2>/dev/null || true
    
    # Update PATH for the current session
    export PATH="$HOME/.local/bin:$PATH"
    echo -e "${GREEN}PATH updated. It will be active in new terminals or after sourcing the config file.${NC}"
fi

echo -e "${GREEN}Installation complete!${NC}"
echo "You can launch the application from your application menu ('Codebase Exporter') or by typing 'codebase-exporter' in the terminal."
echo "To uninstall: rm -rf $INSTALL_DIR/codebase-exporter $DESKTOP_DIR/codebase-exporter.desktop $ICON_DIR/codebase-exporter.svg"
