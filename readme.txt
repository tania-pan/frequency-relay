COMPSYS 723 - Assignment 1: Low Cost Frequency Relay (LCFR)
Christian Motta and Tania Pan

-- HARDWARE SETUP --
Plug the power jack into the DE2-115 board.
Plug the VGA adapter into the board, and connect its USB power cable to the USB port on the FPGA.
Connect the PS/2 keyboard to the PS/2 port.
Connect the USB Blaster to the board, and plug the other end into your computer.
Ensure you have the standard Altera USB Blaster drivers installed (included with Quartus Prime Lite 18.1).

-- FPGA CONFIG (QUARTUS) --
Open Quartus Prime Lite 18 and click on the Programmer icon.
Click "Hardware Setup" and ensure the USB Blaster is recognized and selected (should show as "USB-Blaster [USB-0]" in JTAG mode).
Click "Add File", navigate to your project directory, and select the "freq_relay_controller.sof" file.
Ensure the "Program/Configure" checkbox is ticked.
Click "Start" and wait for the progress bar in the top right to reach 100% (Successful).
Note: If this fails, restart your computer and verify you are using the correct Quartus version.

-- SOFTWARE COMPILATION & EXECUTION (ECLIPSE) --
Open "Nios II Software Build Tools for Eclipse (Quartus Prime 18.1)".
In your workspace, go to File -> Import -> "Existing Projects into Workspace".
Import both the "frequency_relay" and "frequency_relay_bsp" projects.
Right-click on the "frequency_relay_bsp" project, go to "Nios II", and select "Generate BSP".
Right-click and "Build" the "frequency_relay_bsp" project.
Right-click and "Build" the main "frequency_relay" project.
Right-click on the "frequency_relay" project and go to Run As -> Run Configurations.
Under "Nios II Hardware", ensure that "frequency_relay.elf" is selected.
Go to the "Target Connection" tab. Ensure the USB Blaster is detected.
Check the boxes for "Ignore mismatched System ID" and "Ignore mismatched timestamp".
Ensure both "Start processor" and "Download ELF file" are checked.
Click "Run". You will see initialization messages in the console, followed by the VGA display turning on.

-- CONTROLS --
KEY3: Toggles the system in and out of Maintenance Mode.
SW0 to SW4: Physical switches that turn the 5 individual loads on or off.
Keyboard '1': Selects the Frequency (TF) threshold for editing.
Keyboard '2': Selects the Rate of Change (TROC) threshold for editing.
(An indicator box on the VGA will display '0' or '1' next to the currently selected threshold).
Keyboard Up Arrow: Increases the selected threshold by 0.5.
Keyboard Down Arrow: Decreases the selected threshold by 0.5.