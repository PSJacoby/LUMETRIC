# LUMETRIC Architecture Diagrams

---

## 1. High-Level Acquisition Lifecycle

```mermaid
flowchart TD
    A([Script Start]) --> B[Build GUI\nJFrame + 3 Tabs]
    B --> C{Auto-load\nlast config?}
    C -- yes --> D[config.loadFromFile\nupdateSettingsDisplay]
    C -- no --> E[GUI shown\nframe.setVisible]
    D --> E

    E --> F([User: OK - Start Acquisition])
    F --> G[validateSequence]
    G -- fail --> E
    G -- pass --> H[captureFromGUI\nsnapshot all GUI state into\nconfig.acqSettings]
    H --> I[frame.dispose]
    I --> J[[Background Thread]]

    J --> K[createExperimentFolder\nExperiment_timestamp/images/]
    K --> L{Visualization\nneeded?}
    L -- yes --> M[setupVisualizationROIs\nsnap image, crop half if Spatial\nuser draws ROIs, save RoiSet.zip]
    L -- no --> N
    M --> N[exportTableToCSV\nAquisitionSettings.csv\nAquisitionSettings_Arduino.csv]
    N --> O[[EDT: createInlineAcquisitionWindow\nbuild chart panels, send table to Arduino]]

    O --> P{Mode?}
    P -- Circular Buffer\nhardware triggered --> Q[[runCircularBufferAcquisition]]
    P -- Simple Snap\nor Advanced Sequence --> R[[runAcquisition]]

    Q --> S([User: Quit])
    R --> S

    S --> T[Send byte 66 to Arduino\nstop trigger]
    T --> U[exportGraphDataToCSV\none CSV per active graph]
    U --> V[exportEventTimesToCSV\nEvent-Data.csv]
    V --> W{convertToStack?}
    W -- yes --> X[convertTiffsToStack\nbuild Image_Stack.tif\ndelete individual TIFFs]
    W -- no --> Y
    X --> Y([Done])
```

---

## 2. Circular Buffer Acquisition Path (Hardware TTL)

```mermaid
flowchart TD
    A([runCircularBufferAcquisition]) --> B[setupCircularBuffer\n75% JVM max memory]
    B --> C[Pre-arm cleanup\nbyte 66 → stop Arduino\nsetAutoShutter false\nclearCircularBuffer]
    C --> D[setConfig cameraTriggerConfig\nsleep 1500ms]
    D --> E[startSequenceAcquisition\n1000 frames, DCAM mode]
    E --> F[sleep 500ms\nflush stale frames]
    F --> G[Send byte 61\nArduino starts TTL pulses]

    G --> H{Frame\navailable?}
    H -- yes --> I[popNextTaggedImage\nconvert to ImageProcessor]
    I --> J[saveImageAsTiff\nFrame_NNNNN.tif]
    J --> K[measureROIs\nmean intensity per ROI]
    K --> L[buildDataRow\nframe, timestamp, roi values]
    L --> M{250ms\nthrottle?}
    M -- yes --> N[updateLiveView\nscaled image + ROI overlay]
    M -- no --> O
    N --> O[updateGraphs\nsee Graph Update detail]
    O --> H
    H -- sequence done\nor running=false --> P

    P[byte 66 → stop Arduino\nrelease-kicker pulse\nstopSequenceAcquisition\nsetConfig cameraSnapConfig\nsetAutoShutter true]
    P --> Q{convertToStack?}
    Q -- yes --> R[convertTiffsToStack]
    Q -- no --> S
    R --> S([acqFrame.dispose])
```

---

## 3. Software Snap Acquisition Path

```mermaid
flowchart TD
    A([runAcquisition]) --> B{useSimpleSnapMode?}

    B -- Simple Snap --> C[setConfig cameraSnapConfig\nsetAutoShutter true]
    C --> D{running?}
    D -- yes --> E[mm.live.snap\nblocking]
    E --> F[saveImageAsTiff]
    F --> G[measureROIs]
    G --> H[buildDataRow]
    H --> I[updateLiveView\nthrottled 250ms]
    I --> J[updateGraphs\nthrottled 250ms]
    J --> K[sleep\nframeInterval - elapsed ms]
    K --> D
    D -- stopped --> L[convertTiffsToStack\nif requested]
    L --> M([acqFrame.dispose])

    B -- Advanced Sequence --> N[Walk sequenceRows\nby currentStepIndex]
    N --> O[Set channel + exposure\nvia setConfig]
    O --> P[snap N frames loop\nsame as Simple Snap]
    P --> Q{loopGoto\nor next row?}
    Q -- continue --> N
    Q -- done --> L
```

---

## 4. Graph Update Logic

```mermaid
flowchart TD
    A([updateGraphs\nframe, time, roiValues]) --> B{For each\ngraph row}

    B --> C[shouldProcessFrame\ncheck: every / odd / even\n/ custom interval + offset]
    C -- skip --> B
    C -- process --> D{graphType?}

    D -- Intensity --> E[append roiValues\ndirectly to XYSeries\nvia SwingUtilities.invokeLater]

    D -- Frame Ratio\ne.g. Ch1/Ch2 --> F{N-frame\ncycle?}
    F -- every 2nd\nodd/even --> G[cacheFrameData\nstore in even or odd slot]
    G --> H{Both slots\nfilled?}
    H -- no --> B
    H -- yes --> I[calculateFrameRatio\nnumerator ROIs / denominator ROIs]
    I --> J[append ratio to XYSeries]

    F -- N-frame\ncycle --> K[cacheFrameDataAtPosition\np1...pN]
    K --> L{All N positions\nfilled?}
    L -- no --> B
    L -- yes --> I

    D -- Split Ratio\nSpatial mode --> M[calculateSpatialRatio\nsplit roiValues into\nsource half / mirror half]
    M --> N[compute per-pair ratio\nUp/Down, Left/Right etc.]
    N --> O[append to XYSeries\nvia SwingUtilities.invokeLater]

    E --> B
    J --> B
    O --> B
```

---

## 5. GUI Structure & Button Map

```mermaid
flowchart LR
    subgraph MainWindow["Main Window (JFrame)"]
        subgraph T1["Tab 1: Setup Configuration"]
            LC[loadConfigButton\n→ config.loadFromFile\n→ updateSettingsDisplay]
            RC[reloadConfigButton\n→ config.loadFromFile]
            SC[saveConfigButton\n→ config.saveToFile]
            HB[readFromHubButton\n→ config.loadFromHub]
        end

        subgraph T2["Tab 2: Acquisition Settings"]
            ADD[addButton\n→ tableModel.addRow]
            REM[removeButton\n→ tableModel.removeRow]
            VAL[validateButton\n→ validateSequence]
            SAV[saveSettingsButton\n→ saveSettingsToCSV]
            LOD[loadSettingsButton\n→ loadSettingsFromCSV\n→ parseCSVLine]
            FLD[selectFolderButton\n→ update prefs + label]
            OK[okButton\nOK - Start Acquisition]
        end

        subgraph T3["Tab 3: Data Visualization"]
            VIZ[vizStartButton\nOK - Start Acquisition\n same path as okButton]
            PMC[processingModeCombo\n→ vizTableModel.setProcessingMode\n→ typeEditor.setProcessingMode]
            SAX[splitAxisCombo\n→ repopulate roiSourceCombo\n→ typeEditor.setProcessingMode]
            RSC[roiSourceCombo\n→ typeEditor.setProcessingMode]
        end
    end

    OK --> ACQ
    VIZ --> ACQ

    subgraph ACQ["Acquisition Window (inline JFrame)"]
        EVT[eventButton\n→ add ValueMarker to plots\n→ log timestamp]
        LSW[loopSwitchButton\n→ send byte 67 to Arduino]
        RZ[resetAllButton / resetXButton\n→ chart.restoreAutoBounds]
        QT[quitButton\n→ byte 66 to Arduino\n→ exportGraphDataToCSV\n→ exportEventTimesToCSV\n→ acqFrame.dispose]
    end

    subgraph ROIWin["ROI Setup Mini-Window"]
        DONE[doneButton\n→ lock.notifyAll done=true]
        CNC[cancelButton\n→ lock.notifyAll cancelled=true]
    end
```
