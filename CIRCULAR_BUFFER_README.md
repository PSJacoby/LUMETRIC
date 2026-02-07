# Circular Buffer Acquisition Implementation

## Overview
The script now supports **Micro-Manager's circular buffer acquisition** for high-performance image capture with real cameras. This replaces the simple snap-based acquisition for production use while maintaining backward compatibility.

## Features Implemented

### 1. Circular Buffer Setup
- Automatically allocates **75% of available Java heap memory** for the buffer
- Calculates and reports approximate frame capacity
- Displays image dimensions and bytes per pixel

### 2. Image Saving (TIFF Format)
- **Images are ALWAYS saved** during acquisition
- Saves each acquired frame as individual TIFF files
- Filename format: `Frame_00001.tif`, `Frame_00002.tif`, etc.
- Saved to the experiment folder automatically
- Progress logging every 100 frames
- **Note**: The "Live Pictures" checkbox controls GUI display only, not image saving

### 3. Buffer Monitoring
- **Real-time buffer usage tracking**
- **Warning at 75%**: Alert every 10 seconds
- **Critical at 90%**: Alert every 5 seconds
- **Overflow detection**: Reports lost frames if buffer overflows
- Final statistics at end of acquisition

### 4. Acquisition Modes
Two modes are available:

#### **Circular Buffer Mode** (Default - for real cameras)
- Uses `mmc.startSequenceAcquisition()`
- Efficient hardware-triggered capture
- Supports continuous high-speed acquisition
- Monitors and reports buffer status

#### **Simple Snap Mode** (for demo camera)
- Uses `mm.live().snap(false)`
- One frame at a time
- Lower performance but works with demo camera

## Live Pictures Checkbox

**Important**: The "Live Pictures" checkbox controls **GUI display only**, not image saving.

- **Images are ALWAYS saved** to disk during acquisition
- **Checkbox CHECKED** ✅: Live view panel is shown in the GUI with real-time image updates (250ms throttle)
- **Checkbox UNCHECKED** ❌: No live view panel (saves GUI resources and improves performance)

This allows you to:
- Run high-speed acquisitions without GUI overhead by unchecking the box
- Still have all frames saved to TIFF files for later analysis
- Enable live view only when you need to monitor the acquisition visually

**Tip**: For maximum performance in long acquisitions, **uncheck** Live Pictures to reduce GUI overhead.

## Configuration

### In Config File
Add to your config file:

**For Demo Camera** (config_example.txt):
```ini
# Demo camera does NOT support hardware triggers!
USE_CIRCULAR_BUFFER = false
```

**For Real Camera with Triggers** (config_RealSetup.txt):
```ini
# Real camera with Arduino triggers
USE_CIRCULAR_BUFFER = true

# Camera configurations
CAMERA_CONFIG_GROUP = Hamamatsu
CAMERA_SNAP_CONFIG = Snapping_and_Live
CAMERA_TRIGGER_CONFIG = GlobalResetLevelTriggerArduinoMode
```

**IMPORTANT**: Circular buffer mode **requires hardware triggers** (from Arduino or external source). The demo camera will **NOT work** with circular buffer mode enabled!

### In Code
You can also toggle it programmatically:

```beanshell
config.useCircularBuffer = true;  // Use circular buffer
config.useCircularBuffer = false; // Use simple snap mode
```

## How It Works

### Acquisition Flow

1. **Setup Phase**
   - Calculate memory allocation (75% of heap)
   - Configure circular buffer
   - Set camera to trigger mode
   - Start sequence acquisition

2. **Acquisition Loop**
   ```
   while (running && images available) {
       - Check buffer level
       - Pop next tagged image from buffer
       - Save image to TIFF (always)
       - Process ROIs for intensity measurements
       - Update live view (throttled to 250ms, if Live Pictures enabled)
       - Update graphs (throttled, with frame filtering)
       - Monitor for buffer overflow
   }
   ```

3. **Cleanup**
   - Stop sequence acquisition
   - Report statistics (frames captured, max buffer depth, overflow status)

### Buffer Monitoring Output

During acquisition, you'll see:
```
Buffer size: 3072 MB
Approximate capacity: 3000 frames
Processed: 100 frames, Buffer: 25
WARNING: Buffer at 78% (2340/3000 frames)
CRITICAL: Buffer at 92% (2760/3000 frames)
Processing rate may be too slow!
```

Final report:
```
========== ACQUISITION COMPLETE ==========
Total frames processed: 1500
Maximum buffer depth: 150 frames
Peak buffer usage: 5%
No buffer overflows detected.
==========================================
```

## Key Functions

### `setupCircularBuffer()`
Configures the circular buffer size based on available memory.

### `saveImageAsTiff(Object img, int frameNumber, String experimentFolder)`
Saves a TaggedImage or MM Image as TIFF format.
- Supports both 8-bit (byte) and 16-bit (short) images
- Handles both Micro-Manager 2.0 Images and TaggedImage objects

### `runCircularBufferAcquisition(...)`
Main acquisition loop using circular buffer.
- Parameters: same as original `runAcquisition()`
- Replaces snap-based acquisition
- Adds buffer monitoring and overflow detection

### `runAcquisition(...)`
Original snap-based acquisition (kept for demo camera compatibility).

## Performance Considerations

### Buffer Size
- Default: 75% of Java heap memory
- Adjust if you experience overflows: increase heap size with `-Xmx` argument

### Processing Speed
- ROI measurements done immediately on each frame
- GUI updates throttled to 250ms to reduce overhead
- Console output reduced (every 10th frame instead of every frame)

### Image Saving
- TIFF saving is fast but disk I/O can be a bottleneck
- Consider using fast SSD storage
- Monitor disk space - images accumulate quickly!

## Troubleshooting

### No Frames Received / Acquisition Stuck
**Symptom**: "WARNING: No frames received for X seconds" or acquisition starts but doesn't capture any frames

**This is the most common issue!**

**Solutions**:
1. **Using Demo Camera?** 
   - Set `USE_CIRCULAR_BUFFER = false` in your config file
   - Demo camera does NOT support hardware triggers
   - Circular buffer mode will wait forever for triggers that never come
   
2. **Arduino/Trigger Source Not Connected**
   - Check Arduino USB connection
   - Verify Arduino is running the trigger sketch
   - Check serial port in config matches Arduino port
   
3. **Camera Not in Trigger Mode**
   - Verify `CAMERA_TRIGGER_CONFIG` is correct in config
   - Camera must be set to external trigger mode
   
4. **Wrong Acquisition Mode**
   - If testing without triggers: use simple snap mode (`USE_CIRCULAR_BUFFER = false`)
   - If using triggers: use circular buffer mode (`USE_CIRCULAR_BUFFER = true`)

**Quick Fix**: Press **Quit** button, set `USE_CIRCULAR_BUFFER = false` in config, restart acquisition.

### Buffer Overflow
**Symptom**: Message "BUFFER OVERFLOW DETECTED"

**Solutions**:
1. Reduce acquisition frame rate (if using Arduino triggers)
2. Increase Java heap size in Micro-Manager settings
3. Reduce ROI processing complexity
4. Disable image saving temporarily

### Memory Errors
**Symptom**: OutOfMemoryError

**Solutions**:
1. Increase Micro-Manager heap size:
   - Edit `ImageJ.cfg` or `Micro-Manager.cfg`
   - Increase `-Xmx` value (e.g., `-Xmx8000m` for 8GB)
2. Reduce buffer size in `setupCircularBuffer()`

### Images Not Saving
**Symptom**: No TIFF files created

**Solutions**:
1. Verify experiment folder exists and has been created
2. Check disk space
3. Verify write permissions
4. Check console for error messages during saving

## Example Usage

### For Real Camera with Triggered Acquisition
```beanshell
// In config file or script
config.useCircularBuffer = true;
config.cameraTriggerConfig = "GlobalResetLevelTriggerArduinoMode";

// Enable live view GUI (optional - images are always saved)
livePicsCheckbox.setSelected(true);

// Start acquisition - will use circular buffer automatically
// All frames will be saved to experiment folder as TIFF files
```

### For Demo Camera Testing
```beanshell
// In config file or script
config.useCircularBuffer = false;

// Disable live view for better performance (optional)
livePicsCheckbox.setSelected(false);

// Start acquisition - will use simple snap mode
// All frames still saved to disk
```

## Technical Details

### Memory Calculation
```java
long maxMem = Runtime.getRuntime().maxMemory();
long bufferMem = (long)(maxMem * 0.75);  // 75% for buffer
int bufferSizeMB = (int)(bufferMem / (1024 * 1024));

// Approximate capacity
int approxCapacity = bufferMem / (width * height * bytesPerPixel);
```

### TaggedImage Handling
The code supports both:
- **Micro-Manager 2.0 Image objects** (from `mm.data()`)
- **TaggedImage objects** (from `mmc.popNextTaggedImage()`)

Images are converted to ImageJ's `ImageProcessor` for ROI analysis.

## Integration with Existing Features

The circular buffer acquisition **maintains full compatibility** with:
- ✅ ROI intensity measurements
- ✅ Live view display (throttled)
- ✅ Multi-graph visualization with frame filtering
- ✅ CSV data export
- ✅ Event logging
- ✅ Experiment folder structure

## Future Enhancements

Potential improvements:
1. Variable buffer size configuration
2. Multi-page TIFF saving (reduce file count)
3. Compression options
4. Metadata embedding in TIFF files
5. Buffer usage graph in GUI

---

**Created**: February 7, 2026  
**Author**: Implementation based on user requirements  
**Script**: GUI_Test.bsh
