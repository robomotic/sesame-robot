# Hardware

All mechanical and electrical source files for the Sesame Robot live here. The hardware stack supports multiple build paths:

- **Hand-wired / Lolin S2 Mini build** for simple protoboard construction (recommended for DIY builds).
- **Custom Sesame Distro Board V2 build** for a professional SMD design with PD and battery support (included in Sesame Build Kits, pre-flashed).
- **Custom Sesame Distro Board V1 build** (legacy) for ESP32-DevKitC-32E stackup (phased out but still supported).

Use the sections below to jump to the files that match the version you are assembling.

## Directory Guide

| Folder | What you will find |
| --- | --- |
| [bom](bom/README.md) | Full bill of materials covering both wiring approaches, plus power budget notes and links back to the build tutorial. Start here to gather every component before printing or soldering. |
| [cad](cad/README.md) | Parametric STEP and Fusion 360 source models for every printed part. Great for remixing joint geometry or adapting the shell—just note the caution about features that may not translate across CAD packages. |
| [pcb](pcb/README.md) | Sesame Distro Board V2 (current) and V1 (legacy) schematics, layout files, Gerber files, BOM, and Pick-and-Place files. Includes ordering instructions for PCBway fabrication and assembly services. |
| [printing](printing/README.md) | Practical PLA print settings, support callouts, and image references for joint orientation and the top cover’s manual supports. |

## Getting Started

1. **Decide on a wiring strategy.** Review the comparison section in [docs/wiring-guide/README.md](../docs/wiring-guide/README.md) to choose between the protoboard and custom PCB routes. The Lolin S2 Mini build is recommended for DIY builders. If you have a Sesame Build Kit, your V2 Distro Board is already pre-flashed and ready to use.
2. **Print the shell and joints.** Follow the presets in [printing/README.md](printing/README.md) to prep the 3D files from `hardware/printing/stl`.
3. **Source electronics.** Use the line-by-line list in [bom/README.md](bom/README.md) and verify your power supply meets the 5V/3A requirement.
4. **Assemble electronics.** Reference either the hand-wiring steps or the appropriate Distro Board guide plus the wiring diagrams in `docs/wiring-guide/`.

> [!TIP]
> Keep photos of your wiring progress. They are invaluable for troubleshooting later and help when sharing build notes with the community.

## Supporting Documents

- [docs/build-guide/README.md](../docs/build-guide/README.md) for end-to-end assembly sequencing.
- [docs/wiring-guide/README.md](../docs/wiring-guide/README.md) for wiring diagrams, safety notes, and packing tips.

Improvements, remixes, and test reports are welcome, file issues or PRs with any updates you discover while building. <3
