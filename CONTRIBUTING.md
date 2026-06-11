# Contributing to DONUT.OS

Thank you for your interest in DONUT.OS!

## Welcome

- **Bug reports** — always welcome. Please include hardware version, firmware version, and steps to reproduce.
- **Hardware test reports** — if you test on Cardputer or Cardputer-ADV, your feedback is valuable.
- **Documentation improvements** — corrections, clarifications, better explanations.

## Guidelines

### What we welcome

- Bug fixes
- Performance improvements (without breaking the no-dynamic-memory constraint)
- Documentation improvements
- Hardware compatibility reports

### What requires discussion first

- Large feature pull requests — please open an issue first to discuss.
- New UI panels or system features.
- Changes to rendering pipeline or RGB+ implementation.

### What we will not accept

- Features that require `String`, `new/delete`, `malloc/free`, `std::vector`, `std::string`, or similar dynamic allocation in the firmware.
- WiFi, HTTP server, SD file system, LVGL, or other heavy additions.
- Breaking changes to RGB+, z-buffer, checker background, or Preferences V9.

## Code Style

- **No dynamic memory** in render hot path or main loop.
- **Keep UI clean** — minimal, focused, not cluttered.
- **Preserve the aesthetic** — DONUT.OS is intentionally minimal and polished.
- **Avoid `using namespace std`** — use explicit namespacing if needed.

## Getting Started

1. Fork the repository.
2. Create a feature branch.
3. Make your changes.
4. Test on actual hardware if possible.
5. Submit a pull request with a clear description.

## Questions?

Open an issue for questions, suggestions, or discussion.
