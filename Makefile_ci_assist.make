# This is included in every build folder for convience in downloading and using CI builds
TARGET = maketest
BUILD_DIR = .

all:
	@echo "make swd :- load release binary via swd programmer"
	@echo "make swd-attach :- attach to running device "
	@echo "make swd-console :- access semihost console (if avaliable)"
	@echo "make swd-gdbgui :- launch gdbgui and load program"

#######################################
# SWD
#######################################
BIN_IMAGE = $(BUILD_DIR)/$(TARGET).elf

# make swd (OpenOCD CLI Telnet server on TCP port 4444)
swd: all
	arm-none-eabi-gdb -x gdbinit_stm32h7xx $(BIN_IMAGE)
	@echo "--------------- openocd.log (First 20 lines) ---------------------"
	@head -20 /tmp/openocd.log

# make swd-attach
swd-attach: all
	arm-none-eabi-gdb -x boards/gdbinit_stm32h7xx_attach $(BIN_IMAGE)
	@echo "--------------- openocd.log (First 20 lines) ---------------------"
	@head -20 /tmp/openocd.log

# make swd-console
swd-console: all
	@echo "--------------- WARN: Must have semihost enabled FW ------------------"
	/usr/local/bin/openocd                                                       \
		-s /usr/local/share/openocd/scripts                                        \
		-f /usr/local/share/openocd/scripts/interface/stlink.cfg                   \
		-f /usr/local/share/openocd/scripts/target/stm32h7x_dual_bank.cfg          \
		-c "init; arm semihosting enable; log_output /tmp/openocd.log; reset run;"
	@echo "--------------- openocd.log (First 20 lines) ---------------------"
	@head -20 /tmp/openocd.log

# make swd-load
swd-load: all
	/usr/local/bin/openocd                                                       \
		-f /usr/local/share/openocd/scripts/interface/stlink.cfg                   \
		-f /usr/local/share/openocd/scripts/target/stm32h7x_dual_bank.cfg          \
		-c "log_output /tmp/openocd.log"                                           \
		-c init                                                                    \
		-c "reset halt"                                                            \
		-c "flash write_image erase ${BIN_IMAGE}"                                  \
		-c "verify_image ${BIN_IMAGE}"                                             \
		-c reset                                                                   \
		-c shutdown
	@echo "--------------- openocd.log (First 20 lines) ---------------------"
	@head -20 /tmp/openocd.log

# make swd-gdbgui
swd-gdbgui: all
	gdbgui --gdb-args="-x gdbinit_stm32h7xx" $(BIN_IMAGE)
	@echo "--------------- openocd.log (First 20 lines) ---------------------"
	@head -20 /tmp/openocd.log

# make swd-gdbgui-attached
swd-gdbgui-attached: all
	gdbgui --gdb-args="-x gdbinit_stm32h7xx_attach" $(BIN_IMAGE)
	@echo "--------------- openocd.log (First 20 lines) ---------------------"
	@head -20 /tmp/openocd.log
