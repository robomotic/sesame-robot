# Wiring Guide

Complete wiring guides for the Sesame Robot.

---

## Choosing a Wiring Approach

The project supports multiple wiring strategies that suit different build constraints:

- **Lolin S2 Mini / hand wiring (RECOMMENDED FOR DIY BUILDS):** Uses a Lolin S2 Mini, loose headers, and point-to-point wiring. It is the lowest-cost option, easy to source, supports USB-C PD for tethered power, but demands patience to keep the harness tidy and leaves less room for mistakes. **Critical: Use 30AWG wire for data lines and 22AWG for power—larger gauge wire will make assembly nearly impossible.**
- **Sesame Distro Board V2 (INCLUDED IN BUILD KITS):** The latest custom PCB with SMD components, supports both USB-C PD and battery power. Pre-flashed and included with all Sesame Build Kits. Advanced to hand-solder; professional assembly recommended if ordering separately.
- **Sesame Distro Board V1 / ESP32-DevKitC-32E (LEGACY):** Uses the custom Distro Board V1 PCB stacked on an ESP32-DevKitC-32E. Now phased out but still supported. V1 has known limitations and cannot run on tethered USB-C power (battery + buck converter required).

Pick the approach that matches your component availability and comfort with managing wire bundles; the remainder of this guide dives into both workflows.

## How to wire the Lolin S2 Mini / Hand Wiring

The Lolin S2 Mini method is a traditional protoboard build. Consult the S2 Mini wiring diagram below while reading this section; it captures every motor, sensor, OLED, and button lead even if the photos only show a subset of the connections. Expect a high connection count, so plan wire lengths and colors before soldering to avoid confusion later.

<img src="s2-mini-wiring-guide-new.png" alt="S2 Mini wiring diagram" width="70%">

*Credit to @captianeverypowersr on Discord for the new S2 mini guide.*

### Prep

1. **Secure and tin** all of the connections on the board before starting
2. To connect all the 3-pin headers together and to the board, first attach them to a small section of protoboard

<img src="assets/proto-headers.png" alt="proto-headers" width="70%">

### Building Power and Ground Rails

1. Solder one wire to one end of the pins, then guide it along and remove the insulation
2. Solder this exposed wire to every middle pin (this creates the **5V rail**)
3. Do the same for the ground lane (this creates the **ground rail**)

<img src="assets/proto-power-rails.png" alt="proto-power-rails" width="70%">

### Data Lines

You can now cut eight equal-length wires for data connections.

**Wire Recommendations:**

- **Data lines:** 30AWG silicone wire
- **Power and ground:** 22AWG silicone wire
- **Important:** Things will get super cluttered if you use large gauge wire

<img src="assets/proto-datalines.png" alt="proto-data-lines" width="70%">

Alongside the eight motor data leads, the diagram also calls out the OLED connections that branch from the microcontroller.

### Packing Electronics

When packing electronics into the frame, it's difficult with the hand wiring setup because there are a lot of stray wires. My advice is to work slowly and methodically:

1. Group wires with similar destinations
2. Use zip ties and heat shrink tubing to make them as compact as possible
3. Consider connecting the power switch after finishing most other wiring, since it's attached to the top cover

<img src="assets/wire-managment.png" alt="wire-managment" width="70%">

### Safety and Testing

> [!CAUTION]
> Do not solder and de-solder connections with power connected!

**Before powering on:**

- Double check your power and ground lines before turning the robot on
- Make sure to cover any exposed wires as they can touch the pin headers and fry the ESP32 (especially when cramming the cover on)

> [!TIP]
> To make fishing out the wires for the OLED display easier, you can temporarily twist them together into a group and then guide them through the opening in the top cover.

---

## How to wire the Sesame Distro Board V1 / ESP32-DevKitC-32E

### Overview

> [!CAUTION]
> UPDATE 1/20/26: Upon further testing, the Sesame distro board V1 will work, but it has a few issues that make it a little harder to assemble and will not run on teathered power (eg. USB C). Until V2 is released, I recommend using the S2 Mini / Hand Wiring approach. If you ordered a distro board V1, it will still be supported with wiring guides and firmware for the foreseeable future <3.

This option provides a cleaner, more organized wiring solution.
Consult the distro-board wiring diagram below for exact connection locations.

> [!IMPORTANT]
> **ESP32 Pin Header Requirement:** The distro board V1 stacks on top of the ESP32-DevKitC-32E, so you need an ESP32 board **without pre-soldered pin headers**. If your board came with headers already soldered on the top, you will need to desolder all of the headers and flip them to the bottom side of the ESP32 board so the distro board can mount on top.

<img src="distro-board-wiring-guide.png" alt="distro-board wiring diagram" width="70%">

### Component Installation

**Optional components** (you can solder directly to pads instead):

- 4-pin JST connector
- 2-pin screw terminal

**Pin header installation tip:** If you're having difficulties keeping the pin headers in place to solder, try placing them in a protoboard first, then transferring them over.

### Buck Converter Setup

A buck converter takes any voltage (5V-12V) and drops it to a stable 5V for the motors and ESP32.

**If using a battery:**

1. Make sure to solder the buck converter enable pads
2. The buck converter is **required** for battery operation
3. 2× 10440 Li-ion cells in a 2× AAA holder (2S, ~7.4 V nominal) work well with the buck and fit the chassis battery slot

**Alternative power options:**

- If you're using a benchtop supply or have another regulated 5V source
- You can override the buck converter and take voltage directly from the screw terminal by soldering the override pads

### Battery Connection

> [!CAUTION]
> If your battery has an XT30 or JST RCY connector, don't cut it off and wire the battery directly to the distro board! This is unsafe and means you can't charge your battery anymore.

**Proper method:**

1. Get a female XT30 or JST RCY connector (matching your battery)
2. Wire it to two wires that lead to the power terminal
3. This allows you to safely connect and disconnect the battery

**Note on 2× AAA holders with 10440 Li-ion cells:**

- Treat this as a 2S Li-ion pack; wire the holder leads through the switch and buck converter.
- **Always remove the 10440 cells from the holder and recharge them separately using a proper Li-ion charger.** Most AAA holders do not support safe Li-ion charging and attempting to charge in-holder can be dangerous.

### Sourcing the Distro Board

The Sesame Distro Board V1 is a custom PCB designed specifically for this project. It mounts on top of a ESP32-DevKitC-32E.

More information is avalible in the [PCB](../../hardware/pcb/README.md) section of the documentation.

---

## General Wiring Tips

- Work slowly and methodically
- Test connections before applying power
- Keep wire runs as short and neat as possible
- Label wires if helpful for troubleshooting
- Take photos during assembly for reference
