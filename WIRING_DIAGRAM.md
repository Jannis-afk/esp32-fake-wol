# Wiring Diagram - ESP32 Smart PC Power Controller

## Overview

This document provides detailed wiring instructions for connecting the ESP32 to your PC's front panel header. The ESP32 will be powered from a USB header on your motherboard and will control the power switch and monitor the power LED.

## Required Components

- ESP32 Development Board
- 4x Jumper Wires (male-to-male)
- 1x USB Cable (for power)
- Small breadboard (optional, for testing)

## Pin Mapping

| ESP32 Pin | Function | Motherboard Header | Wire Color (Recommended) |
|-----------|----------|-------------------|--------------------------|
| GPIO2     | Power Switch Control | PWRBTN# (Power Switch +) | Red |
| GPIO4     | Power LED Monitor | PWR_LED+ (Power LED +) | Green |
| 3.3V     | Power LED Ground | PWR_LED- (Power LED -) | Black |
| GND       | Common Ground | PWRBTN# (Power Switch -) | Black |

## Step-by-Step Wiring Instructions

### Step 1: Power the ESP32
1. Connect the ESP32's USB port to a spare USB header on your motherboard
2. Ensure the ESP32 receives power (status LED should blink)

### Step 2: Identify Front Panel Header
1. Locate the front panel header on your motherboard
2. It's usually labeled "F_PANEL", "FRONT_PANEL", or similar
3. Look for these specific pins:
   - **PWRBTN#** or **PWR_SW** (Power Switch)
   - **PWR_LED+** and **PWR_LED-** (Power LED)

### Step 3: Connect Power Switch Control
1. Connect ESP32 GPIO2 to PWRBTN# (Power Switch +)
2. Connect ESP32 GND to PWRBTN# (Power Switch -)
3. This allows the ESP32 to simulate pressing the power button

### Step 4: Connect Power LED Monitor
1. Connect ESP32 GPIO4 to PWR_LED+ (Power LED +)
2. Connect ESP32 3.3V to PWR_LED- (Power LED -)
3. This allows the ESP32 to read the PC's power state

## Visual Wiring Diagram

```
Motherboard USB Header          ESP32 Board
┌─────────────────┐            ┌─────────────────┐
│                 │            │                 │
│   USB +5V ─────┼────────────┼─── 5V/VIN      │
│                 │            │                 │
│   USB GND ─────┼────────────┼─── GND          │
│                 │            │                 │
└─────────────────┘            └─────────────────┘

Motherboard Front Panel Header  ESP32 Board
┌─────────────────────────────┐ ┌─────────────────┐
│                             │ │                 │
│  PWRBTN# (Power Switch +) ─┼─┼─── GPIO2       │
│                             │ │                 │
│  PWRBTN# (Power Switch -) ─┼─┼─── GND         │
│                             │ │                 │
│  PWR_LED+ (Power LED +) ───┼─┼─── GPIO4       │
│                             │ │                 │
│  PWR_LED- (Power LED -) ───┼─┼─── 3.3V        │
│                             │ │                 │
└─────────────────────────────┘ └─────────────────┘
```

## Alternative Wiring (Using Breadboard)

If you want to test the connections first:

```
Breadboard Layout:
┌─────────────────────────────────────────────────┐
│                                                 │
│  ESP32 GPIO2 ──┬─ Motherboard PWRBTN# +       │
│                 │                               │
│  ESP32 GND ─────┴─ Motherboard PWRBTN# -       │
│                                                 │
│  ESP32 GPIO4 ──┬─ Motherboard PWR_LED+         │
│                 │                               │
│  ESP32 3.3V ───┴─ Motherboard PWR_LED-         │
│                                                 │
└─────────────────────────────────────────────────┘
```

## Testing the Connections

### Before Powering On PC:
1. Use a multimeter to verify connections
2. Check for continuity between ESP32 pins and motherboard header
3. Ensure no short circuits exist

### After Powering On PC:
1. ESP32 status LED should blink
2. Web interface should be accessible
3. Power button control should work
4. Power LED monitoring should show correct state

## Common Header Pinouts

### Standard ATX Front Panel Header
```
┌─────────────────────────────────────┐
│ 1  3  5  7  9  11  13  15  17  19 │
│ 2  4  6  8  10 12  14  16  18  20 │
└─────────────────────────────────────┘

Pin 6:  PWRBTN# (Power Switch +)
Pin 8:  PWRBTN# (Power Switch -)
Pin 2:  PWR_LED+ (Power LED +)
Pin 4:  PWR_LED- (Power LED -)
```

### Mini-ITX Front Panel Header
```
┌─────────────────────┐
│ 1  3  5  7  9  11  │
│ 2  4  6  8  10 12  │
└─────────────────────┘

Pin 6:  PWRBTN# (Power Switch +)
Pin 8:  PWRBTN# (Power Switch -)
Pin 2:  PWR_LED+ (Power LED +)
Pin 4:  PWR_LED- (Power LED -)
```

## Safety Checklist

- [ ] PC is completely powered off
- [ ] ESP32 is powered via USB
- [ ] All connections are secure
- [ ] No exposed wires or short circuits
- [ ] Multimeter verification completed
- [ ] Breadboard testing (if applicable) completed

## Troubleshooting Wiring Issues

### Power Switch Not Working:
1. Check GPIO2 connection to PWRBTN# +
2. Verify GND connection to PWRBTN# -
3. Test with multimeter for continuity

### Power LED Not Reading:
1. Check GPIO4 connection to PWR_LED+
2. Verify 3.3V connection to PWR_LED-
3. Ensure LED polarity is correct

### ESP32 Not Powering:
1. Check USB connection to motherboard
2. Verify USB header is active
3. Check for loose connections

## Mounting Considerations

1. **Location**: Mount ESP32 near the front panel header for short wire runs
2. **Securing**: Use double-sided tape or small screws to secure the board
3. **Cable Management**: Route wires neatly to avoid interference
4. **Ventilation**: Ensure ESP32 doesn't block airflow in the case

## Next Steps

After completing the wiring:
1. Power on the ESP32
2. Follow the setup instructions in the main README
3. Test all functionality
4. Mount securely in your PC case
5. Enjoy remote PC power control!
