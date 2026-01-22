# Design: Comprehensive Documentation

## Context

The ESP32 Microwave Motion Sensor project is being released as open source under GPL v3. Documentation must serve multiple audiences: complete beginners, electronics hobbyists, and developers who want to contribute. The Diataxis framework provides a proven structure for organizing technical documentation.

### Stakeholders
- Complete beginners who need hardware assembly guidance
- Hobbyists with Arduino experience who want to build and customize
- Developers who want to contribute code or fork the project
- Users integrating with Home Assistant, WLED, or other systems

### Constraints
- GitHub Wiki for hosting (easy community contributions)
- Must include proper GPL v3 attribution throughout
- Documentation should match the current Stage 9 implementation
- Wiring diagrams should be simple and reproducible

## Goals / Non-Goals

### Goals
- Provide complete beginner-to-expert documentation path
- Enable community contributions with clear guidelines
- Document all features across all stages
- Explain design decisions and architecture
- Include visual aids (diagrams, screenshots)

### Non-Goals
- Video tutorials (text/image documentation only)
- Translated documentation (English only initially)
- Automated documentation generation from code
- Hosting documentation outside GitHub

## Decisions

### Decision: Diataxis Framework Structure
**Choice:** Organize all documentation into four distinct sections per Diataxis.

**Rationale:**
- Proven framework used by major projects (Django, NumPy)
- Clear separation of concerns prevents documentation bloat
- Users can find what they need based on their current goal
- Easier to maintain when each doc type has clear purpose

**Structure:**
```
Wiki/
├── Home                          # Landing page with navigation
├── Tutorials/
│   ├── Getting-Started           # First-time setup, full walkthrough
│   └── Your-First-Detection      # Build and test motion detection
├── How-To-Guides/
│   ├── Home-Assistant-Setup      # Complete HA integration
│   ├── WLED-Integration          # Visual alarm feedback
│   ├── Webhook-Notifications     # HTTP callbacks
│   ├── API-Usage                 # Programmatic control
│   ├── Tuning-Sensitivity        # Optimize for your environment
│   └── Updating-Firmware         # Flash new versions
├── Reference/
│   ├── API-Endpoints             # Complete REST API
│   ├── Configuration-Options     # All settings explained
│   ├── MQTT-Topics               # Home Assistant MQTT reference
│   ├── Hardware-Specifications   # Pin mappings, power requirements
│   ├── State-Machine             # Motion detection states
│   └── Event-Types               # All logged events
└── Explanation/
    ├── How-Motion-Detection-Works   # Radar principles
    ├── Adaptive-Filter-Design       # Why filtering is needed
    ├── Security-Model               # Authentication design
    ├── Architecture-Overview        # Code structure
    └── Stage-Evolution              # Why features were added progressively
```

### Decision: GPL v3 License
**Choice:** GNU General Public License v3.0

**Rationale:**
- User requirement: derivatives must remain open source (copyleft)
- User requirement: attribution required
- Compatible with all project dependencies:
  - ESP32 Arduino Core: LGPL 2.1 (compatible)
  - ArduinoJson: MIT (compatible)
  - PubSubClient: MIT (compatible)
  - mbedtls: Apache 2.0 (compatible)

**Implementation:**
- LICENSE file in repository root
- License header in main source files
- About page in web interface with license info
- Copyright notice: "Copyright 2024-2026 Miswired"

### Decision: About Page for License Display
**Choice:** Dedicated About/Info page in web interface

**Rationale:**
- User preference over footer display
- Allows for full license text and project information
- Cleaner UI than persistent footer
- Accessible from navigation

### Decision: Contributor Documentation
**Choice:** Include CONTRIBUTING.md, CODE_OF_CONDUCT.md, CHANGELOG.md

**Rationale:**
- Standard open source project files
- CONTRIBUTING.md lowers barrier for new contributors
- CODE_OF_CONDUCT.md sets community expectations
- CHANGELOG.md helps users understand version differences

### Decision: Hardware Documentation
**Choice:** Include Bill of Materials and wiring diagrams

**Rationale:**
- Complete beginners need parts lists with purchase links
- Visual wiring diagrams prevent assembly errors
- ASCII art diagrams for maximum compatibility
- Optional: Fritzing diagrams for those who prefer them

## Documentation Content Plan

### Tutorials (Learning-Oriented)

**1. Getting Started**
- Parts list with links
- Tools needed
- Step-by-step assembly with photos/diagrams
- Initial firmware flash
- First boot and WiFi setup
- Verifying motion detection works

**2. Your First Detection**
- Understanding the web interface
- Triggering motion and observing response
- Reading the diagnostics page
- Basic tuning adjustments

### How-To Guides (Goal-Oriented)

**1. Home Assistant Setup**
- Already exists: HOME_ASSISTANT_SETUP.md
- Move to wiki with enhancements

**2. WLED Integration**
- Configure WLED device
- Set up sensor to trigger WLED
- Custom JSON payloads

**3. Webhook Notifications**
- Configure notification URL
- GET vs POST methods
- Testing notifications
- Example webhook receivers

**4. API Usage**
- Authentication setup
- Reading status programmatically
- Changing settings via API
- Integration examples (Python, Node.js)

**5. Tuning Sensitivity**
- Understanding trip delay
- Adjusting clear timeout
- Filter threshold explained
- Environment-specific recommendations

**6. Updating Firmware**
- Download latest release
- Flash with arduino-cli
- Preserve settings during update

### Reference (Information-Oriented)

**1. API Endpoints**
- Complete REST API documentation
- Request/response examples
- Error codes

**2. Configuration Options**
- All settings with types, defaults, ranges
- NVS storage details

**3. MQTT Topics**
- Discovery topics
- State topics
- Command topics
- Payload formats

**4. Hardware Specifications**
- ESP32 pin mappings
- RCWL-0516 specifications
- Power requirements
- Enclosure considerations

**5. State Machine**
- State diagram
- State transitions
- Timing parameters

**6. Event Types**
- All event types
- When each occurs
- Data payloads

### Explanation (Understanding-Oriented)

**1. How Motion Detection Works**
- Doppler radar principles
- RCWL-0516 operation
- Digital output interpretation

**2. Adaptive Filter Design**
- Why filtering is needed
- Moving average implementation
- Threshold behavior
- Trade-offs

**3. Security Model**
- Session-based authentication
- API key authentication
- Password hashing
- Threat model

**4. Architecture Overview**
- Code organization
- Major components
- Data flow

**5. Stage Evolution**
- Why progressive stages
- What each stage added
- Design decisions at each stage

## Risks / Trade-offs

### Risk: Documentation becomes outdated
**Mitigation:**
- Include "Last updated" dates
- Link documentation updates to code changes
- Mark version-specific information clearly

### Risk: Wiki contributions without review
**Mitigation:**
- Main documentation maintained by project owner
- Community contributions via issues/PRs
- Clear contribution guidelines

### Risk: Overwhelming beginners
**Mitigation:**
- Clear "Start Here" guidance on home page
- Progressive complexity in tutorials
- Separate reference from tutorials

## Open Questions

1. Should screenshots be included in wiki, or is text sufficient?
   - Recommendation: Include key screenshots for web interface pages

2. Should we include troubleshooting sections in each how-to, or a separate troubleshooting page?
   - Recommendation: Both - inline for common issues, separate page for comprehensive troubleshooting
