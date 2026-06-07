# Welcome to LUMETRIC

LUMETRIC is an acquisition and real‑time analysis system for fluorescence microscopy,
built on the Micro‑Manager ecosystem. It integrates automated image acquisition,
region‑of‑interest (ROI)–based analysis, and live visualization of fluorescence
intensity as well as ratio data within a unified graphical user interface.

The system addresses a common gap in microscopy workflows: the lack of an open,
configurable tool that combines flexible acquisition protocols, real‑time data
feedback, and reproducible data handling without relying on proprietary,
black‑box software.

LUMETRIC supports two acquisition backends:

1. Software‑controlled
2. Hardware‑triggered, while maintaining a shared user interface, analysis pipeline,
and data model.

## Intended use and scope

LUMETRIC is intended for researchers performing fluorescence microscopy using
Micro‑Manager who require live feedback during experiments and transparent,
reproducible acquisition workflows. It supports experimental designs involving
real‑time visualization of fluorescence intensity and ratio data, flexible
multi‑step and multi‑channel imaging protocols, and alternating excitation or
photoswitching schemes.

## What LUMETRIC provides (independent of acquisition mode)

The following capabilities are available in **all** LUMETRIC installations,
including LUMETRIC *lite*:

### Image acquisition structure

- Configurable exposure times, frame intervals, and frame counts
- Support for single channel and opto-split setups
- Automatic saving of acquired frames in TIFF format

### Real‑time analysis and visualization

- Live ROI‑based fluorescence intensity measurements
- Background subtraction using designated background ROIs
- Temporal and spatial ratio calculations (including FRET analyses)
- Simultaneous display of multiple live graphs
- Event markers during acquisition

### Data management and reproducibility

- Automatic creation of experiment‑specific output folders
- Export of raw and corrected ROI data as CSV
- Export of acquisition settings in human‑readable and hardware‑resolved formats
- Saving of ROIs in ImageJ‑compatible format
- Optional post‑processing of completed experiments without re‑acquisition

These features define the **core LUMETRIC data and analysis pipeline** and do not
depend on the chosen acquisition backend.

## Acquisition modes

LUMETRIC supports two acquisition modes that differ in **how timing and
triggering are implemented**, while sharing the same interface, analysis tools,
and output formats.

### Mode comparison

|  | LUMETRIC *lite* (Snap mode) | LUMETRIC Imaging System (TTL / Circular Buffer mode) |
|------|------|------|
| Additional hardware required | No | Yes (Arduino + wiring) |
| Camera triggering | Software‑controlled | TTL‑triggered |
| Timing precision | Host‑ and USB‑limited (≈10–50 ms) | Hardware‑limited (<1 ms) |
| Fast imaging (<150 ms frame interval) | No | Yes |
| Multi‑step acquisition tables | No | Yes |
| Alternating excitation protocols | No | Yes |
| Photoswitching / Pause steps | No | Yes |
| Real‑time ROI plots | Yes | Yes |
| Live ratio / FRET analysis | Yes | Yes |
| Post‑processing | Yes | Yes |

### LUMETRIC *lite* (software‑controlled)

LUMETRIC *lite* uses Micro‑Manager’s software‑triggered acquisition mechanisms and
does not require additional hardware.

Typical use cases:

- Single‑wavelength and low‑speed (>150 ms) experiments
- Live visualization of intensity or ratio signals
- Single-channel or FRET-Experiments

### LUMETRIC Imaging System (hardware‑triggered)

The full LUMETRIC Imaging System extends Micro‑Manager with a Arduino‑based
timing unit. Before acquisition, the complete experiment sequence is uploaded to
the controller, which then executes the protocol autonomously.

Typical use cases:

- High‑speed acquisitions
- complex  acquisition protocols with alternating excitation wavelength
- Photoswitching, photoactivation, and multicolor protocols
- Experiments requiring deterministic timing across thousands of frames
