# Hardware

All mechanical and electrical source files for the Sesame Robot live here. The hardware stack supports two build paths:

- **Hand-wired / S2 Mini build** for simple protoboard construction.
- **Custom Sesame Distro Board build** for a cleaner ESP32-DevKitC-32E stackup.

Use the sections below to jump to the files that match the version you are assembling.

## Directory Guide

| Folder | What you will find |
| --- | --- |
| [bom](bom/README.md) | Full bill of materials covering both wiring approaches, plus power budget notes and links back to the build tutorial. Start here to gather every component before printing or soldering. |
| [cad](cad/README.md) | Parametric STEP and Fusion 360 source models for every printed part. Great for remixing joint geometry or adapting the shell—just note the caution about features that may not translate across CAD packages. |
| [pcb](pcb/README.md) | Sesame Distro Board schematics, layout renders, and the `Gerber_Sesame-Distro-Board_PCB_Sesame-Distro-Board_V1.zip` archive you can upload to PCBway (ordering steps included). |
| [printing](printing/README.md) | Practical PLA print settings, support callouts, and image references for joint orientation and the top cover’s manual supports. |

## Getting Started

1. **Decide on a wiring strategy.** Review the comparison section in [docs/wiring-guide/README.md](../docs/wiring-guide/README.md) to choose between the protoboard and custom PCB routes.
2. **Print the shell and joints.** Follow the presets in [printing/README.md](printing/README.md) to prep the 3D files from `hardware/printing/stl`.
3. **Source electronics.** Use the line-by-line list in [bom/README.md](bom/README.md) and verify your power supply meets the 5V/3A requirement.
4. **Assemble electronics.** Reference either the hand-wiring steps or the distro-board guide plus the wiring diagrams in `docs/wiring-guide/`.

> [!TIP]
> Keep photos of your wiring progress. They are invaluable for troubleshooting later and help when sharing build notes with the community.

## Supporting Documents

- [docs/build-guide/README.md](../docs/build-guide/README.md) for end-to-end assembly sequencing.
- [docs/wiring-guide/README.md](../docs/wiring-guide/README.md) for wiring diagrams, safety notes, and packing tips.

Improvements, remixes, and test reports are welcome—file issues or PRs with any updates you discover while building. <3
