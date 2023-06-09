### BLE Client Application

![Code:Incomplete](https://img.shields.io/badge/Code-Incomplete-red)

This reopistory contains the source code of application written in C programming language to create a console based BLE Client using BlueZ library via gdbus API calls. This repository is also a sort of playground where I may push updates as I find some new things to test with BLE and Linux Platform for various circumstances.

This application sets the BLE Discovery filter to ESP32 SSP Server Profile and the project template code can be found here - [Repository: esp32_wifi_ble_coexist_template](https://github.com/pixma/esp32_wifi_ble_coexist_template).

#### Folder Structure
```
├── build
│   ├── CMakeCache.txt
│   ├── CMakeFiles
|
├── CMakeLists.txt
├── include
│   ├── ble_app_client.h        # Generated from gdbus-codegen
│   ├── loguru.hpp
│   └── main.h
├── introspect.xml              # Supplied to gdbus-codegen to generate c and h files "ble_app_client.c/h"
├── output
├── readme.md
└── src
    ├── ble_app_client.c        # Generated from gdbus-codegen
    ├── loguru.cpp
    └── main.cpp
```

#### Libraries Required or Used

```
~/...>objdump -p ble_test_codegen | grep 'NEEDED' 
  NEEDED               libglib-2.0.so.0
  NEEDED               libgio-2.0.so.0
  NEEDED               libgobject-2.0.so.0
  NEEDED               libstdc++.so.6
  NEEDED               libgcc_s.so.1
  NEEDED               libc.so.6

```

#### gdbus-codegen
```
~/>gdbus-codegen --generate-c-code ble_app_client --output-dir=. introspect.xml
```
The code so generated by gdbus-codegen, in that, Only Client related API calls are used in order to make this BLE Client Application which gives some comfort to interact with DBUS.

#### TODO:
- AcquireWrite 
- Open and Hold the fd.(UnixFDList)
- Write to the fd.
- Close the fd.
- On init execution, it should first check if there is any trusted device exist or not which matches UUID and Device Name, then go to discovery/scan.