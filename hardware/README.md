# LUMETRIC (Hardware Enclosure)

This folder contains the mechanical design files for the **LUMETRIC microscopy control enclosure**, designed to house an **Arduino GIGA R1** used as a bridge between **µManager (Micro-Manager)**, custom microscopy control firmware, lasers/light sources, and camera trigger systems.

The firmware and system-level control logic are documented separately in the firmware folder and accompanying documentation.

This README describes only the **mechanical housing (case)**.


## Overview

The enclosure is a two-part 3D-printable case (top and bottom shell) designed for laboratory use in a modular microscopy setup. It provides structured access to:

- Laser/light source control outputs (BNC)
- Camera trigger inputs/outputs (TTL, 3V, 5V)
- Arduino GIGA R1 interface access
- Secure mechanical mounting of electronics

The design is optimized for:
- modular optical setups
- multi-camera environments
- reproducible lab integration
- robust cable management



## Hardware Structure

### Case Design

The enclosure consists of:
- **Bottom shell**
- **Top cover**

Both parts are secured using screws in all four corners.

Each corner includes:
- Reinforced mounting stems
- Heat-set insert compatibility (metal threaded inserts recommended)
- Screw holes aligned between top and bottom shell


#### Level Shifter Placement

Inside the case, there is a dedicated rectangular area designed to accommodate a level shifter module. This space allows the level shifter to be mounted upside down, providing a convenient way to shift voltage levels from 5V to 3.3V. This feature is especially useful for driving cameras or other peripherals that require 3.3V logic, ensuring compatibility with devices that cannot tolerate 5V signals.

The level shifter area is positioned for easy access to both the Arduino and camera trigger lines, supporting flexible wiring and reliable operation in multi-voltage environments.


## Arduino Mounting

Inside the enclosure, the Arduino GIGA R1 is mounted on four internal standoffs:

- **Three standoffs use PCB alignment spikes** to secure board positioning
- **One standoff includes a threaded insert hole**
  - Designed for a screw with heat-set insert
  - Provides mechanical fixation of the Arduino board

This ensures stable alignment under repeated cable insertion/removal.


## Front Panel – Laser / Light Source Outputs

The front side of the enclosure contains:

### BNC Output Array
- **2 rows × 7 BNC connectors (14 total)**
- Individually labeled:
  - `1–7` (top row)
  - `1–7` (bottom row)

These outputs are intended for:
- laser control
- illumination sources
- external modulation signals


## Right Side Panel – Arduino & Camera Interface

The right side provides access to Arduino and camera trigger signals.

### Arduino I/O Access
- Central cutout for Arduino GIGA R1 connectors

### Camera Trigger Outputs
- 2 BNC connectors:
  - **3V trigger output**
  - **5V trigger output**

These allow:
- compatibility with different camera trigger voltage requirements
- support for multiple camera systems in the same lab

### Camera Trigger Input
- 1 BNC connector labeled:
  - **TTL**

This signal is:
- incoming camera trigger feedback
- routed to Arduino input for synchronization


## Materials & Assembly Notes

### Recommended Assembly Hardware
- Heat-set threaded inserts (M3 recommended)
- M3 screws for case closure
- M3 screw for Arduino fixation

### 3D Printing Recommendations
- Material: PETG or PLA+
- Layer height: 0.2 mm recommended
- Ensure dimensional accuracy for BNC connector tolerances


## Design Files

### CAD Source Files
- `cad/`  
  Native FreeCAD project files (`.FCStd`)

### Printable Files
- `stl/`  
  Ready-to-print STL files for enclosure parts


## License

This hardware design is licensed under:

**CERN Open Hardware Licence Version 2 – Permissive (CERN-OHL-P-2.0)**

You are free to:
- use
- modify
- manufacture
- redistribute

provided that attribution and license notice are preserved.

See `LICENSE` and `NOTICE` files for details.

---

## Attribution

If you use or modify this design in research, publications, or derivative hardware, please attribute:

**LUMETRIC Project**  
https://github.com/PSJacoby/LUMETRIC

---
