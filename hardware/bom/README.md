# Bill of Materials

Every part required to assemble Sesame is cataloged here. Pick the wiring strategy that fits your parts bin, then follow the build flow in [docs/build-guide/README.md](../../docs/build-guide/README.md).

> [!NOTE]
> Amazon links below point to representative search results so you can choose local suppliers or equivalent listings. Pricing and availability change frequently. If you don't mind waiting shipping times you can also order direct from manufacturer for much lower rates.


## Core Electronics (Both Builds)

| Item | Qty | Notes | Source |
| --- | --- | --- | --- |
| MG90S all-metal micro servos | 8 (buy 10 for spares) | Primary hip/leg actuators; includes servo horns but keep extras | [Amazon](https://www.amazon.com/s?k=mg90s+metal+gear+servo+pack+of+8) |
| 0.96" SSD1306 I2C OLED | 1 | 128x64 display that slides into the top cover slot | [Amazon](https://www.amazon.com/s?k=0.96%22+I2C+OLED+SSD1306) |
| USB-C data/power cable | 1 | Needs to carry 5V/3A for flashing and tethered mode | [Amazon](https://www.amazon.com/s?k=usb+c+cable+60w) |
| Rocker power switch (KCD1, panel mount) | 1 | Snaps into the top cover cutout | [Amazon](https://www.amazon.com/s?k=KCD1+mini+rocker+switch+2+pin) |
| 22AWG silicone wire kit | 1 | Power/ground bus lines | [Amazon](https://www.amazon.com/s?k=22awg+silicone+wire+kit) |
| 30AWG silicone wire kit | 1 | Signal leads and dense harnessing | [Amazon](https://www.amazon.com/s?k=30awg+silicone+wire) |
| Heat-shrink assortment | 1 | Insulate OLED, switch, and battery joints | [Amazon](https://www.amazon.com/s?k=heat+shrink+tubing+kit) |
| Small zip ties | 1 pack | Bundling wires inside the frame | [Amazon](https://www.amazon.com/s?k=mini+zip+ties) |

## Wiring Option A – S2 Mini / Hand-Wired Harness

| Item | Qty | Notes | Source |
| --- | --- | --- | --- |
| Lolin/WeMos ESP32-S2 Mini | 1 | Native USB-C, fits on perfboard for the hand-wired build | [Amazon](https://www.amazon.com/s?k=esp32+s2+mini) |
| Small protoboard (approx. 5×7 cm) | 1 | Hosts the header matrix and rails | [Amazon](https://www.amazon.com/s?k=prototype+perfboard) |
| 3-pin male headers | 8 | Build the servo breakout; match spacing to MG90 plugs | [Amazon](https://www.amazon.com/s?k=pin+header+strip) |
| Buck converter (5–12 V in to stable 5V/3A out) | 1 | Powers motors + MCU when using batteries | [Amazon](https://www.amazon.com/s?k=3a+dc+dc+buck+converter+module) |

## Wiring Option B – Sesame Distro Board V2 (Included in Build Kits)

> [!NOTE]
> If you purchased a Sesame Build Kit, your V2 Distro Board is already assembled, pre-flashed, and included. You don't need to order these parts separately.

| Item | Qty | Notes | Source |
| --- | --- | --- | --- |
| Sesame Distro Board V2 PCB | 1 | Fully SMD design. Order with PCBway assembly service or attempt advanced hand soldering. See [ordering guide](/hardware/pcb/README.md) | [GitHub](/hardware/pcb/README.md) |

## Wiring Option C – Sesame Distro Board V1 / ESP32-DevKitC-32E (Legacy)

> [!CAUTION]
> V1 is now phased out but still supported. Only choose this if you already have a V1 board.

| Item | Qty | Notes | Source |
| --- | --- | --- | --- |
| ESP32-DevKitC-32E (ESP32-WROOM-32) | 1 | Base board the Distro Board V1 stacks on. This one is very tricky because its a very specific board. You can use the 32E with the floating pcb antenna OR you can use the 32U but you have to route an antenna inside. | [Amazon](https://www.amazon.com/s?k=ESP32+DevKitC+32) |
| Sesame Distro Board V1 PCB | 1 | Order `Gerber_Sesame-Distro-Board_PCB_Sesame-Distro-Board_V1.zip` via PCBway | [GitHub](/hardware/pcb/README.md) |
| 5V buck converter (same spec as above) | 1 | Mounts on the distro board pads | [Amazon](https://www.amazon.com/s?k=3a+dc+dc+buck+converter+module) |
| 1000 µF electrolytic capacitor | 1 | Smooths output voltage on buck converter; 10V+ rating recommended | [Amazon](https://www.amazon.com/s?k=1000uf+electrolytic+capacitor) |
| 4-pin JST-XH or PH header | 1 | Optional external connector footprint on PCB | [Amazon](https://www.amazon.com/s?k=jst+xh+4+pin+kit) |
| 2-pin screw terminal (2.54 mm pitch) | 1 | Optional battery input on PCB | [Amazon](https://www.amazon.com/s?k=2+pin+screw+terminal+block+2.54mm+pitch) |
| M2.5 × 5 mm male-female standoffs | 4 | Elevate the PCB over the DevKit mounting holes | [Amazon](https://www.amazon.com/s?k=m2.5+male+female+standoff+5mm) |

## Power Sources & Connectors

| Item | Qty | Notes | Source |
| --- | --- | --- | --- |
| 3S 450 mAh LiPo with XT30 | 1 | Recommended wireless pack; ensure high-discharge rating | [Amazon](https://www.amazon.com/s?k=3s+450mah+lipo+xt30) |
| 2× AAA holder | 1 set | 2S pack using AAA size Li-Ion Batteries (~7.4 V nominal) that fits the chassis battery slot; use a buck converter | [Amazon](https://www.amazon.com/s?k=2x+aaa+battery+holder) |
| 10440 Li-ion cells (350–400 mAh typical) | 2 | Standard 10×44 mm Li-ion cells; buy spares | [Amazon](https://www.amazon.com/s?k=10440+li-ion+cell+350mah) |
| 2-slot 10440 Li-ion charger | 1 | Charge cells outside the holder; avoid in-holder charging unless the holder is designed for it | [Amazon](https://www.amazon.com/s?k=10440+li-ion+charger+2+slot) |
| XT30 female pigtail | 1 | Interface battery to switch/PCB without cutting stock leads | [Amazon](https://www.amazon.com/s?k=xt30+female+pigtail) |

## Fasteners & Mechanical Hardware

| Item | Qty | Usage | Amazon |
| --- | --- | --- | --- |
| M2 × 5 mm self-threading screws | ~40 | All plastic joints, OLED retention, motor mounts, and covers (Can just get a variety pack) | [Amazon](https://www.amazon.com/s?k=m2+self+tapping+screws+kit) |
| M2.5 × 5mm machine screws | 10 | Servo horn attachment to servo shafts only. Included servo horn screws are usually too short. | [Amazon](https://www.amazon.com/s?k=m2+machine+screw+kit) |

## 3D Printed Parts

Print the 11-part part set outlined in [printing/README.md](../printing/README.md). STL and CAD sources live under `hardware/printing/`.

## Consumables & Tools Checklist

| Item | Notes | Source |
| --- | --- | --- |
| Leaded solder (0.6–0.8 mm) | Easier flow for dense perfboard work | [Amazon](https://www.amazon.com/s?k=63%2F37+solder+0.8mm) |
| Flux pen | Protects pads on the perfboard and PCB | [Amazon](https://www.amazon.com/s?k=flux+pen) |
| Solder wick / pump | For rework on the OLED pins | [Amazon](https://www.amazon.com/s?k=solder+wick) |
| Small flush cutters | Trim servo leads, perfboard traces, or supports | [Amazon](https://www.amazon.com/s?k=flush+cutters) |
| Precision screwdriver set | Needed for self-tapping M2 hardware | [Amazon](https://www.amazon.com/s?k=precision+screwdriver+set) |

## Power & Safety Notes

- Sesame needs at least 5 V at 3 A available at the rails. 
- **Lolin S2 Mini:** Can be powered via USB-C PD (5V/3A capable) for tethered operation, or via battery + buck converter.
- **Distro Board V2:** Supports both USB-C PD (5V/3A) for tethered operation AND battery + buck converter. Included in all Sesame Build Kits.
- **Distro Board V1 (Legacy):** Cannot run on tethered USB-C power due to design limitations. Must use battery + buck converter for operation.
- When battery powering either build, route the pack through the rocker switch and buck converter before it touches the rails, mirroring the schematic in [docs/wiring-guide/README.md](../../docs/wiring-guide/README.md).
- **Never cut the factory battery connector off the pack.** Instead, create adapter pigtails using XT30 or JST RCY leads so the pack remains chargeable.
- A 2× 10440 Li-ion + 2× AAA holder pack fits the stock battery cavity and works well with the existing power switch + buck converter.
- **Always remove and charge 10440 cells separately in a proper Li-ion charger.** Do not charge them while inside the AAA holder unless the holder explicitly supports safe Li-ion charging (most do not).
