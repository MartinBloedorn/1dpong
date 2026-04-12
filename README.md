## To-Do

- [ ] Add ifdefs to turn ESP into an access point, on which the configuration server can be read (?)... Or have an "artgenossenschaft" WiFi anyway? There'll be a bunch of other OSC thingies anyways...
    - [ ] Server can optionally automatically shut down after 5min to save resources.
    - [ ] Change this device's name (currently  esp32s3-94E1CC)
- [ ] Saving parameters with EEPROM: https://www.luisllamas.es/en/esp32-eeprom/


## Ideas

- Prevent paddle from being spammed: add some sort of minimum dead-time between presses/hits (e.g. 250ms or so)