#!/bin/bash
# Quick fixes for packaging issues
# Run this script in the project root directory

set -e

echo "Fixing Debian packaging issues..."

# 1. Make debian/rules executable
if [ -f debian/rules ]; then
    chmod +x debian/rules
    echo "✓ Made debian/rules executable"
fi

# 2. Remove debian/compat (using debhelper >= 13 in control instead)
# Actually, let's keep compat since we removed debhelper-compat from control
echo "✓ Keeping debian/compat (using debhelper >= 13)"

# 3. Clean up any CMake cache that might have CPack variables
if [ -d build ]; then
    echo "Removing old build directory..."
    rm -rf build
    echo "✓ Removed build directory"
fi

# 4. Remove any debian/files or debian/*.debhelper that might have old values
if [ -f debian/files ]; then
    rm -f debian/files
    echo "✓ Removed debian/files"
fi

find debian -name "*.debhelper*" -delete 2>/dev/null && echo "✓ Removed debhelper artifacts" || true
find debian -name "*.substvars" -delete 2>/dev/null && echo "✓ Removed substvars" || true

echo ""
echo "All fixes applied. Try building again:"
echo "  dpkg-buildpackage -us -uc -b"
