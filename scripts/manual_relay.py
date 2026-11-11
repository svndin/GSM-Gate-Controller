#!/usr/bin/env python3
"""Manually pulse one active-low relay."""

import sys
from time import sleep
from gpiozero import OutputDevice

PINS = {"open": 27, "close": 17}

if len(sys.argv) != 2 or sys.argv[1] not in PINS:
    sys.exit("Usage: python3 manual_relay.py open|close")

relay = OutputDevice(
    PINS[sys.argv[1]],
    active_high=False,
    initial_value=False
)

try:
    print(f"Triggering {sys.argv[1]} relay...")
    relay.on()
    sleep(0.2)
finally:
    relay.off()
    relay.close()

print("Relay released.")
