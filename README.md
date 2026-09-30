# Word Clock (ESPHome)

ESPHome firmware for the [GurgleApps WiFi colour word clock kit](https://gurgleapps.com/reviews/electronics/wifi-controlled-color-word-clock-kit-micropython)
(Raspberry Pi Pico 2 W driving an 8×8 WS2812B matrix), replacing the stock
MicroPython firmware so the clock is a native Home Assistant device.

## What Home Assistant gets

| Entity | Notes |
| --- | --- |
| `light.word_clock_display` | Real on/off (all LEDs dark), brightness, colour. Click the name (not the toggle) in HA for the brightness and colour controls. |
| Clock Style (select) | **Word Clock** (the display's colour), **Colour per Word**, **Rainbow**, **Rainbow Cycle**. Same as the display's effect, but it can be changed while the display is off. |
| Hour / Minute / Past & To Colour | Colour pickers for **Colour per Word** (shown as lights under Configuration). Their brightness scales that word group; turning one off hides it. |
| Diagnostics | IP address, WiFi SSID, MAC, WiFi signal, uptime, internal temperature, ESPHome version, UTC offset |
| Buttons | Restart, Safe Mode Boot |

Time and time zone come from Home Assistant, so daylight saving follows your
HA settings; SNTP keeps the clock right if HA is unavailable. Until it has the
time, the clock shows a WiFi symbol.

Brightness uses the stock firmware's curve (gamma 2.2), so HA brightness
(n + 1) / 16 looks like stock level n: 19% is the stock default (level 2),
and the display stays visible down to about 8%.

Settings (on/off, brightness, colours, style) are saved to flash at most once
a minute, so a change made just before a power cut can be lost. On the Pico,
ESPHome stores saved settings by position, so a firmware update that adds or
reorders saved entities resets them once.

Turning the light off stops its effect (standard ESPHome behaviour); the
firmware puts the last-used clock effect back as soon as the light is on again,
so an automation can simply turn it on/off or set a brightness. Choosing
"None" as the effect therefore just returns to the clock.

`default_transition_length` is 0s. ESPHome stops the effect before a fade,
so a fade-out would briefly light the whole panel; avoid passing
`transition:` in automations.

### Example: follow room occupancy

```yaml
automation:
  - alias: Word clock follows occupancy
    triggers:
      - trigger: state
        entity_id: binary_sensor.study_occupancy
    actions:
      - action: "light.turn_{{ 'on' if trigger.to_state.state == 'on' else 'off' }}"
        target:
          entity_id: light.word_clock_display
```

## Files

- `wordclock.yaml` — the ESPHome device config. Tweak `substitutions` for the
  LED pin (27 on V2 kits and later, 2 on V1).
- `word_colour.yaml` — package for one colour picker, used three times.
- `wordclock.h` — the clock face: word masks and time → words logic, plain C++.
- `test/run.sh` — builds `wordclock.h` on your computer and checks it against
  the original MicroPython firmware's own code for all 1440 minutes of the day
  (and the rainbow palette). Needs git, python3 and a C++ compiler.

## Installing

1. **Back up the stock firmware's settings** (optional; it holds your WiFi
   details and colours). With the clock plugged in over USB:
   ```sh
   pip install mpremote
   mkdir -p backup && mpremote cp :config.json :scenes.json :schedules.json backup/
   ```
2. **Secrets**: copy `secrets.yaml.example` to `secrets.yaml` and fill it in.
   If you use the ESPHome Builder add-on, copy `wordclock.yaml` and
   `wordclock.h` into its config folder and add the keys to its shared
   `secrets.yaml` instead.
3. **First flash (USB)**: hold the Pico's BOOTSEL button while plugging in USB,
   then run `esphome run wordclock.yaml` (it finds the `RP2350` drive), or
   build with `esphome compile wordclock.yaml` and copy
   `.esphome/build/wordclock/.pioenvs/wordclock/firmware.uf2` onto the drive.
4. Home Assistant discovers the device; add it with the API encryption key.
   Later updates go over the air.

### Going back to the stock firmware

Hold BOOTSEL, copy the MicroPython UF2 for the Pico 2 W onto the drive, then
copy the upstream `src/` files (and your `backup/` files) back with Thonny or
`mpremote`, as in the upstream README.

## Credits

The word masks and time → words logic are ported from
[gurgleapps/Gurgle-Apps-Word-Clock](https://github.com/gurgleapps/Gurgle-Apps-Word-Clock).
