# Welcome to LUMETRIC

LUMETRIC is a **software tool for fluorescence microscopy** that works together with Micro‑Manager. It combines automated image acquisition, ROI selection, and real-time analysis of intensity and ratio signals (e.g., FRET). It enables live visualization of ROI-based traces during experiments, allowing immediate assessment of signal dynamics in a single interface.

In addition, LUMETRIC supports **flexible acquisition protocols** such as alternating excitation and photoswitching, enabling experimental designs that are widely used in practice. These capabilities are accessible within a unified graphical interface that combines acquisition control, real-time analysis, and live plotting.

By consolidating these functionalities into a single, open and transparent framework, LUMETRIC facilitates direct, reproducible monitoring of fluorescence signals and reduces reliance on fragmented workflows or external post-processing steps. This integration further enables users to perform advanced experiments independently of commercial, proprietary software environments.

**LUMETRIC offers two ways to run image acquisition**, depending on the required timing precision and hardware setup:

1. **Software-controlled mode**, which relies on standard Micro‑Manager control and is suitable for simple single wavelenght experiments without additional hardware
2. **Hardware-triggered mode**, which uses an external timing device to precisely synchronize camera exposure and illumination, enabling more complex and high-speed experiments

Despite these differences, both modes share the same interface, analysis tools, and data output, allowing users to switch between them without changing their workflow.

## Intended use and scope

LUMETRIC is designed for fluorescence microscopy experiments where real-time feedback and reproducible data analysis are essential.
It is particularly suited for workflows that require:

- Live monitoring of fluorescence intensity and intensity ratios (e.g. FRET signals)
- Multi-channel imaging and sequential acquisition steps
- Alternating excitation or photoswitching protocols

## What LUMETRIC provides (independent of acquisition mode)

The following features are available in **all** versions of LUMETRIC, regardless of the selected acquisition mode (including LUMETRIC *lite*):

### Image acquisition structure

- Set exposure time, frame interval, and number of frames
- Support for single-channel and split-image (e.g. opto-split) setups
- Automatic saving of images in TIFF format
- Automatic saving of acquired raw data

### Real‑time analysis and visualization

- Live measurement of fluorescence intensity in user-defined ROIs
- Background correction using dedicated background ROIs
- Temporal and spatial ratio calculations (including FRET analyses)
- Simultaneous display of multiple live plots
- Ability to add event markers during acquisition

### Data management and reproducibility

- Automatic creation of experiment‑specific output folders
- Export of raw and corrected ROI data as CSV
- Export of acquisition settings in human‑readable and hardware‑resolved formats
- Saving of ROIs in ImageJ‑compatible format
- Optional post‑processing of completed experiments without re‑acquisition

These features define the **core LUMETRIC data and analysis pipeline** and do not
depend on the chosen acquisition backend.

## Acquisition modes

LUMETRIC provides two acquisition modes, which differ in how precisely image timing and device control are handled. This allows users to choose between a simple setup without additional hardware and a more advanced setup for high-speed or complex experiments.
Both modes share the same interface, live analysis tools, and data output, ensuring a consistent workflow independent of the chosen configuration.

### Mode comparison

|  | LUMETRIC *lite* | LUMETRIC Imaging System |
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
