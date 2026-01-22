# Contributing to ESP32 Microwave Motion Sensor

Thank you for your interest in contributing! This document provides guidelines for contributing to the project.

## Code of Conduct

By participating in this project, you agree to abide by our [Code of Conduct](CODE_OF_CONDUCT.md).

## How to Contribute

### Reporting Bugs

1. **Check existing issues** - Search the [issue tracker](https://github.com/miswired/esp32-radar/issues) to see if the bug has already been reported.

2. **Create a new issue** with the following information:
   - Clear, descriptive title
   - Steps to reproduce the bug
   - Expected behavior
   - Actual behavior
   - Firmware version (stage number)
   - Hardware setup (ESP32 variant, sensor model)
   - Serial monitor output (if applicable)

### Suggesting Features

1. **Check existing issues** - Your idea may already be proposed or in development.

2. **Create a feature request** with:
   - Clear description of the feature
   - Use case / problem it solves
   - Proposed implementation (if you have ideas)

### Submitting Code

1. **Fork the repository** to your GitHub account.

2. **Create a feature branch** from `main`:
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make your changes** following the code style guidelines below.

4. **Test your changes**:
   - Compile successfully with `arduino-cli compile --fqbn esp32:esp32:esp32 firmware`
   - Test on actual hardware if possible
   - Verify web interface functions correctly

5. **Commit your changes** with clear, descriptive messages:
   ```bash
   git commit -m "feat: Add support for multiple sensors"
   ```

6. **Push to your fork** and create a Pull Request.

## Code Style Guidelines

### Arduino/C++ Code

- Use consistent indentation (2 spaces)
- Use descriptive variable and function names
- Add comments for complex logic
- Keep functions focused and reasonably sized
- Follow existing patterns in the codebase

### HTML/CSS/JavaScript (Web Interface)

- Inline styles and scripts are acceptable (reduces HTTP requests on ESP32)
- Use semantic HTML elements
- Maintain consistent color scheme
- Test on mobile viewports

### Commit Messages

Follow conventional commit format:
- `feat:` New features
- `fix:` Bug fixes
- `docs:` Documentation changes
- `refactor:` Code refactoring
- `test:` Test additions/changes
- `chore:` Build process, dependencies

Examples:
```
feat: Add MQTT auto-reconnect with exponential backoff
fix: Resolve memory leak in diagnostic logging
docs: Update Home Assistant setup guide
```

## Development Setup

### Prerequisites

- Arduino CLI or Arduino IDE
- ESP32 board support package
- Required libraries:
  - ArduinoJson
  - PubSubClient

### Building

```bash
# Install libraries
arduino-cli lib install ArduinoJson PubSubClient

# Compile
arduino-cli compile --fqbn esp32:esp32:esp32 firmware

# Upload
arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 firmware
```

### Testing

1. **Compile test**: Ensure code compiles without errors
2. **Hardware test**: Deploy to ESP32 and verify functionality
3. **Web interface test**: Test all pages and API endpoints
4. **MQTT test**: Verify Home Assistant integration (if applicable)

## Project Structure

```
esp32-radar/
├── firmware/               # Latest firmware (compile this!)
│   ├── firmware.ino              # Main source file
│   ├── credentials.h.example     # WiFi credentials template
│   ├── README.md                 # Firmware documentation
│   └── HOME_ASSISTANT_SETUP.md   # HA setup guide
├── libs/                   # Archived Arduino libraries
├── docs/                   # Additional documentation
├── archive/stages/         # Historical development stages (1-9)
├── openspec/               # OpenSpec proposals and specifications
├── LICENSE                 # GPL v3 license
├── CONTRIBUTING.md         # This file
├── CODE_OF_CONDUCT.md      # Community standards
└── CHANGELOG.md            # Version history
```

## OpenSpec Workflow

This project uses OpenSpec for managing feature specifications:

1. **Proposals** are created in `openspec/changes/`
2. **Specifications** live in `openspec/specs/`
3. Use `openspec validate` to check proposals
4. See `openspec/AGENTS.md` for detailed instructions

## Questions?

- Open an issue for questions about contributing
- Check the [wiki](https://github.com/miswired/esp32-radar/wiki) for documentation
- Review existing issues and pull requests for context

## License

By contributing, you agree that your contributions will be licensed under the GPL v3 license.
