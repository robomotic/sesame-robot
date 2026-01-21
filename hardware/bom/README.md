# Bill of Materials

Every part required to assemble Sesame is cataloged here. Pick the wiring strategy that fits your parts bin, then follow the build flow in [docs/build-guide/README.md](../../docs/build-guide/README.md).

> [!NOTE]
> Amazon links below point to representative search results so you can choose local suppliers or equivalent listings. Pricing and availability change frequently. If you don't mind waiting shipping times you can also order direct from manufacturer for much lower rates.


## Core Electronics (Both Builds)

| Item | Qty | Notes | Amazon |
| --- | --- | --- | --- |
| MG90S all-metal micro servos | 8 (buy 10 for spares) | Primary hip/leg actuators; includes servo horns but keep extras | [link](https://www.amazon.com/s?k=mg90s+metal+gear+servo+pack+of+8) |
| 0.96" SSD1306 I2C OLED | 1 | 128x64 display that slides into the top cover slot | [link](https://www.amazon.com/s?k=0.96%22+I2C+OLED+SSD1306) |
| USB-C data/power cable | 1 | Needs to carry 5V/3A for flashing and tethered mode | [link](https://www.amazon.com/s?k=usb+c+cable+60w) |
| Rocker power switch (SPST, panel mount) | 1 | Snaps into the top cover cutout | [link](https://www.amazon.com/s?k=mini+rocker+switch+2+pin) |
| 22AWG silicone wire kit | 1 | Power/ground bus lines | [link](https://www.amazon.com/s?k=22awg+silicone+wire+kit) |
| 30AWG silicone wire kit | 1 | Signal leads and dense harnessing | [link](https://www.amazon.com/s?k=30awg+silicone+wire) |
| Heat-shrink assortment | 1 | Insulate OLED, switch, and battery joints | [link](https://www.amazon.com/s?k=heat+shrink+tubing+kit) |
| Small zip ties | 1 pack | Bundling wires inside the frame | [link](https://www.amazon.com/s?k=mini+zip+ties) |

## Wiring Option A – S2 Mini / Hand-Wired Harness

| Item | Qty | Notes | Amazon |
| --- | --- | --- | --- |
| Lolin/WeMos ESP32-S2 Mini | 1 | Native USB-C, fits on perfboard for the hand-wired build | [link](https://www.amazon.com/s?k=esp32+s2+mini) |
| Small protoboard (approx. 5×7 cm) | 1 | Hosts the header matrix and rails | [link](https://www.amazon.com/s?k=prototype+perfboard) |
| 3-pin male headers | 8 | Build the servo breakout; match spacing to MG90 plugs | [link](https://www.amazon.com/s?k=3+pin+servo+header+strip) |
| Buck converter (5–12 V in to stable 5 V/5 A out) | 1 | Powers motors + MCU when using batteries | [link](https://www.amazon.com/s?k=5a+dc+dc+buck+converter+module) |

## Wiring Option B – Sesame Distro Board / ESP32-DevKitC-32E

| Item | Qty | Notes | Amazon |
| --- | --- | --- | --- |
| ESP32-DevKitC-32E (ESP32-WROOM-32) | 1 | Base board the distro PCB stacks on. This one is very tricky because its a very specific board. You can use the 32E with the floating pcb antenna OR you can use the 32U but you have to route an antenna inside. | [link](https://www.amazon.com/s?k=ESP32+DevKitC+32) |
| Sesame Distro Board PCB | 1 | Order `Gerber_Sesame-Distro-Board_PCB_Sesame-Distro-Board_V1.zip` via PCBway | [link](/hardware/pcb/README.md) |
| 5 V buck converter (same spec as above) | 1 | Mounts on the distro board pads | [link](https://www.amazon.com/s?k=5a+dc+dc+buck+converter+module) |
| 4-pin JST-XH or PH header | 1 | Optional external connector footprint on PCB | [link](https://www.amazon.com/s?k=jst+xh+4+pin+kit) |
| 2-pin screw terminal (5.08 mm) | 1 | Optional battery input on PCB | [link](https://www.amazon.com/s?k=2+pin+screw+terminal+block) |
| M2.5 × 5 mm male-female standoffs | 4 | Elevate the PCB over the DevKit mounting holes | [link](https://www.amazon.com/s?k=m2.5+male+female+standoff+5mm) |

## Power Sources & Connectors

| Item | Qty | Notes | Amazon |
| --- | --- | --- | --- |
| 3S 450 mAh LiPo with XT30 | 1 | Recommended wireless pack; ensure high-discharge rating | [link](https://www.amazon.com/s?k=3s+450mah+lipo+xt30) |
| 2× 10440 Li-ion cells + 2× AAA holder | 1 set | 2S pack (~7.4 V nominal) that fits the chassis battery slot; use a buck converter | [link](https://www.amazon.com/s?k=2x+aaa+battery+holder+with+leads) |
| 10440 Li-ion cells (350–400 mAh typical) | 2 | Standard 10×44 mm Li-ion cells; buy spares | [link](https://www.amazon.com/s?k=10440+li-ion+cell+350mah) |
| 2-slot 10440 Li-ion charger | 1 | Charge cells outside the holder; avoid in-holder charging unless the holder is designed for it | [link](https://www.amazon.com/s?k=10440+li-ion+charger+2+slot) |
| Alternative: 3× 10440 Li-ion cell pack | 1 | If LiPo sourcing is tricky; retain 5–12 V input requirement | [link](https://www.amazon.com/s?k=10440+li-ion+pack) |
| XT30 female pigtail | 1 | Interface battery to switch/PCB without cutting stock leads | [link](https://www.amazon.com/s?k=xt30+female+pigtail) |
| JST RCY female pigtail | 1 | Use if your battery ships with JST RCY instead of XT30 | [link](https://www.amazon.com/s?k=jst+rcy+female+pigtail) |

## Connectors, Harnessing & Protection

| Item | Qty | Notes | Amazon |
| --- | --- | --- | --- |
| Servo extension leads (dupont-style) | 4–8 | Helpful for rerouting or extending motor wires | [link](https://www.amazon.com/s?k=servo+extension+lead+10cm) |
| Solderable inline toggle or push button (optional) | 1 | Debug reset switch if desired | [link](https://www.amazon.com/s?k=mini+push+button+switch) |
| Liquid electrical tape or Kapton | 1 | Insulate tight joints near the OLED cavity | [link](https://www.amazon.com/s?k=liquid+electrical+tape) |

## Fasteners & Mechanical Hardware

| Item | Qty | Usage | Amazon |
| --- | --- | --- | --- |
| M2 × 4 mm self-tapping screws | ~40 | Servo horns, joints, OLED retention | [link](https://www.amazon.com/s?k=m2+self+tapping+screws+4mm) |
| M2 × 10 mm self-tapping screws | 10 | Securing top + bottom covers to the internal frame | [link](https://www.amazon.com/s?k=m2+self+tapping+screws+10mm) |
| M2 - M2.5 machine screws + nuts (optional) | 10 | Alternate fastening for servo horns if included screws are short | [link](https://www.amazon.com/s?k=m2+machine+screw+kit) |

## 3D Printed Parts

Print the 11-part part set outlined in [printing/README.md](../printing/README.md). STL and CAD sources live under `hardware/printing/`.


## Consumables & Tools Checklist

| Item | Notes | Amazon |
| --- | --- | --- |
| Leaded solder (0.6–0.8 mm) | Easier flow for dense perfboard work | [link](https://www.amazon.com/s?k=63%2F37+solder+0.8mm) |
| Flux pen | Protects pads on the perfboard and PCB | [link](https://www.amazon.com/s?k=flux+pen) |
| Solder wick / pump | For rework on the OLED pins | [link](https://www.amazon.com/s?k=solder+wick) |
| Small flush cutters | Trim servo leads, perfboard traces, or supports | [link](https://www.amazon.com/s?k=flush+cutters) |
| Precision screwdriver set | Needed for self-tapping M2 hardware | [link](https://www.amazon.com/s?k=precision+screwdriver+set) |

## Power & Safety Notes

- Sesame needs at least 5 V at 3 A available at the rails. USB-A chargers rarely meet that spec; stick with USB-C PD or the buck-converted battery input.
- When battery powering the S2 Mini harness, route the pack through the rocker switch and buck converter before it touches the rails, mirroring the schematic in [docs/wiring-guide/README.md](../../docs/wiring-guide/README.md).
- **Never cut the factory battery connector off the pack.** Instead, create adapter pigtails using XT30 or JST RCY leads so the pack remains chargeable.
- A 2× 10440 Li-ion + 2× AAA holder pack fits the stock battery cavity and works well with the existing power switch + buck converter.
- Charge 10440 cells in a proper Li-ion charger outside the holder unless the holder explicitly supports charging.
