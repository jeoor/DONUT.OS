# Design Notes

## What DONUT.OS Is (and Is Not)

DONUT.OS is **OS-like firmware** — not a real operating system.

It does not provide:
- Process scheduling
- Memory management
- File system abstraction
- Device drivers
- Multi-tasking

What it does provide:
- A polished, persistent visual interface
- Interactive system panels
- Persistent settings
- Power management (QoL version)
- An experience that *feels* like a small OS

Think of it as a **visual micro system** or **interactive firmware shell** for Cardputer.

## Rendering Pipeline

### Donut Renderer

The 3D donut is rendered entirely in software using fixed-point math and a z-buffer approach:

1. **Torus mesh generation** — parametric torus with configurable segments
2. **Rotation and projection** — 3D to 2D perspective projection
3. **Z-buffer visibility** — per-pixel depth test
4. **Shade buffer** — surface normal dot lighting for shading
5. **Optional glow** — post-process glow effect
6. **Dirty rect composition** — only update changed regions

**Key constraint:** No dynamic allocation (`malloc`, `new`, `String`, etc.) in the render hot path.

### Background Cache

Background rendering uses a half-resolution cache to reduce CPU load:

1. Render background at half resolution
2. Scale up to full resolution
3. Composite donut on top using dirty rect

This allows the perspective checker floor to move smoothly without impacting donut frame rate.

## RGB+ Palette Architecture

RGB+ is a palette-level effect inspired by RGB565 byte-swap aesthetics.

**What it does:**
- Shifts palette colors to create a neon/retro feel
- Applied at the palette level, not the framebuffer level

**What it does NOT do:**
- No actual framebuffer byte swap
- No pollution of UI colors (TopBar, Toast, etc.)
- No RGB+ rail/sweep line layer

This keeps the effect clean and intentional.

## Dirty Rect Composition

Instead of redrawing the entire 240×135 framebuffer every frame, DONUT.OS tracks which regions have changed:

1. Mark changed regions as "dirty"
2. Only re-render dirty regions
3. Compose final frame from cached and updated regions

This significantly reduces CPU usage and improves frame rate.

## Preferences V9

Settings are persisted using the ESP32 `Preferences` library (namespace: `dos`):

- Theme
- Background mode
- RGB+ state
- Glow state
- Brightness
- Quality
- Speed
- Scale
- And more...

**Factory Reset** is triggered by a key chord (R + ~) and clears all stored preferences.

## AutoDim & GO DeepSleep (QoL Version)

### AutoDim

After a configurable idle timeout, the display brightness gradually decreases to a minimum level. Any key press restores full brightness.

### AutoSleep

After a longer idle timeout, the device enters DeepSleep via GO-button simulation.

**Guard:** If the Stopwatch or Timer is actively running, AutoSleep is inhibited to prevent interrupting timed activities.

### GO DeepSleep

Pressing `GO` or `G0` triggers ESP32 DeepSleep. The device can be woken by pressing the GO button again.

**Wake behavior:**
- Device boots fresh
- Preferences are restored
- Display shows the donut

## Thread Safety

DONUT.OS runs primarily on a single core with cooperative multitasking. The render loop and input handling are synchronized by design.
