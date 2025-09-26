# ESP32

## idf.py menuconfig

`Component config --->` \
`    [*] Bluetooth` \
`        Host (NimBLE - BLE only)` \
`        Controller (Enabled)` \

Artisanのusb接続で使う場合以下も設定する必要あり

`Component config --->` \
`    ESP System Setting` \
`        Channel for console output (USB Serial/JTAG Controller)` \
`        Channel for console secondary output (No secondary console)` \