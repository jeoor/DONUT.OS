# Build Instructions

## Prerequisites

- [Arduino IDE](https://www.arduino.cc/en/software) (2.x recommended)
- USB-C cable for Cardputer
- M5Stack Cardputer or Cardputer-ADV

## Step 1: Install Board Support

1. Open Arduino IDE → **File** → **Preferences**
2. In **Additional Board Manager URLs**, add:
   ```
   https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json
   ```
3. Open **Tools** → **Board** → **Boards Manager**
4. Search for **M5Stack** and install the M5Stack board package

## Step 2: Install Libraries

Open **Tools** → **Manage Libraries** and install:

- `M5Cardputer`
- `M5GFX`

These libraries should pull in `LovyanGFX` as a dependency.

## Step 3: Select Board

1. Connect Cardputer via USB-C
2. **Tools** → **Board** → Select your Cardputer variant
3. **Tools** → **Port** → Select the COM port for your device

> The exact board name may vary depending on your M5Stack board package version. Look for entries containing "Cardputer" or "M5Stack-S3".

## Step 4: Open Firmware

Choose one of the two firmware versions:

| Version | Path |
|---------|------|
| Mainline (frozen) | `firmware/donut_cardputer_os_final_release_checked/donut_cardputer_os_final_release_checked.ino` |
| QoL (recommended) | `firmware/donut_cardputer_os_qol_go_deepsleep_fixed/donut_cardputer_os_qol_go_deepsleep_fixed.ino` |

## Step 5: Compile and Upload

1. Click **Verify** (checkmark icon) to compile
2. Click **Upload** (arrow icon) to flash
3. Wait for the device to restart

## Troubleshooting

- **Compilation errors:** Ensure M5Cardputer and M5GFX libraries are installed and up to date.
- **Upload fails:** Hold the GO button on Cardputer while uploading to enter download mode.
- **Board not found:** Check USB cable (must support data, not charge-only) and drivers.
