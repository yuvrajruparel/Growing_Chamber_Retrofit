# Connecting to Wifi

Flash the ESP32 using `idf.py build flash monitor`

In the serial monitor, look for the line:
```
I (749) wifi:mode : sta (4c:c3:82:c3:4e:1c)
```

Copy the text in parenthesis (the mac address) and register it with DukeReg to allow access to the wifi.
