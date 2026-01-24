#!/bin/bash
# Validation script for dbus-mqtt-bridge packaging
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Ed Lee
#
# This script validates the Debian packaging files for correctness.
# Run this before building the package.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

echo "========================================"
echo "dbus-mqtt-bridge Packaging Validation"
echo "========================================"
echo

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

error_count=0
warning_count=0

check_error() {
    if [ $1 -ne 0 ]; then
        echo -e "${RED}✗ FAIL${NC}: $2"
        ((error_count++))
        return 1
    else
        echo -e "${GREEN}✓ PASS${NC}: $2"
        return 0
    fi
}

check_warning() {
    echo -e "${YELLOW}⚠ WARNING${NC}: $1"
    ((warning_count++))
}

check_file_exists() {
    if [ -f "$1" ]; then
        echo -e "${GREEN}✓ PASS${NC}: File exists: $1"
        return 0
    else
        echo -e "${RED}✗ FAIL${NC}: File missing: $1"
        ((error_count++))
        return 1
    fi
}

echo "=== Checking Required Files ==="
echo

# Check debian packaging files
check_file_exists "debian/control"
check_file_exists "debian/rules"
check_file_exists "debian/changelog"
check_file_exists "debian/copyright"
check_file_exists "debian/compat"
check_file_exists "debian/postinst"
check_file_exists "debian/postrm"
check_file_exists "debian/dirs"
check_file_exists "debian/README.Debian"
check_file_exists "debian/source/format"

# Check example files
check_file_exists "examples/config.yaml"
check_file_exists "examples/dbus-policy.conf"

# Check service file
check_file_exists "dbus-mqtt-bridge.service"

echo
echo "=== Checking File Permissions ==="
echo

# Check that scripts are executable
for script in debian/rules debian/postinst debian/postrm; do
    if [ -f "$script" ]; then
        if [ -x "$script" ]; then
            echo -e "${GREEN}✓ PASS${NC}: $script is executable"
        else
            echo -e "${RED}✗ FAIL${NC}: $script is not executable (chmod +x needed)"
            ((error_count++))
        fi
    fi
done

echo
echo "=== Checking debian/control ==="
echo

if [ -f "debian/control" ]; then
    # Check for required fields
    if grep -q "^Source: dbus-mqtt-bridge" debian/control; then
        echo -e "${GREEN}✓ PASS${NC}: Source field present"
    else
        echo -e "${RED}✗ FAIL${NC}: Source field missing or incorrect"
        ((error_count++))
    fi
    
    if grep -q "^Package: dbus-mqtt-bridge" debian/control; then
        echo -e "${GREEN}✓ PASS${NC}: Package field present"
    else
        echo -e "${RED}✗ FAIL${NC}: Package field missing or incorrect"
        ((error_count++))
    fi
    
    # Check dependencies
    if grep -q "adduser" debian/control; then
        echo -e "${GREEN}✓ PASS${NC}: adduser dependency present"
    else
        check_warning "adduser not in dependencies (needed for user creation)"
    fi
    
    if grep -q "libsystemd" debian/control; then
        echo -e "${GREEN}✓ PASS${NC}: libsystemd dependency present"
    else
        echo -e "${RED}✗ FAIL${NC}: libsystemd dependency missing"
        ((error_count++))
    fi
fi

echo
echo "=== Checking debian/rules ==="
echo

if [ -f "debian/rules" ]; then
    # Check for dh_installsystemd override
    if grep -q "override_dh_installsystemd" debian/rules; then
        echo -e "${GREEN}✓ PASS${NC}: dh_installsystemd override present"
        
        if grep -q "\-\-no-enable" debian/rules; then
            echo -e "${GREEN}✓ PASS${NC}: --no-enable flag present"
        else
            echo -e "${RED}✗ FAIL${NC}: --no-enable flag missing"
            ((error_count++))
        fi
        
        if grep -q "\-\-no-start" debian/rules; then
            echo -e "${GREEN}✓ PASS${NC}: --no-start flag present"
        else
            echo -e "${RED}✗ FAIL${NC}: --no-start flag missing"
            ((error_count++))
        fi
    else
        echo -e "${RED}✗ FAIL${NC}: dh_installsystemd override missing"
        ((error_count++))
    fi
fi

echo
echo "=== Checking debian/postinst ==="
echo

if [ -f "debian/postinst" ]; then
    # Check for user creation
    if grep -q "adduser.*dbus-mqtt-bridge" debian/postinst; then
        echo -e "${GREEN}✓ PASS${NC}: User creation present"
    else
        echo -e "${RED}✗ FAIL${NC}: User creation missing"
        ((error_count++))
    fi
    
    # Check for group creation
    if grep -q "addgroup.*dbus-mqtt-bridge" debian/postinst; then
        echo -e "${GREEN}✓ PASS${NC}: Group creation present"
    else
        echo -e "${RED}✗ FAIL${NC}: Group creation missing"
        ((error_count++))
    fi
    
    # Check for #DEBHELPER# token
    if grep -q "#DEBHELPER#" debian/postinst; then
        echo -e "${GREEN}✓ PASS${NC}: #DEBHELPER# token present"
    else
        check_warning "#DEBHELPER# token missing in postinst"
    fi
    
    # Check for directory creation
    if grep -q "/etc/dbus-mqtt-bridge" debian/postinst; then
        echo -e "${GREEN}✓ PASS${NC}: Config directory creation present"
    else
        check_warning "Config directory creation not found"
    fi
    
    # Check for permission setting
    if grep -q "chmod.*0640" debian/postinst || grep -q "chmod.*640" debian/postinst; then
        echo -e "${GREEN}✓ PASS${NC}: Config file permissions set"
    else
        check_warning "Config file permissions (0640) not explicitly set"
    fi
fi

echo
echo "=== Checking debian/postrm ==="
echo

if [ -f "debian/postrm" ]; then
    # Check for user removal on purge
    if grep -q "deluser.*dbus-mqtt-bridge" debian/postrm; then
        echo -e "${GREEN}✓ PASS${NC}: User removal present"
    else
        check_warning "User removal not found in postrm"
    fi
    
    # Check for purge handling
    if grep -q "purge)" debian/postrm; then
        echo -e "${GREEN}✓ PASS${NC}: Purge case handled"
    else
        check_warning "Purge case not explicitly handled"
    fi
fi

echo
echo "=== Checking systemd service file ==="
echo

if [ -f "dbus-mqtt-bridge.service" ]; then
    # Check for security hardening
    if grep -q "^User=dbus-mqtt-bridge" dbus-mqtt-bridge.service; then
        echo -e "${GREEN}✓ PASS${NC}: Service runs as dbus-mqtt-bridge user"
    else
        echo -e "${RED}✗ FAIL${NC}: Service does not run as dbus-mqtt-bridge user"
        ((error_count++))
    fi
    
    if grep -q "^NoNewPrivileges=true" dbus-mqtt-bridge.service; then
        echo -e "${GREEN}✓ PASS${NC}: NoNewPrivileges enabled"
    else
        check_warning "NoNewPrivileges not enabled"
    fi
    
    if grep -q "^ProtectSystem=" dbus-mqtt-bridge.service; then
        echo -e "${GREEN}✓ PASS${NC}: ProtectSystem enabled"
    else
        check_warning "ProtectSystem not configured"
    fi
    
    if grep -q "^ProtectHome=" dbus-mqtt-bridge.service; then
        echo -e "${GREEN}✓ PASS${NC}: ProtectHome enabled"
    else
        check_warning "ProtectHome not configured"
    fi
    
    # Check it doesn't run as root
    if grep -q "^User=root" dbus-mqtt-bridge.service; then
        echo -e "${RED}✗ FAIL${NC}: Service configured to run as root (security issue)"
        ((error_count++))
    fi
    
    # Check for capability restrictions
    if grep -q "^CapabilityBoundingSet=" dbus-mqtt-bridge.service; then
        echo -e "${GREEN}✓ PASS${NC}: CapabilityBoundingSet configured"
    else
        check_warning "CapabilityBoundingSet not configured"
    fi
fi

echo
echo "=== Checking example config ==="
echo

if [ -f "examples/config.yaml" ]; then
    # Basic YAML syntax check (if yamllint is available)
    if command -v yamllint &> /dev/null; then
        if yamllint -d relaxed examples/config.yaml &> /dev/null; then
            echo -e "${GREEN}✓ PASS${NC}: config.yaml is valid YAML"
        else
            echo -e "${RED}✗ FAIL${NC}: config.yaml has YAML syntax errors"
            ((error_count++))
        fi
    else
        check_warning "yamllint not available, skipping YAML validation"
    fi
    
    # Check for required sections
    if grep -q "^mqtt:" examples/config.yaml; then
        echo -e "${GREEN}✓ PASS${NC}: mqtt section present"
    else
        echo -e "${RED}✗ FAIL${NC}: mqtt section missing"
        ((error_count++))
    fi
    
    # Check for security warnings
    if grep -qi "SECURITY" examples/config.yaml; then
        echo -e "${GREEN}✓ PASS${NC}: Security warnings present in config"
    else
        check_warning "No security warnings in example config"
    fi
fi

echo
echo "=== Checking D-Bus policy example ==="
echo

if [ -f "examples/dbus-policy.conf" ]; then
    # Basic XML syntax check (if xmllint is available)
    if command -v xmllint &> /dev/null; then
        if xmllint --noout examples/dbus-policy.conf &> /dev/null; then
            echo -e "${GREEN}✓ PASS${NC}: dbus-policy.conf is valid XML"
        else
            echo -e "${RED}✗ FAIL${NC}: dbus-policy.conf has XML syntax errors"
            ((error_count++))
        fi
    else
        check_warning "xmllint not available, skipping XML validation"
    fi
    
    # Check for policy user
    if grep -q 'user="dbus-mqtt-bridge"' examples/dbus-policy.conf; then
        echo -e "${GREEN}✓ PASS${NC}: Policy for dbus-mqtt-bridge user present"
    else
        echo -e "${RED}✗ FAIL${NC}: Policy for dbus-mqtt-bridge user missing"
        ((error_count++))
    fi
    
    # Check for deny-by-default
    if grep -q '<deny send_type="method_call"/>' examples/dbus-policy.conf; then
        echo -e "${GREEN}✓ PASS${NC}: Deny-by-default for method calls"
    else
        check_warning "No explicit deny-by-default for method calls"
    fi
fi

echo
echo "=== Summary ==="
echo

if [ $error_count -eq 0 ]; then
    echo -e "${GREEN}All critical checks passed!${NC}"
    if [ $warning_count -gt 0 ]; then
        echo -e "${YELLOW}$warning_count warning(s) found - review recommended${NC}"
    fi
    exit 0
else
    echo -e "${RED}$error_count error(s) found - please fix before packaging${NC}"
    if [ $warning_count -gt 0 ]; then
        echo -e "${YELLOW}$warning_count warning(s) found${NC}"
    fi
    exit 1
fi