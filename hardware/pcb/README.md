# PCB Schematics

Electronic schematics and PCB designs for the Sesame Robot Project. There are two versions of the Sesame Distro Board available: V2 (current) and V1 (legacy).

> [!NOTE]
> **For Sesame Build Kits:** All Sesame Build Kits include a pre-flashed Sesame Distro Board V2, so you don't need to order or assemble a board separately.

> [!TIP]
> **Building from Scratch?** If you're building a Sesame Robot from scratch, we recommend using the **S2 Mini with hand wiring approach** for the easiest assembly experience. The V2 distro board uses advanced SMD components that require specialized soldering skills or PCB assembly services.

---

## Sesame Distro Board V2 (Current)

The **Sesame Distro Board V2** is the latest version and features:
- **Power Delivery (PD) support** - Works with USB-C PD power supplies
- **Battery power support** - Also works with battery power
- **Compact SMD design** - All surface-mount components for a professional finish
- **Included in all Sesame Build Kits** - Pre-flashed and ready to use

### Assembly Options for V2

The V2 board consists entirely of SMD (surface-mount) components, which are **advanced to hand solder**. We recommend:

1. **PCB Assembly Service (Recommended):** Use [PCBway's PCB assembly service](https://www.pcbway.com/pcb-assembly.html) to have the board professionally assembled. Upload the Gerber, BOM, and Pick-and-Place files to their assembly service.
2. **Hand Soldering (Advanced Only):** Only attempt hand soldering if you're experienced with SMD components and have the proper tools (hot air station, fine-tip soldering iron, flux, etc.).

### V2 Files Available

All files are located in the [`distro-v2/`](distro-v2/) directory:
- **Schematic source:** `SCH_Sesame-Distro-Board-V2_2026-03-06.json` - EasyEDA source design file for the V2 schematic
- **Gerber file:** `Gerber_Sesame-Distro-V2_PCB.zip` - For PCB fabrication
- **BOM file:** `BOM_Sesame-Distro-V2.csv` - Bill of materials for SMD components
- **Pick-and-Place file:** `PickAndPlace_PCB_Sesame-Distro-V2.csv` - Component placement data for assembly

### PCBway / JLCPCB Sourcing Notes

When uploading the BOM to PCBway or JLCPCB for assembly, note the following three components that require special handling:

| Designator | Issue | Corrected Part | LCSC # |
|---|---|---|---|
| `U5`, `U7` | **Solder Pads** — these are copper pads only, no physical component is needed. Remove these lines from the BOM when ordering assembly. | N/A | N/A |
| `5-12V` | 2-pin 3.5 mm screw terminal. Original part `1984617` (Phoenix Contact) cannot be sourced. | `XY302V-3.5-2P` | [C784940](https://www.lcsc.com/product-detail/C784940.html) |
| `JST-XH` | 4-pin JST XH connector. Original `JST-XH-4-PIN` cannot be sourced. | `B4B-XH-A(LF)(SN)` | [C144395](https://www.lcsc.com/product-detail/C144395.html) |

The BOM CSV has already been updated with the corrected parts. The solder-pad rows are marked **"No Part Required"** in the Name field so they are easy to identify and exclude.

---

## Sesame Distro Board V1 (Legacy)

> [!CAUTION]  
> The Sesame Distro Board V1 is now **phased out** but still supported. V1 has some limitations: it won't run on tethered power (e.g., USB-C) and is harder to assemble than other options. V1 is still supported with wiring guides and firmware and works on battery power. If you have a V1 board, you can still use it successfully.

The Distro Board V1 pairs with the ESP32-DevKitC-32E.

> [!IMPORTANT]
> **ESP32 Pin Header Requirement:** The distro board V1 stacks on top of the ESP32-DevKitC-32E, so you need an ESP32 board **without pre-soldered pin headers**. If your board came with headers already soldered on the top, you will need to desolder all of the headers and flip them to the bottom side of the ESP32 board so the distro board can mount on top.

### V1 Board Schematic:

<img src="distro-v1/Schematic_Sesame-Distro-Board.png" alt="Schematic_Sesame-Distro-Board" width="70%">

### V1 Files Available

All files are located in the [`distro-v1/`](distro-v1/) directory.

---

## PCBway Sponsorship

**PCBway Sponsorship**

This project was sponsored by [PCBway](https://www.pcbway.com/), who manufactured the custom distro boards. PCBway offers high-quality PCB fabrication services with fast turnaround times and excellent customer support. 

If you're building your own Sesame Robot, PCBway is a great option for getting professional-quality PCB fabrication and assembly services at reasonable prices:
- **PCB Fabrication:** Upload Gerber files to get boards manufactured
- **PCB Assembly:** Upload Gerber, BOM, and Pick-and-Place files for fully assembled boards (recommended for V2)

<img src="pcbs.png" alt="pcbs-from-pcbway" width="70%">

---

## How to Order

### Ordering V2 Boards (Recommended)

**Option 1: PCB Assembly Service (Recommended)**

1. Go to [PCBway's PCB Assembly service](https://www.pcbway.com/pcb-assembly.html)
2. Upload the Gerber file: `distro-v2/Gerber_Sesame-Distro-V2_PCB.zip`
3. Upload the BOM file: `distro-v2/BOM_Sesame-Distro-V2.csv`
4. Upload the Pick-and-Place file: `distro-v2/PickAndPlace_PCB_Sesame-Distro-V2.csv`
5. Confirm board specifications and component availability
6. Place your order to receive fully assembled boards

**Option 2: Fabrication Only (For Advanced Users)**

1. Download the Gerber file from [`distro-v2/Gerber_Sesame-Distro-V2_PCB.zip`](distro-v2/Gerber_Sesame-Distro-V2_PCB.zip)
2. Upload to [PCBway](https://www.pcbway.com/) or another PCB manufacturer
3. Confirm board specifications in the preview
4. Order the bare PCBs and hand-solder SMD components yourself (advanced)

### Ordering V1 Boards (Legacy)

1. Download the Gerber file from [`distro-v1/Gerber_Sesame-Distro-Board_PCB_Sesame-Distro-Board_V1.zip`](distro-v1/Gerber_Sesame-Distro-Board_PCB_Sesame-Distro-Board_V1.zip)
2. Upload to [PCBway](https://www.pcbway.com/) or another PCB manufacturer
3. Confirm board specifications in the preview
4. Place your order to receive bare V1 boards

