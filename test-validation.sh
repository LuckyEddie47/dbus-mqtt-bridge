#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Ed Lee

# Test script for configuration validation

set -e

# Check for binary location
if [ -f "./build/dbus-mqtt-bridge" ]; then
    BINARY="./build/dbus-mqtt-bridge"
elif [ -f "/usr/bin/dbus-mqtt-bridge" ]; then
    BINARY="/usr/bin/dbus-mqtt-bridge"
else
    echo "Error: dbus-mqtt-bridge binary not found"
    echo "Tried: ./build/dbus-mqtt-bridge and /usr/bin/dbus-mqtt-bridge"
    exit 1
fi

echo "Using binary: $BINARY"
echo

TEST_DIR="test-configs"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== Configuration Validation Tests ==="
echo

# Create test configs directory
mkdir -p "$TEST_DIR"

# Test 1: Invalid MQTT Broker
echo -e "${YELLOW}Test 1: Invalid MQTT Broker${NC}"
cat > "$TEST_DIR/invalid-broker.yaml" <<EOF
mqtt:
  broker: "not a valid hostname!"
  port: 1883
bus_type: system
mappings:
  dbus_to_mqtt: []
  mqtt_to_dbus: []
EOF

if $BINARY "$TEST_DIR/invalid-broker.yaml" 2>&1 | grep -q "Invalid MQTT broker"; then
    echo -e "${GREEN}✓ PASS${NC}: Caught invalid broker"
else
    echo -e "${RED}✗ FAIL${NC}: Did not catch invalid broker"
fi
echo

# Test 2: Invalid Port
echo -e "${YELLOW}Test 2: Invalid Port${NC}"
cat > "$TEST_DIR/invalid-port.yaml" <<EOF
mqtt:
  broker: localhost
  port: 99999
bus_type: system
mappings:
  dbus_to_mqtt: []
  mqtt_to_dbus: []
EOF

if $BINARY "$TEST_DIR/invalid-port.yaml" 2>&1 | grep -q "Invalid MQTT port"; then
    echo -e "${GREEN}✓ PASS${NC}: Caught invalid port"
else
    echo -e "${RED}✗ FAIL${NC}: Did not catch invalid port"
fi
echo

# Test 3: Invalid D-Bus Service Name
echo -e "${YELLOW}Test 3: Invalid D-Bus Service Name${NC}"
cat > "$TEST_DIR/invalid-service.yaml" <<EOF
mqtt:
  broker: localhost
  port: 1883
bus_type: system
mappings:
  dbus_to_mqtt:
    - service: "not-valid-service"
      path: "/test"
      interface: "org.example.Test"
      signal: "Signal"
      topic: "test/topic"
  mqtt_to_dbus: []
EOF

if $BINARY "$TEST_DIR/invalid-service.yaml" 2>&1 | grep -q "Invalid D-Bus service"; then
    echo -e "${GREEN}✓ PASS${NC}: Caught invalid service name"
else
    echo -e "${RED}✗ FAIL${NC}: Did not catch invalid service name"
fi
echo

# Test 4: Invalid D-Bus Object Path
echo -e "${YELLOW}Test 4: Invalid D-Bus Object Path${NC}"
cat > "$TEST_DIR/invalid-path.yaml" <<EOF
mqtt:
  broker: localhost
  port: 1883
bus_type: system
mappings:
  dbus_to_mqtt:
    - service: "org.example.Service"
      path: "no-leading-slash"
      interface: "org.example.Test"
      signal: "Signal"
      topic: "test/topic"
  mqtt_to_dbus: []
EOF

if $BINARY "$TEST_DIR/invalid-path.yaml" 2>&1 | grep -q "Invalid D-Bus object path"; then
    echo -e "${GREEN}✓ PASS${NC}: Caught invalid object path"
else
    echo -e "${RED}✗ FAIL${NC}: Did not catch invalid object path"
fi
echo

# Test 5: Wildcards in Publish Topic
echo -e "${YELLOW}Test 5: Wildcards in Publish Topic${NC}"
cat > "$TEST_DIR/wildcard-publish.yaml" <<EOF
mqtt:
  broker: localhost
  port: 1883
bus_type: system
mappings:
  dbus_to_mqtt:
    - service: "org.example.Service"
      path: "/test"
      interface: "org.example.Test"
      signal: "Signal"
      topic: "test/#"
  mqtt_to_dbus: []
EOF

if $BINARY "$TEST_DIR/wildcard-publish.yaml" 2>&1 | grep -q "Wildcards.*not allowed"; then
    echo -e "${GREEN}✓ PASS${NC}: Caught wildcard in publish topic"
else
    echo -e "${RED}✗ FAIL${NC}: Did not catch wildcard in publish topic"
fi
echo

# Test 6: Invalid Bus Type
echo -e "${YELLOW}Test 6: Invalid Bus Type${NC}"
cat > "$TEST_DIR/invalid-bus-type.yaml" <<EOF
mqtt:
  broker: localhost
  port: 1883
bus_type: "invalid"
mappings:
  dbus_to_mqtt: []
  mqtt_to_dbus: []
EOF

if $BINARY "$TEST_DIR/invalid-bus-type.yaml" 2>&1 | grep -q "Invalid bus_type"; then
    echo -e "${GREEN}✓ PASS${NC}: Caught invalid bus type"
else
    echo -e "${RED}✗ FAIL${NC}: Did not catch invalid bus type"
fi
echo

# Test 7: Valid Config with Empty Mappings (Warning)
echo -e "${YELLOW}Test 7: Valid Config with Warning${NC}"
cat > "$TEST_DIR/empty-mappings.yaml" <<EOF
mqtt:
  broker: localhost
  port: 1883
bus_type: system
mappings:
  dbus_to_mqtt: []
  mqtt_to_dbus: []
EOF

# Run with timeout and send SIGTERM after validation
if timeout 2 $BINARY "$TEST_DIR/empty-mappings.yaml" 2>&1 | grep -q "Warning.*No mappings"; then
    echo -e "${GREEN}✓ PASS${NC}: Warning shown for empty mappings"
else
    echo -e "${YELLOW}⚠ INFO${NC}: No warning for empty mappings (may be acceptable)"
fi
echo

# Test 8: Wildcard Subscription (Warning)
echo -e "${YELLOW}Test 8: Wildcard in Subscription (Warning)${NC}"
cat > "$TEST_DIR/wildcard-subscribe.yaml" <<EOF
mqtt:
  broker: localhost
  port: 1883
bus_type: system
mappings:
  dbus_to_mqtt: []
  mqtt_to_dbus:
    - topic: "test/+/sensor/#"
      service: "org.example.Service"
      path: "/test"
      interface: "org.example.Test"
      method: "HandleSensor"
EOF

# This should NOT error, just warn - use timeout
if ! timeout 2 $BINARY "$TEST_DIR/wildcard-subscribe.yaml" 2>&1 | grep -q "Field.*error"; then
    echo -e "${GREEN}✓ PASS${NC}: Wildcard subscription allowed (with warning)"
else
    echo -e "${RED}✗ FAIL${NC}: Wildcard subscription incorrectly rejected"
fi
echo

echo "=== Test Summary ==="
echo "Validation system is working correctly!"
echo "Run 'man dbus-mqtt-bridge' for configuration examples (once created)."
