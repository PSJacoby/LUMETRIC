# Graphical User Interface

The LUMETRIC interface is a single window titled **LUMETRIC – Hardware control**, organized into three tabs. The first tab configures hardware, the second builds the acquisition sequence, and the third defines live visualization. Starting an acquisition hides this window and opens a separate live **Acquisition Window**. A **Post-Production** window is available for re-analyzing finished experiments.

The same interface drives both operating modes:

- **LUMETRIC** (full system) — hardware-triggered acquisition through the Arduino timing unit. Supports multi-channel sequences, alternating excitation, photoswitching, looping, and sub-millisecond timing.
- **LUMETRIC *lite*** — software-controlled (snap) acquisition with no Arduino. Captures one channel using only the GUI exposure and interval settings.

The **LUMETRIC Lite** checkbox at the top of the Acquisition Settings tab switches between them. When ticked, acquisition runs in snap mode with no serial communication and no channel switching; when unticked, the full hardware-triggered path is used.

!!! Note "Mode terminology"
    *LUMETRIC lite* corresponds internally to snap mode (`USE_CIRCULAR_BUFFER = false` / simple snap). The full system corresponds to circular-buffer, hardware-triggered acquisition.

---

## Window structure

- **Tab 1 — Setup Configuration:** load, generate, reload, and save the `.properties` configuration; inspect the available channels.
- **Tab 2 — Acquisition Settings:** select the operating mode, build the sequence table, choose the save folder, set post-acquisition stack options, and start the run.
- **Tab 3 — Data Visualization:** choose the processing mode, configure up to four live graphs, enable corrections, and start the run.
- **Acquisition Window:** live camera view and graphs shown during a run, with event, loop-switch, zoom-reset, and quit controls.
- **Post-Production Window:** reopen a finished experiment folder to refine ROIs and re-export corrected data.

On startup, the interface opens on the Acquisition Settings tab if a configuration was used previously, or on the Setup Configuration tab otherwise.

---

## Tab 1 — Setup Configuration

This tab displays the active configuration and the channel list, and provides the configuration management buttons. The **Current Configuration** panel shows the config file path, camera group, Arduino config group, and serial port. The **Available Channels** panel lists each `PIN_n = channel` mapping.

### Buttons

- **Generate Config from MM** — auto-detects settings from the running Micro-Manager instance (config groups, presets, and the Arduino COM port) and opens a verify-and-save dialog to write a new `.properties` file. Use this to create a configuration without editing text by hand.
- **Load Configuration File** — opens a file chooser to load an existing `.properties` configuration. On success the channel list updates and the interface switches to the Acquisition Settings tab.
- **Reload Current Config** — re-reads the currently loaded configuration file from disk, picking up any external edits.
- **Read Channels from Hub** — queries the Micro-Manager Arduino Switch device and populates the channel list automatically from its presets. Each preset that sets the switch state to a power-of-two value becomes a channel mapped to the corresponding pin.
- **Save Config** — writes the current channel/pin configuration back to the loaded config file (asks for confirmation before overwriting).

!!! Note "LUMETRIC vs LUMETRIC *lite*"
    The Arduino config group, serial port, and **Read Channels from Hub** apply to the full system only. In *lite* mode, only the camera configuration is required; the channel list and Arduino fields are not used during acquisition.

---

## Tab 2 — Acquisition Settings

This tab builds the acquisition sequence and starts the run.

### Mode selection

- **LUMETRIC Lite** (checkbox) — when ticked, runs single-channel snap acquisition using only the GUI exposure settings, with no Arduino and no multi-channel automation. When unticked, runs the full hardware-triggered sequence. A label beside it reads *"← No Arduino or multi-channel setup required."*

### Sequence table

Each row defines one acquisition step. Up to 32 rows are supported. A **Row** number column (bold, not editable) labels each step.

| Column | Purpose |
|--------|---------|
| **Channel** | Illumination channel(s) for the step. Clicking the cell opens a checkbox dialog listing the available channels; select one or several. Empty means a photoswitching row. |
| **Exposure [ms]** | LED on-time and camera exposure per frame (0–60000). `0` creates a photoswitching row (no camera trigger). |
| **Frame Interval [ms]** | Full frame period including exposure and the inter-frame gap (0–60000). Must be ≥ exposure. |
| **Frames** | Number of frames to capture (0–60000). `0` runs until a loop ends or Quit is pressed. |
| **Loop Goto** | 1-based row to jump back to once the frame count is reached. Auto-filled to the next row; the last row defaults to looping back to the first. Editing a cell marks it as user-set and stops auto-updates. |
| **Loop Times** | Number of loop repetitions. Empty or `0` means infinite (shown as `0`). |
| **Constant Illum** | Channel(s) held on during the inter-frame gap. The camera trigger bit is masked out automatically, and a channel already in the row's Channel list cannot also be a constant-illumination channel. |
| **Loop Switch Goto** | 1-based row to jump to when the **Loop Switch** button is pressed during acquisition. Empty means disabled (shown as `none`). |

Numeric fields are clamped to the 0–60000 range as they are entered.

!!! Note "LUMETRIC vs LUMETRIC *lite*"
    Channel switching, Constant Illum, Loop Goto / Loop Times, Loop Switch Goto, and photoswitching rows are hardware features executed by the Arduino. In *lite* (snap) mode the run uses the GUI exposure and interval only; multi-row channel automation does not apply.

### Sequence controls

- **Add Row** — appends a new sequence row with default values.
- **Remove Row** — deletes the currently selected row (prompts if no row is selected).
- **Validate Sequence** — checks the table for errors before starting; the same validation runs automatically when an acquisition is started.
- **Save Settings** — exports the current acquisition table and visualization settings to a CSV file.
- **Load Settings** — restores acquisition and visualization settings from a previously saved CSV file.
- **Use hardcoded test table (case 69)** (checkbox) — bypasses the serial table upload and loads a fixed test pattern directly on the Arduino (one LED, 100 ms exposure, 1000 ms interval). Intended for hardware bring-up and diagnostics.
- **OK – Start Acquisition** — validates the sequence, asks for a save folder if none is set, captures all GUI settings, hides the main window, and launches the acquisition.
- **Cancel** — closes the interface without starting.
- **Open Post-Production** — opens a finished experiment folder to refine ROIs and re-export data (see [Post-Production](#post-production-window)).

### Save folder and post-acquisition options

- **Data Directory / Select Folder** — chooses the directory where experiment data is written. The selected path is remembered between sessions; a folder is required before acquisition can start.
- **Convert to Stack** (checkbox, on by default) — after acquisition, combines the individual `Frame_XXXXX.tif` files into a single stack and deletes the originals.
- **Stack per Row** (checkbox) — after acquisition, creates one TIFF stack per sequence row. Can be combined with **Convert to Stack**.

!!! Tip "Images are always saved"
    Every acquired frame is written to disk regardless of the stack options or the **Live Pictures** checkbox.

---

## Tab 3 — Data Visualization

This tab configures the real-time analysis and graphs. Up to four graphs can be displayed during acquisition.

### Processing mode

- **Mode** (combo: Temporal / Spatial)
    - **Temporal** — compares ROI intensities across frames over time. Graph types: **Intensity** and **Ratio** (frame ratio, e.g. odd/even frames).
    - **Spatial** — compares two halves of the same frame (dual-view / split-image). ROIs are drawn on one half and mirrored to the other. Graph types: **Split ratio** and region intensities.

Selecting Spatial reveals the **Spatial Configuration** and **Bleedthrough Correction** panels.

### Spatial configuration (Spatial mode only)

- **Split Axis** (combo: Horizontal / Vertical) — the axis along which each frame is split.
- **ROI Region** (combo) — which half the ROIs are drawn on. Options follow the split axis: Upper/Lower for Horizontal, Left/Right for Vertical.

### Graph table

| Column | Purpose |
|--------|---------|
| **Graph** | Graph number (1–4). |
| **Type** | Graph type for this row. In Temporal mode: `none`, `Intensity`, `Ratio`. In Spatial mode: `none`, split-ratio directions (e.g. Upper/Lower), and region intensities (e.g. Intensity Upper). |
| **Numerator** | (Temporal only) Sequence row whose frames feed the graph. `All Rows` accepts frames from any row — useful for single-row experiments or plain intensity graphs. |
| **Denominator** | (Temporal only) Sequence row used as the divisor for `Ratio` graphs. Locked to `1` for `Intensity`. |

The Numerator/Denominator dropdowns list the available sequence rows and update automatically when rows are added or removed.

### Corrections

- **Enable Background Correction** (checkbox) — subtracts a designated background ROI from all measured ROIs. The background ROI is selected after starting, during ROI setup. Available in both modes.
- **Enable Bleedthrough Correction** (checkbox, Spatial mode only) — subtracts a scaled source-half signal from the mirror half to correct spectral bleedthrough. Enabled only when Background Correction is also active.
    - **Direction** (combo) — the physical bleedthrough direction (e.g. Upper → Lower), independent of where ROIs are drawn.
    - **Factor** (text field) — the bleedthrough correction factor; accepts `,` or `.` as the decimal separator.

### Start

- **Live Pictures** (checkbox) — shows the live camera view in the Acquisition Window during the run. Controls GUI display only; images are always saved. Uncheck to reduce overhead during fast or long acquisitions.
- **OK – Start Acquisition** — same as on the Acquisition Settings tab: validates, asks for a save folder if needed, captures all settings, and launches the run. If validation fails, the interface switches to the Acquisition Settings tab to show the errors.

!!! Note "LUMETRIC vs LUMETRIC *lite*"
    Temporal/Spatial analysis, live graphs, background and bleedthrough correction, and post-processing are available in **all** modes. Per-row graph filtering is most useful for multi-row (full-system) sequences; in *lite* mode a single row is used, so `All Rows` is the natural Numerator setting.

---

## Acquisition Window

Starting an acquisition opens an inline window containing the live camera view (if **Live Pictures** is enabled) and the configured graphs. ROIs drawn at the start are overlaid on the live image. Graphs update in real time, throttled to roughly 250 ms.

### Controls

- **Event** — logs a timestamp and draws a vertical marker line across all active graphs, for annotating stimulus or intervention times.
- **Loop Switch** — sends the switch command to the Arduino, causing the currently active row to jump to its **Loop Switch Goto** target. The Arduino confirms with a notification packet, and the live graphs' row assignments are corrected retroactively so plotting stays consistent. *(Full system only.)*
- **Reset X / Reset All** — restores automatic axis bounds for one graph or for all graphs.
- **Quit Acquisition** — stops the Arduino (full system), stops the run, exports all graph data and event timestamps to CSV, optionally assembles the image stack, and returns to the main interface.

---

## Post-Production Window

Opened via **Open Post-Production**, this window re-analyzes a completed experiment without re-acquiring. It loads settings from the experiment's `AquisitionSettings.csv`, resolves the image source (the `images/` folder, a single `Image_Stack.tif`, or per-row stacks), and shows a reference frame for ROI drawing.

### Workflow and controls

1. Select an existing experiment folder containing `AquisitionSettings.csv`.
2. Settings load automatically.
3. Draw ROIs in the ROI Manager, or restore a previously saved ROI set.
4. **Recalculate** — re-measures all stack slices with the current ROIs and refreshes the graphs.
5. **Export** — writes corrected data to a new numbered sub-folder beside the original experiment.

Correction levels exported depend on what is enabled:

- **RAW** — always exported.
- **BGcorr** — when Background Correction is enabled.
- **BTcorr** — when Bleedthrough Correction is enabled (Spatial only).
- **BTDEcorr** — when **Direct Excitation correction** is enabled. This post-processor-only correction subtracts a scaled acceptor offset to remove direct excitation of the acceptor by the donor laser. It requires Bleedthrough Correction, uses a reference stack and a frame range to compute the per-ROI offset, and follows the same direction setting as bleedthrough.

---

## Output files

Each run creates a timestamped experiment folder containing:

- `Frame_XXXXX.tif` individual frames (5-digit zero-padded), or an assembled `Image_Stack.tif` / per-row stacks when stack conversion is enabled
- `AquisitionSettings.csv` (human-readable) and `AquisitionSettings_Arduino.csv` (pin/byte values)
- `Graph{N}_{type}_{frameFilter}_{corrLevel}.csv` for each active graph (corrLevel: RAW, BGcorr, BTcorr, BTDEcorr)
- Event timestamps as CSV
- `RoiSet.zip` (ImageJ format)

---

## Capabilities summary

| Capability | LUMETRIC *lite* (Snap) | LUMETRIC (Hardware-triggered) |
|-----------|------------------------|-------------------------------|
| Additional hardware | None | Arduino GIGA R1 + wiring |
| Camera triggering | Software-controlled | TTL-triggered |
| Timing precision | ≈10–50 ms | <1 ms |
| Fast imaging (<150 ms interval) | No | Yes |
| Multi-step sequence table (up to 32 rows) | No | Yes |
| Multi-channel / alternating excitation | No | Yes |
| Constant illumination during gaps | No | Yes |
| Photoswitching / pause rows | No | Yes |
| Looping (goto / repeat) and Loop Switch | No | Yes |
| Live ROI intensity, ratio, and FRET plots | Yes | Yes |
| Background and bleedthrough correction | Yes | Yes |
| Automatic TIFF saving and stack assembly | Yes | Yes |
| Post-processing and re-export | Yes | Yes |
