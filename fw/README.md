Building the firmware
---------------------

* Install Zephyr 2.4 in ~/src/zephyrproject/zephyr (or set ZEPHYR_BASE)
* Install the Zephyr SDK in ~/zephyr-sdk (or set ZEPHYR_SDK_INSTALL_DIR)
* Build with `make`
* Use `make flash` to load with SWD (ST-LINK v2/v3) or `make dfu` to load with USB DFU.
