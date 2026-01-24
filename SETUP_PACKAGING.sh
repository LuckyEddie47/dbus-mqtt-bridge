#!/bin/bash
# Setup script for dbus-mqtt-bridge Debian packaging
# This script sets up all the files and permissions correctly
# Run from the project root directory

set -e

PROJECT_ROOT="$(pwd)"
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "========================================="
echo "dbus-mqtt-bridge Packaging Setup"
echo "========================================="
echo

# Function to check if file exists
check_file() {
    if [ -f "$1" ]; then
        echo -e "${GREEN}✓${NC} $1 exists"
        return 0
    else
        echo -e "${RED}✗${NC} $1 missing"
        return 1
    fi
}

# Function to create directory if needed
ensure_dir() {
    if [ ! -d "$1" ]; then
        mkdir -p "$1"
        echo -e "${GREEN}✓${NC} Created $1"
    else
        echo -e "${GREEN}✓${NC} $1 exists"
    fi
}

echo "Step 1: Checking required directories..."
ensure_dir "debian"
ensure_dir "debian/source"
ensure_dir "examples"
echo

echo "Step 2: Checking required files..."
MISSING_FILES=0

# Core files
check_file "CMakeLists.txt" || ((MISSING_FILES++))
check_file "dbus-mqtt-bridge.service" || ((MISSING_FILES++))

# Debian packaging files
check_file "debian/control" || ((MISSING_FILES++))
check_file "debian/rules" || ((MISSING_FILES++))
check_file "debian/changelog" || ((MISSING_FILES++))
check_file "debian/copyright" || ((MISSING_FILES++))
check_file "debian/postinst" || ((MISSING_FILES++))
check_file "debian/postrm" || ((MISSING_FILES++))
check_file "debian/dirs" || ((MISSING_FILES++))
check_file "debian/install" || ((MISSING_FILES++))
check_file "debian/README.Debian" || ((MISSING_FILES++))
check_file "debian/source/format" || ((MISSING_FILES++))

# Example files
check_file "examples/config.yaml" || ((MISSING_FILES++))
check_file "examples/dbus-policy.conf" || ((MISSING_FILES++))

echo

if [ $MISSING_FILES -gt 0 ]; then
    echo -e "${RED}ERROR: $MISSING_FILES files are missing${NC}"
    echo "Please ensure all artifact files are copied to the project."
    exit 1
fi

echo "Step 3: Fixing file permissions..."

# Make debian/rules executable
if [ -f debian/rules ]; then
    chmod +x debian/rules
    echo -e "${GREEN}✓${NC} debian/rules is executable"
fi

# Make maintainer scripts executable
for script in debian/postinst debian/postrm debian/preinst debian/prerm; do
    if [ -f "$script" ]; then
        chmod +x "$script"
        echo -e "${GREEN}✓${NC} $script is executable"
    fi
done

echo

echo "Step 4: Removing conflicting files..."

# Remove debian/compat if it exists (we use debhelper-compat in control)
if [ -f debian/compat ]; then
    rm -f debian/compat
    echo -e "${GREEN}✓${NC} Removed debian/compat (using debhelper-compat)"
else
    echo -e "${GREEN}✓${NC} No debian/compat to remove"
fi

# Clean up any existing build artifacts
if [ -d build ]; then
    echo -e "${YELLOW}⚠${NC}  Removing existing build directory..."
    rm -rf build
fi

if [ -f debian/files ]; then
    rm -f debian/files
    echo -e "${GREEN}✓${NC} Removed debian/files"
fi

# Remove debhelper artifacts
find debian -name "*.debhelper*" -delete 2>/dev/null || true
find debian -name "*.substvars" -delete 2>/dev/null || true
find debian -type d -name "dbus-mqtt-bridge" -exec rm -rf {} + 2>/dev/null || true
find debian -type d -name ".debhelper" -exec rm -rf {} + 2>/dev/null || true

echo -e "${GREEN}✓${NC} Cleaned build artifacts"

echo

echo "Step 5: Validating debian/control..."

if grep -q "debhelper-compat (= 13)" debian/control; then
    echo -e "${GREEN}✓${NC} debhelper-compat found in debian/control"
else
    echo -e "${RED}✗${NC} debhelper-compat missing from debian/control"
    echo "Please ensure debian/control has:"
    echo "  Build-Depends: debhelper-compat (= 13), ..."
    exit 1
fi

echo

echo "Step 6: Checking source files..."
SRC_FILES=0
if [ -d src ]; then
    SRC_FILES=$(find src -name "*.cpp" | wc -l)
    echo -e "${GREEN}✓${NC} Found $SRC_FILES source files"
else
    echo -e "${YELLOW}⚠${NC}  src/ directory not found"
fi

if [ -d include ]; then
    HEADER_FILES=$(find include -name "*.hpp" | wc -l)
    echo -e "${GREEN}✓${NC} Found $HEADER_FILES header files"
else
    echo -e "${YELLOW}⚠${NC}  include/ directory not found"
fi

echo

echo "Step 7: Summary"
echo "==============="

echo -e "${GREEN}✓${NC} All Debian packaging files present"
echo -e "${GREEN}✓${NC} File permissions set correctly"
echo -e "${GREEN}✓${NC} No conflicting files (debian/compat removed)"
echo -e "${GREEN}✓${NC} Build artifacts cleaned"

echo
echo "========================================="
echo "Setup Complete!"
echo "========================================="
echo
echo "Next steps:"
echo "  1. Review files in debian/ directory"
echo "  2. Update debian/changelog if needed:"
echo "     dch -i"
echo "  3. Build the package:"
echo "     dpkg-buildpackage -us -uc -b"
echo "  4. Install and test:"
echo "     sudo dpkg -i ../dbus-mqtt-bridge_*.deb"
echo
echo "For troubleshooting, see BUILD_TROUBLESHOOTING.md"
echo

exit 0