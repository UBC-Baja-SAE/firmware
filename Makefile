CC          = arm-none-eabi-gcc
OBJCOPY     = arm-none-eabi-objcopy

CFLAGS      = -mcpu=cortex-m3 -mthumb -O2 -Wall
LDFLAGS     = -Tbluepill.ld --specs=nosys.specs

# ECU selection
ifeq ($(ECU),FR)
    ECU_DEFINE = -D ECU_FR
    TARGET     = bluepill_fr
else ifeq ($(ECU),FL)
    ECU_DEFINE = -D ECU_FL
    TARGET     = bluepill_fl
else ifeq ($(ECU),RR)
    ECU_DEFINE = -D ECU_RR
    TARGET     = bluepill_rr
else ifeq ($(ECU),RL)
    ECU_DEFINE = -D ECU_RL
    TARGET     = bluepill_rl
else ifeq ($(ECU),Rear)
    ECU_DEFINE = -D ECU_REAR
    TARGET     = bluepill_rear
else
    $(error Please specify ECU=FR|FL|RR|RL|Rear)
endif

# Directories
SRC_DIR   = src
BUILD_DIR = build

# Find all source files in src
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Default target: produce a flashable binary (.bin)
all: $(BUILD_DIR)/$(TARGET).bin

# Compile each .c file into an object file
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(ECU_DEFINE) -c $< -o $@

# Link object files to produce an ELF
$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $(ECU_DEFINE) $(OBJS) $(LDFLAGS) -o $@

# Convert ELF to binary for flashing
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Flash target (adjust command to your programmer, here using st-flash)
flash: $(BUILD_DIR)/$(TARGET).bin
	st-flash write $(BUILD_DIR)/$(TARGET).bin 0x8000000

# Clean up build directory
clean:
	rm -rf $(BUILD_DIR)
