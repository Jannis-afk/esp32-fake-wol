# ESP32 Smart PC Controller - Wiring Diagram

## Overview
This document shows how to wire the ESP32 to your PC motherboard for smart power control using a minimal 4-wire configuration.

## 4-Wire Configuration Benefits

- **Minimal Wiring**: Only 4 wires needed for full functionality
- **Clean Installation**: No extra ground wires or complex routing
- **USB Power**: Reliable 5V power from motherboard USB header
- **Floating GPIO**: GPIO2 doesn't interfere with normal operation
- **Voltage Sensing**: GPIO4 accurately reads power LED state
- **Easy Setup**: Simple connections with clear pin assignments

## Power Supply
- **ESP32 VIN**: Connect to 5V from spare motherboard USB header
- **ESP32 GND**: Connect to GND from same USB header
- **ESP32 3.3V**: Not used (internal regulator will provide 3.3V from 5V input)

## PC Power Switch Connection
- **ESP32 GPIO2**: Connect to `power_sw+` pin on motherboard front panel header
- **Logic**: ESP32 pin is normally floating (high impedance), then temporarily pulls LOW to simulate pressing the power button
- **Note**: The ESP32 pin starts as INPUT (floating), changes to OUTPUT only when needed, then returns to INPUT

## PC Power LED Connection
- **ESP32 GPIO4**: Connect to power LED wire from motherboard
- **Voltage Levels**:
  - **5V** = PC is OFF
  - **3.3V** = PC is ON
- **Note**: This pin is configured as an analog input to read voltage levels

## ESP32 Status Indicators
- **ESP32 GPIO5**: Status LED (blinks to show ESP32 is running)
- **ESP32 GPIO15**: WOL LED (blinks when Wake-on-LAN packet is detected)

## Complete Pin Assignment (4-Wire Configuration)

| ESP32 Pin | Function | Connection | Notes |
|-----------|----------|------------|-------|
| VIN | Power Input | 5V from USB header | Main power supply |
| GND | Ground | GND from USB header | Common ground |
| GPIO2 | Power Switch | power_sw+ | Normally floating, pulls LOW when needed |
| GPIO4 | Power LED Monitor | Power LED wire | Analog input (5V=OFF, 3.3V=ON) |

**Optional Pins:**
| ESP32 Pin | Function | Connection | Notes |
|-----------|----------|------------|-------|
| GPIO5 | Status LED | Optional LED + resistor | Shows ESP32 is running |
| GPIO15 | WOL LED | Optional LED + resistor | Blinks on WOL detection |

## Wiring Details

### Power Switch Connection
```
ESP32 GPIO2 ────┐
                 │
                 ├─── power_sw+ (motherboard)
                 │
ESP32 GND ───────┘
```

**How it works:**
- ESP32 normally keeps GPIO2 as INPUT (floating/high impedance)
- When power button is "pressed", ESP32 temporarily changes GPIO2 to OUTPUT, pulls LOW for 50ms, then returns to INPUT
- This simulates pressing the physical power button without interfering with normal operation

### Power LED Connection
```
Motherboard Power LED ──── ESP32 GPIO4 (Analog Input)
                                    │
                                    └─── ESP32 GND
```

**Voltage Reading:**
- **Above 3V**: PC is OFF (5V detected)
- **Below 2V**: PC is ON (3.3V detected)
- **Between 2V-3V**: Use previous state to avoid flickering

### Power Supply Connection
```
Motherboard USB Header ──── ESP32
    5V ────────────────── VIN
    GND ───────────────── GND
```

## Safety Notes

1. **Voltage Levels**: The ESP32 GPIO pins are 3.3V tolerant, but the power LED wire carries 5V when PC is off. The analog input can safely read this voltage.

2. **Power Switch**: The power switch connection is safe as it only pulls the signal LOW, which is the same as pressing the physical button.

3. **Ground Connection**: Always ensure proper ground connection between ESP32 and motherboard to avoid floating voltages.

4. **USB Header**: Use a spare USB header to avoid interfering with existing USB devices.

## Quick Setup (4 Wires)

1. **Power**: Connect ESP32 VIN to 5V from USB header, GND to GND from USB header
2. **Power Switch**: Connect ESP32 GPIO2 to power_sw+ on motherboard
3. **Power LED**: Connect ESP32 GPIO4 to power LED wire from motherboard
4. **Test**: Power on ESP32 and access web interface

## Testing the Connection

1. **Power Supply**: ESP32 should power on and status LED should blink
2. **Power Switch**: Use web interface to "press" power button - should start PC
3. **Power LED**: Monitor serial output to see voltage readings and state changes
4. **WOL**: Send Wake-on-LAN packet to test LED blinking

## Troubleshooting

### ESP32 Won't Power On
- Check 5V and GND connections
- Verify USB header is providing power
- Check for short circuits

### Power Button Not Working
- Verify GPIO2 connection to power_sw+
- Check that ESP32 is pulling pin LOW when commanded
- Ensure proper ground connection

### Power LED Reading Incorrect
- Check GPIO4 connection
- Verify voltage levels (should see 5V when OFF, 3.3V when ON)
- Check serial output for voltage readings

### PC Won't Start
- Verify power_sw+ connection
- Check that ESP32 is properly grounded to motherboard
- Ensure PC power supply is working
