export ZEPHYR_BASE ?= $(HOME)/src/zephyrproject/zephyr
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR ?= $(HOME)/zephyr-sdk

BUILD=build
BUILD_NATIVE=build_native
TARGET_BOARD ?= usb_led_matrix

.PHONY: all clean FORCE flash

all: $(BUILD_NATIVE)/zephyr/zephyr.elf $(BUILD)/zephyr/zephyr.elf

$(BUILD_NATIVE)/zephyr/zephyr.elf: FORCE
	west build -b native_posix -d $(BUILD_NATIVE)

$(BUILD)/zephyr/zephyr.elf: FORCE
	west build -d $(BUILD) -b $(TARGET_BOARD)

flash: $(BUILD)/zephyr/zephyr.elf
	west flash -d $(BUILD) -r openocd

dfu: $(BUILD)/zephyr/zephyr.elf
	dfu-util -a 0 -D $(BUILD)/zephyr/zephyr.bin --dfuse-address 0x08000000

debug: $(BUILD)/zephyr/zephyr.elf
	west debug -d $(BUILD) -r openocd

clean:
	rm -rf $(BUILD) $(BUILD_NATIVE)
