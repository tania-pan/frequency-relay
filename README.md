# Frequency Relay

A FreeRTOS-based application running on a Nios II soft-core processor (Intel/Altera Cyclone V FPGA).

## Hardware & Software Requirements
* **Target Hardware:** Intel/Altera Cyclone V FPGA
* **Processor:** Nios II Soft-Core Processor
* **IDE:** Nios II Software Build Tools (SBT) for Eclipse
* **RTOS:** FreeRTOS

## Project Structure
* `frequency_relay/`: Main application source code (`.c` / `.h` files) and FreeRTOS tasks.
* `frequency_relay_bsp/`: Board Support Package containing hardware abstractions, drivers, and FreeRTOS source files.

## Build Instructions
1. Open Nios II SBT for Eclipse and select your workspace.
2. Import both the `frequency_relay` and `frequency_relay_bsp` projects into your workspace.
3. Right-click the `frequency_relay_bsp` project -> **Nios II** -> **Generate BSP**.
4. Right-click the `frequency_relay_bsp` project -> **Build Project**.
5. Right-click the `frequency_relay` main project -> **Build Project**.

## FreeRTOS Architecture Overview
This application relies on the following FreeRTOS inter-task communication/synchronization mechanisms:

**Queues:**
* `buttonCmdQ`: Handles incoming button commands.
* `kbdQ`: Handles keyboard inputs.
* `freqDataQ`: Passes calculated or measured frequency data between tasks.

**Semaphores & Mutexes:**
* `peakReadSem`: Binary semaphore used to signal when a peak reading is ready.
* `loadStatusMutex`: Mutex protecting shared load status variables.
* `systemStatusMutex`: Mutex protecting shared system status variables.
* `timingLogMutex`: Mutex protecting the timing log or logging interface.

## Running the Application
1. Ensure your Cyclone V board is connected and powered on.
2. Program the FPGA with your `.sof` file via the Quartus Programmer.
3. In Eclipse, right-click the `frequency_relay` project -> **Run As** -> **Nios II Hardware**.