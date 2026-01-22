# Home Assistant Setup Guide

This guide walks you through setting up the ESP32 Microwave Motion Sensor with Home Assistant using the built-in Mosquitto MQTT broker add-on.

## Prerequisites

- Home Assistant installed and running (Home Assistant OS, Supervised, or Container)
- ESP32 motion sensor flashed with Stage 9 firmware
- Both devices on the same network

## Part 1: Install Mosquitto Broker Add-on

### Step 1: Open Add-on Store

1. In Home Assistant, click **Settings** in the sidebar
2. Click **Add-ons**
3. Click the **Add-on Store** button (bottom right)

### Step 2: Install Mosquitto

1. Search for "Mosquitto" in the search box
2. Click on **Mosquitto broker**
3. Click **Install**
4. Wait for installation to complete (may take 1-2 minutes)

### Step 3: Configure Mosquitto

1. After installation, stay on the Mosquitto add-on page
2. Click the **Configuration** tab
3. You have two options for authentication:

**Option A: Use Home Assistant Users (Recommended)**

Leave the default configuration. The add-on will automatically allow any Home Assistant user to connect.

**Option B: Create Dedicated MQTT Users**

Add custom logins in the configuration:

```yaml
logins:
  - username: mqtt_sensor
    password: your_secure_password_here
```

4. Click **Save**

### Step 4: Start Mosquitto

1. Go back to the **Info** tab
2. Toggle **Start on boot** to ON
3. Toggle **Watchdog** to ON (auto-restart if crashed)
4. Click **Start**
5. Wait for the add-on to start (status shows "Started")

### Step 5: Check Mosquitto Logs

1. Click the **Log** tab
2. Verify you see messages like:
   ```
   Starting Mosquitto message broker...
   mosquitto version X.X.X running
   Opening ipv4 listen socket on port 1883
   ```

## Part 2: Add MQTT Integration to Home Assistant

### Step 1: Add the Integration

1. Go to **Settings** → **Devices & Services**
2. Click **+ Add Integration** (bottom right)
3. Search for "MQTT"
4. Click **MQTT**

### Step 2: Configure MQTT Connection

If you used Home Assistant users for authentication:

| Field | Value |
|-------|-------|
| Broker | `core-mosquitto` (or `localhost`) |
| Port | `1883` |
| Username | Your Home Assistant username |
| Password | Your Home Assistant password |

If you created dedicated MQTT users:

| Field | Value |
|-------|-------|
| Broker | `core-mosquitto` |
| Port | `1883` |
| Username | `mqtt_sensor` |
| Password | The password you configured |

5. Click **Submit**
6. You should see "Success!" - click **Finish**

## Part 3: Configure the Motion Sensor

### Step 1: Access the Sensor's Web Interface

1. Find the sensor's IP address:
   - Check your router's DHCP client list, OR
   - Look at the serial monitor output during boot
2. Open a web browser and navigate to `http://<sensor-ip>/`

### Step 2: Go to Settings

1. Click **Settings** in the navigation bar

### Step 3: Configure MQTT Settings

Scroll down to the **MQTT / Home Assistant** section and fill in:

| Field | Value |
|-------|-------|
| Enable MQTT | Check the box |
| Broker Host | Your Home Assistant's IP address (e.g., `192.168.1.100`) |
| Port | `1883` |
| Username | Same username from Part 2 |
| Password | Same password from Part 2 |
| Use TLS | Leave unchecked (for local network) |
| Device Name | Custom name (e.g., "Living Room Motion") or leave default |

### Step 4: Save Configuration

1. Click **Save** at the bottom of the page
2. The sensor will attempt to connect to the MQTT broker
3. The status indicator should change to **Connected** (green)

### Step 5: Verify Connection in Sensor Logs

If you have serial monitor connected:
```
[MQTT] Connecting to 192.168.1.100:1883...
[MQTT] Connected!
[MQTT] Publishing discovery...
[MQTT] Publishing availability: online
[LOG] MQTT_CONNECTED
```

## Part 4: Verify Device in Home Assistant

### Step 1: Check MQTT Integration

1. Go to **Settings** → **Devices & Services**
2. Click on **MQTT**
3. Click on **devices**
4. You should see your motion sensor listed (e.g., "Microwave Motion Sensor" or your custom name)

### Step 2: View Device Entities

1. Click on your motion sensor device
2. You should see these entities:

**Controls:**
- Clear Timeout (number)
- Filter Threshold (number)
- Trip Delay (number)

**Sensors:**
- Alarm Count
- Filter Level
- Free Heap
- IP Address
- Motion (binary sensor)
- Motion Count
- Uptime
- WiFi Signal

### Step 3: Add Entities to Dashboard

1. Go to your Home Assistant dashboard
2. Click the three dots menu → **Edit Dashboard**
3. Click **+ Add Card**
4. Search for your motion sensor entities
5. Add cards for:
   - **Entity** card for Motion binary sensor
   - **Entities** card for diagnostic sensors
   - **Number** cards for controls (optional)

## Part 5: Testing

### Test 1: Motion Detection

1. Open Home Assistant dashboard with the Motion entity visible
2. Trigger motion in front of the sensor
3. The Motion entity should change from `off` to `on`
4. After the clear timeout, it should change back to `off`

**Expected behavior:**
- State changes should appear within 1 second
- The "Alarm Count" sensor should increment

### Test 2: Remote Control

1. In Home Assistant, find the "Trip Delay" number entity
2. Change the value (e.g., from 3 to 5)
3. Open the sensor's web interface **Settings** page
4. Verify the Trip Delay value has changed to match

### Test 3: Diagnostic Sensors

1. Check that diagnostic sensors are updating:
   - **Uptime** should increase over time
   - **WiFi Signal** should show a reasonable dBm value (typically -30 to -80)
   - **Free Heap** should show available memory (typically 200,000+ bytes)
2. Diagnostics update every 60 seconds

### Test 4: Availability

1. In Home Assistant, note the motion sensor is showing as "Available"
2. Power off the ESP32 sensor
3. Wait 60-90 seconds (MQTT keepalive + broker timeout)
4. The sensor should show as "Unavailable" in Home Assistant
5. Power the sensor back on
6. It should reconnect and show as "Available" again

### Test 5: Home Assistant Restart

1. Restart Home Assistant (Settings → System → Restart)
2. After restart, verify the motion sensor is still visible
3. The sensor should automatically re-publish discovery messages

## Troubleshooting

### Sensor not appearing in Home Assistant

**Check MQTT broker is running:**
1. Go to Settings → Add-ons → Mosquitto broker
2. Verify status shows "Started"
3. Check logs for errors

**Check sensor connection:**
1. Open sensor's Diagnostics page (`http://<sensor-ip>/diagnostics`)
2. Look for MQTT connection status
3. Check Serial monitor for connection errors

**Verify credentials:**
1. Ensure username/password match exactly
2. Try connecting with an MQTT client tool (like MQTT Explorer) using same credentials

### Motion not updating

**Check sensor state:**
1. Open sensor's web interface main page
2. Verify motion is being detected (state changes visible)
3. Check that MQTT is enabled and connected

**Check MQTT messages:**
Use MQTT Explorer or Home Assistant's MQTT tool:
1. Go to Settings → Devices & Services → MQTT → Configure
2. Subscribe to `mw_motion_#` (all topics for your sensor)
3. Trigger motion and watch for messages

### "Unavailable" shown but sensor is running

**Check network connectivity:**
1. Can you access the sensor's web interface?
2. Can you ping the MQTT broker from another device?

**Check keepalive:**
- The sensor sends a keepalive every 60 seconds
- If WiFi is unstable, connection may drop
- Check WiFi signal strength on Diagnostics page

### Number controls not working

**Verify subscription:**
1. In MQTT Explorer, check that the sensor is subscribed to `/set` topics
2. Publish a test value manually to the set topic:
   - Topic: `mw_motion_<chip_id>/trip_delay/set`
   - Payload: `5`

## Example Automations

### Turn on lights when motion detected

```yaml
automation:
  - alias: "Motion - Turn On Living Room Lights"
    trigger:
      - platform: state
        entity_id: binary_sensor.microwave_motion_sensor_motion
        to: "on"
    action:
      - service: light.turn_on
        target:
          entity_id: light.living_room
```

### Turn off lights after no motion

```yaml
automation:
  - alias: "No Motion - Turn Off Living Room Lights"
    trigger:
      - platform: state
        entity_id: binary_sensor.microwave_motion_sensor_motion
        to: "off"
        for:
          minutes: 5
    action:
      - service: light.turn_off
        target:
          entity_id: light.living_room
```

### Send notification at night

```yaml
automation:
  - alias: "Night Motion Alert"
    trigger:
      - platform: state
        entity_id: binary_sensor.microwave_motion_sensor_motion
        to: "on"
    condition:
      - condition: time
        after: "23:00:00"
        before: "06:00:00"
    action:
      - service: notify.mobile_app_your_phone
        data:
          title: "Motion Detected"
          message: "Motion detected in {{ trigger.to_state.attributes.friendly_name }}"
```

### Monitor sensor health

```yaml
automation:
  - alias: "Alert on Low WiFi Signal"
    trigger:
      - platform: numeric_state
        entity_id: sensor.microwave_motion_sensor_wifi_signal
        below: -80
    action:
      - service: notify.mobile_app_your_phone
        data:
          title: "Sensor Warning"
          message: "Motion sensor WiFi signal is weak: {{ states('sensor.microwave_motion_sensor_wifi_signal') }} dBm"
```

## Finding Your Sensor's Chip ID

The chip ID is used in MQTT topics and entity IDs. To find it:

1. **From Serial Monitor:** Look for the line during boot:
   ```
   [MQTT] Device ID: mw_motion_abc123
   ```

2. **From MQTT Topics:** Use MQTT Explorer to see topics like:
   ```
   homeassistant/binary_sensor/mw_motion_abc123/motion/config
   ```

3. **From Entity ID:** In Home Assistant, the entity ID includes the chip ID:
   ```
   binary_sensor.mw_motion_abc123_motion
   ```

## Next Steps

- Create automations based on motion detection
- Add the sensor to Home Assistant areas for better organization
- Set up multiple sensors and distinguish them by device name
- Use the number controls to fine-tune sensitivity remotely
