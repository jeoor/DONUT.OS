# Controls

Complete controls reference for DONUT.OS.

## Key Bindings

| Key | Action | Notes |
|-----|--------|-------|
| `Space` | Theme | Cycle through color themes |
| `B` | Background mode | Cycle: NONE → FLOW → CHECK+FLOW |
| `C` | RGB+ toggle | Enable/disable RGB+ palette effect |
| `T` | Floating text | Toggle floating text overlay |
| `K` | Help panel | Show/hide help overlay |
| `G` | Glow toggle | Enable/disable glow effect |
| `H` | Quality toggle | Toggle rendering quality |
| `P` | Pause | Pause/resume animation |
| `F` | FPS target | Cycle FPS target values |
| `;` | Speed down | Decrease rotation speed |
| `.` | Speed up | Increase rotation speed |
| `+` | Scale up | Increase donut scale |
| `-` | Scale down | Decrease donut scale |
| `[` | Brightness down | Decrease display brightness |
| `]` | Brightness up | Increase display brightness |
| `W` | View pan up | Pan view upward |
| `A` | View pan left | Pan view leftward |
| `S` | View pan down | Pan view downward |
| `D` | View pan right | Pan view rightward |
| `0` | View reset | Reset view to default position |
| `Q` | Background depth down | Decrease background perspective depth |
| `E` | Background depth up | Increase background perspective depth |
| `M` | Motion profile | Cycle motion profiles |
| `O` | Idle orbit | Toggle automatic idle orbit |
| `Y` | System panel | Cycle system panels (SYS → RND → SET) |
| `N` | Stopwatch / Timer | Switch between Stopwatch and Timer modes |
| `X` | Run / pause watch | Start/pause Stopwatch or Timer |
| `Z` | Reset watch | Reset Stopwatch or Timer to zero |
| `R` + `~` | Factory reset | Hold both keys to reset all settings |
| `~` | Restart / boot | Restart the device |

## QoL Version Only

| Key | Action | Notes |
|-----|--------|-------|
| `GO` / `G0` | DeepSleep | Enter ESP32 DeepSleep. Press GO to wake. |

## Notes

- **GO / G0 DeepSleep** is only available in the QoL version (`firmware/qol`).
- **Factory Reset** clears all persisted preferences and restarts the device.
- **AutoDim** activates after idle timeout (QoL version). Any key press restores brightness.
- **AutoSleep** activates after long idle (QoL version). Guarded when Stopwatch/Timer is running.
