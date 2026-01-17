# Testing Capability

## ADDED Requirements

### Requirement: Automated Self-Test on Boot
The system SHALL run automated self-tests on boot and display PASS/FAIL results via serial console.

#### Scenario: Self-test runs automatically after firmware flash
- **WHEN** the ESP32 boots after firmware upload
- **THEN** the system SHALL execute all self-test functions for the current stage
- **AND** the system SHALL print "=== STAGE X SELF TEST ===" header to serial
- **AND** the system SHALL print PASS or FAIL for each test
- **AND** the system SHALL print overall result (ALL TESTS PASSED or SOME TESTS FAILED)

#### Scenario: Self-test detects I2C failure
- **WHEN** the I2C sensor is not connected or pull-up resistors are missing
- **THEN** testI2CCommunication() SHALL return FALSE
- **AND** the serial output SHALL display "I2C Communication: FAIL"
- **AND** the overall self-test SHALL report SOME TESTS FAILED

#### Scenario: Self-test validates heap memory
- **WHEN** self-test runs
- **THEN** testHeapMemory() SHALL check ESP.getFreeHeap() > 100KB
- **AND** the serial output SHALL display heap size in bytes
- **AND** the test SHALL return TRUE if heap is sufficient

### Requirement: Serial Command Interface
The system SHALL provide serial commands to manually trigger tests and access debugging functions.

#### Scenario: Running self-test via serial command
- **WHEN** the user types 't' in the serial monitor
- **THEN** the system SHALL execute runSelfTest() function
- **AND** all test results SHALL be printed to serial
- **AND** sensor operation SHALL continue normally after test completes

#### Scenario: Help menu via serial command
- **WHEN** the user types 'h' in the serial monitor
- **THEN** the system SHALL print a help menu listing all available commands
- **AND** the menu SHALL include: t (run test), h (help), r (restart), i (system info)

#### Scenario: System restart via serial command
- **WHEN** the user types 'r' in the serial monitor
- **THEN** the system SHALL call ESP.restart()
- **AND** the ESP32 SHALL reboot immediately

#### Scenario: System info via serial command
- **WHEN** the user types 'i' in the serial monitor
- **THEN** the system SHALL print IP address, free heap, WiFi status, and uptime
- **AND** the information SHALL be formatted in a readable manner

### Requirement: Web Test Endpoint (Stage 2+)
The system SHALL provide a /run-tests HTTP endpoint that returns test results as JSON.

#### Scenario: Accessing /run-tests endpoint
- **WHEN** an HTTP GET request is made to /run-tests
- **THEN** the system SHALL execute all test functions
- **AND** the system SHALL respond with HTTP 200 OK
- **AND** the response SHALL be valid JSON

#### Scenario: JSON test result format
- **WHEN** /run-tests returns results
- **THEN** the JSON SHALL include a boolean field for each test
- **AND** the format SHALL be: `{"i2c": true, "sensor": true, "wifi": true, "heap": true, "webserver": true}`
- **AND** TRUE indicates test passed, FALSE indicates failure

#### Scenario: Web tests run without blocking
- **WHEN** /run-tests endpoint is accessed
- **THEN** the tests SHALL complete within 5 seconds
- **AND** sensor operation SHALL not be interrupted
- **AND** the web server SHALL remain responsive

### Requirement: Test Function Naming Convention
All test functions SHALL follow a consistent naming pattern and return boolean results.

#### Scenario: Test function signature
- **WHEN** implementing a test function
- **THEN** the function name SHALL start with "test" followed by the component name
- **AND** the function SHALL return bool (true = PASS, false = FAIL)
- **AND** the function SHALL print its result to serial in format "  ComponentName: PASS/FAIL"

#### Scenario: Example test function structure
- **WHEN** creating testI2CCommunication()
- **THEN** the function SHALL perform I2C communication check
- **AND** the function SHALL print "  I2C Communication: PASS" or "FAIL"
- **AND** the function SHALL return true only if communication succeeded

### Requirement: Stage-Specific Test Coverage
Each stage SHALL implement cumulative tests covering all features from current and previous stages.

#### Scenario: Stage 1 tests
- **WHEN** Stage 1 self-test runs
- **THEN** the system SHALL test: I2C communication, sensor initialization, heap memory, sensor reading

#### Scenario: Stage 2 tests
- **WHEN** Stage 2 self-test runs
- **THEN** the system SHALL test: All Stage 1 tests, WiFi connection, AP fallback, web server, /status endpoint

#### Scenario: Stage 3 tests
- **WHEN** Stage 3 self-test runs
- **THEN** the system SHALL test: All Stage 2 tests, NVS write, NVS read, configuration persistence

#### Scenario: Stage 4 tests
- **WHEN** Stage 4 self-test runs
- **THEN** the system SHALL test: All Stage 3 tests, HTTP GET notification, HTTP POST notification, socket notification, state change detection

#### Scenario: Stage 5 tests
- **WHEN** Stage 5 self-test runs
- **THEN** the system SHALL test: All Stage 4 tests, debounce logic, presence timeout, parameter validation, distance filtering

#### Scenario: Stage 6 tests
- **WHEN** Stage 6 self-test runs
- **THEN** the system SHALL test: All Stage 5 tests, event logging, watchdog feeding, diagnostics endpoint, events endpoint, memory leak check

### Requirement: Test Result Documentation
Test results SHALL be clear, actionable, and include relevant diagnostic information.

#### Scenario: Failed test provides diagnostic info
- **WHEN** a test fails
- **THEN** the serial output SHALL indicate which test failed
- **AND** the output SHALL include relevant values (e.g., "Free Heap (85000 bytes): FAIL")
- **AND** the user SHALL be able to diagnose the issue from the output

#### Scenario: All tests pass confirmation
- **WHEN** all tests pass
- **THEN** the serial output SHALL print "Overall: ALL TESTS PASSED"
- **AND** the user can confidently proceed to the next stage

### Requirement: Non-Blocking Test Execution
Self-tests SHALL not significantly disrupt normal sensor operation.

#### Scenario: Tests complete quickly
- **WHEN** self-test runs
- **THEN** all tests SHALL complete within 10 seconds
- **AND** sensor polling SHALL resume immediately after

#### Scenario: Tests don't exhaust memory
- **WHEN** self-test runs
- **THEN** test functions SHALL not allocate significant heap memory
- **AND** heap usage SHALL return to normal after tests complete

### Requirement: Network-Dependent Test Handling
Tests requiring network connectivity SHALL gracefully handle unavailable endpoints.

#### Scenario: Notification tests without configured endpoint
- **WHEN** Stage 4+ self-test runs
- **AND** notification endpoint is not configured or unreachable
- **THEN** notification tests MAY report FAIL
- **AND** the system SHALL document that network tests require valid endpoint configuration
- **AND** other tests SHALL continue to execute normally

#### Scenario: WiFi test in AP mode
- **WHEN** self-test runs in AP fallback mode
- **THEN** testWiFiConnection() SHALL report the current mode (AP vs STA)
- **AND** AP mode SHALL be considered valid (not a failure)
- **AND** the test SHALL verify AP IP is 192.168.4.1

### Requirement: Test Dashboard Integration (Stage 6)
The web dashboard SHALL display a test results summary.

#### Scenario: Test results visible in web UI
- **WHEN** the user accesses the web dashboard in Stage 6
- **THEN** the dashboard SHALL display total tests count
- **AND** the dashboard SHALL display PASS count
- **AND** the dashboard SHALL display FAIL count
- **AND** the dashboard SHALL provide a "Run Tests" button

#### Scenario: Running tests from web UI
- **WHEN** the user clicks "Run Tests" in the dashboard
- **THEN** the system SHALL call /run-tests endpoint
- **AND** the results SHALL update in the dashboard without page reload
- **AND** each test result SHALL be displayed with PASS/FAIL indicator
