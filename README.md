# Frequency Relay
COMPSYS723 Assignment 1
## Hardware & Software Requirements
* **Target Hardware:** Intel/Altera Cyclone V FPGA
* **Processor:** Nios II Soft-Core Processor
* **IDE:** Nios II Software Build Tools for Eclipse
* **RTOS:** FreeRTOS

## Build Instructions
1. Open Nios II SBT for Eclipse and select your workspace.
2. Import both the `frequency_relay` and `frequency_relay_bsp` projects into your workspace.
3. Right-click the `frequency_relay_bsp` project -> **Nios II** -> **Generate BSP**.
4. Right-click the `frequency_relay_bsp` project -> **Build Project**.
5. Right-click the `frequency_relay` main project -> **Build Project**.

## Running the Application
1. Ensure your Cyclone V board is connected and powered on.
2. Program the FPGA with your `.sof` file via the Quartus Programmer.
3. In Eclipse, right-click the `frequency_relay` project -> **Run As** -> **Nios II Hardware**.