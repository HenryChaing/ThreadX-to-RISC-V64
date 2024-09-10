# Introduce
Port Eclipse ThreadX RTOS to RISC-V64 architecture, verification device is Milk-V Duo (cv180x chip, dual-core C906, instruction set is RV64GCV (RV64IMAFDCV)). ThreadX runs on the small core C906L without the MMU, and Linux runs on the large core C906B.

This project is modified based on the official Milk-V Duo SDK (tag: DUo-v1.0.9), and only the cv180x series CPU is verified for the time being.

# Compile and Build
## Full SDK Compilation
Pull the project into the official sdk of Milk-V duo, and change the FREERTOS_PATH environment variable of the duo-buildroot-SDK /build/milkvsetup.sh script to the relative path of the warehouse:
```sh
#FREERTOS_PATH="$TOP_DIR"/freertos # Modify this item
FREERTOS_PATH="$TOP_DIR"/threadx
```
Run the official Milk-V Duo compilation script, -j[x] to enable X-core compilation (see the official Milk-V Duo repository README or official Wiki for details)
```
# ./build.sh milkv-duo -j6
```
The fip.bin file generated after compilation contains ThreadX.

## Compile fip.bin Separately
If you do not want to compile all SDK files each time, copy the rtos_build.sh script of the project to the SDK root directory (and the build.sh directory) and run the script. By default, only fip.bin will be compiled and generated.
```
# ./rtos_build.sh milkv-duo -j6
```
You can also comment out clean_uboot and enable make rtos_clean to avoid repeated compilation of contents other than RTOS in fip.bin
```sh
#clean_uboot || return $?

_build_uboot_env
cd "$BUILD_PATH" || return
make rtos-clean

build_uboot || return $?
```

(If you only want to compile the RTOS without modifying the SDK, you can directly run the threadx/build_cv180x.sh script to generate cvirtos.bin. However, you need to manually export the environment variables required for compilation in advance, and manually run the fiptool.py script to synthesize the fip.bin file after compilation is complete)

# Repository Directory Structure
```
├── build_freertos.sh  // The cv181x build script is not needed
├── rtos_build.sh      // Copy the modified build script to the SDK root directory and run it
├── cvitek             // Official BSP
├── threadx            // ThreadX source file and port file
│   ├── common         // ThreadX core source file
│   ├── ports          // Chip architecture and compiler specific files
│   │   ├── risc-v64   // Documents related to RISC-V64 architecture migration
│   │   └── ...
│   └── ...
└── README-zh.md       // REAMME
```

# Main Porting Content
1. Modify the tx_initialize_low_level.S file to add the first free memory address, configure the trap handler entry, and jump to the timer interrupt configuration function port_specific_pre_initialization.
2. Implement a trap handler. Following the official ThreadX convention, the trap handler is also placed in the tx_initialize_low_level.S file (see the FreeRTOS implementation in Milk-V Duo).
3. Add the file port_specific_pre_initialization.c related to timer interrupt configuration.
4. Modify the processing of mstatus register after trap exit in tx_thread_context_save.S, tx_thread_context_restore.S, and tx_thread_schedule.S files to avoid crash due to floating point problems.
5. Modify tx_port.h header file, and add tx_user.h file (located in threadx/cvitek/kernel/include/riscv64 /tx_user.h)

# Example
Provide two simple example, located in the threadx cvitek/task/comm/SRC/riscv64/comm_main.c file, through macro```USE_MAILBOX_EXAMPLE```control conditional compilation.
1. ThreadX's default demo (added print log, see demo_threadx.c in threadx/threadx/samples for the original code).
2. Milk-V Duo's official big and small core mailbox communication demo, through the big core's Linux notification small core's RTOS to control LED.

# Reference
ThreadX repo：https://github.com/eclipse-threadx/threadx/

Milk-V Duo repo： https://github.com/milkv-duo/duo-buildroot-sdk/

Milk-V Duo online Wiki：https://milkv.io/zh/docs/duo/overview
