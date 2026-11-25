DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
CC := $(DEVKITARM)/bin/arm-none-eabi-gcc
TARGET := winterstad
ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := -g -Wall -O2 -mword-relocations -ffunction-sections -fdata-sections $(ARCH) -D__3DS__ -I$(DEVKITPRO)/libctru/include -I$(DEVKITPRO)/portlibs/3ds/include
LDFLAGS := -specs=3dsx.specs -g $(ARCH) -L$(DEVKITPRO)/libctru/lib -L$(DEVKITPRO)/portlibs/3ds/lib
LIBS := -lcitro3d -lctru -lm
PICASSO ?= $(DEVKITPRO)/tools/bin/picasso

all: $(TARGET).3dsx

shader.shbin: source/shader.v.pica
	$(PICASSO) -o $@ $<

shader.o: shader.shbin
	$(DEVKITPRO)/tools/bin/bin2s $< | $(DEVKITARM)/bin/arm-none-eabi-as -march=armv6k -mcpu=mpcore -o $@

$(TARGET).elf: source/main.c source/shader_shbin.h shader.o
	$(CC) $(CFLAGS) source/main.c shader.o $(LDFLAGS) $(LIBS) -o $@

$(TARGET).smdh: icon.png
	$(DEVKITPRO)/tools/bin/smdhtool --create "Winterstad" "Spidey en Deadpool kerstverhaal" "Voor mijn broertje" icon.png $@

$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh
	$(DEVKITPRO)/tools/bin/3dsxtool $< $@ --smdh=$(TARGET).smdh --romfs=romfs

clean:
	rm -f shader.shbin shader.o $(TARGET).elf $(TARGET).smdh $(TARGET).3dsx
