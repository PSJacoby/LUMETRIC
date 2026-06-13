# How to install

!!! Important "Prerequisites"
    - A Windows operating system (others might work but are not tested)
    - Manufacturer drivers for all hardware are installed

## MicroManager and LUMETRIC *lite*

If you already operate your Microscope using MicroManager (2.0.3 or higher), you may skip ahead to [Implementing LUMETRIC](#implementing-lumetric-as-quick-access-button)

### Installing MicroManager and integrating your devices

1. **Download** the latest version of Micromanager from  [MicroManager](<https://micro-manager.org/Download_Micro-Manager_Latest_Release>).
2. **Install** Micro-Manager and launch the application.

    !!! Tip "Additional documentation"
        Detailed installation instructions are provided in the official Micro-Manager
        documentation: [MM Installation](https://micro-manager.org/Micro-Manager_Installation_Notes)

3. **Connect your hardware** to Micromanager:
   **A quick summary:**
   i. Open Devices > Hardware Configuration Wizard ...
   ii. Create a new configuration file
   iii. Add the camera and light source by selecting the appropriate device adapters from the list of available devices.

    !!! Tip "Documentation"
        Official Micro-Manager documentation for these steps: [Hardware Configuration Wizard](https://micro-manager.org/Micro-Manager_Configuration_Guide)
        For the individual device adapters: [Device Support](https://micro-manager.org/Micro-Manager_Configuration_Guide)

    ??? Tip "Troubleshooting: If device detection is unsuccessful"
        The port detection and standard settings are largely automated in MM. You will find device adapters for the most common devices. Check out the individual docs of the adapters [Device Support](https://micro-manager.org/Micro-Manager_Configuration_Guide). If this does not work out:
        - Verify that the correct manufacturer drivers are installed
        - Check the Windows Device Manager for device visibility
        - Confirm correct COM port assignment for serial devices
        - Consult the manufacturer’s hardware documentation

4. **Configure your Presets:**
   It is recommended to at least define individual groups for camera and light source. Depending on your hardware, MM allows also integration of microscope bodies, stages and more.
   **Include as many settings as needed** in your groups and presets but **as little as possible**. Over-defining presets on shared microscopes may lead to silent setting changes.
   If you use a TTL-controlled (Arduino controlled) light source, you will find further information in [Setting up the Arduino](TODO).
   For the camera, it is recommended to define a "Snapping and live" mode, where you use the internal trigger mode (thus controlled via serial connection to MM).
   If the full LUMETRIC-Version is to be used, configure a second camera preset "LUMETRIC" that allows TTL-triggering via Arduino. Example camera presets can be found in table X below. Depending on the camera, the nomenclature might differ. If uncertain which settings to use for TTL-triggering mode, check the camera manual for a behavior as depicted in figure X.

            TODO: Include table of settings and a diagram of pulses!

5. Before continuing, acquire some test images using the configured "Snapping and live" camera mode and illumination presets.
6. When you close MM using the File > close option, it shuts down orderly and will remember your window positioning the next time.
Also, it will ask you if you want to save your Hardware configuration. Please do so.

    !!! Tip "Getting started with MicroManager"
        A detailed description of the Micro-Manager user interface and basic workflows is available in the official user guide: [MM User Guide](https://micro-manager.org/Version_2.0_Users_Guide)

### Implementing LUMETRIC as Quick-Access-Button

LUMETRIC *lite* is distributed as a Micro-Manager BeanShell script and does not
require compilation or modification of Micro-Manager installation files.

1. Download the LUMETRIC-Repository ([LUMETRIC](https://github.com/PSJacoby/LUMETRIC))  using the `<> Code` button and select *Download ZIP*. After unpacking, you will find LUMETRIC.bsh inside.  This is all you need to run LUMETRIC *lite*.
2. In MM, go to **Tools > Quick Access Panel > Create New Panel**. In the left corner of the new pop-up window, click on the wheel-symbol. Here, you will add LUMETRIC in the next step, but it also possible to e.g. add presets or other commodities like often used light source channels or similar.
3. Drag-and-drop the "**Run Script**" control button on a free green rectangle. This will open a file-browser. Select the **LUMETRIC.bsh** script.
4. By clicking the wheel symbol again, you finish the process.
5. MM will open the Quick access panel automatically again when opening the program, as long as you do not close the panel itself. To avoid accidental panel loss, I recommend to save it via **Tools > Quick Access Panel > Save Settings**.

!!! Tip "Alternative execution method"
    The Quick Access Panel is a convenience feature and not required. You can also execute LUMETRIC via **Tools > Script Panel**. There, click "**add**" and choose **LUMETRIC.bsh**, Click "**Run**" to execute the script.

### Run your first test experiment on LUMETRIC *lite*

1. Open LUMETRIC via your predefined Quick-Access Button
2. Choose: "**LUMETRIC Lite**"

A introduction in how to set-up an experiment with LUMETRIC *lite*  can be found in [LUMETRIC lite](../LUMETRIC%20GUI/Singlechannel). If you would like to start with a concise overview of all options you are offered by LUMETRIC, check out [LUMETRIC GUI](../LUMETRIC%20GUI/Interface).
If you want to set up a full LUMETRIC system, continue with [Aquisition Control Box](./ArduinoAssembly).
