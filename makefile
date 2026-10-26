NAME = KLONDIKE
ICON = icon.png
DESCRIPTION = "Basic Solitare Implementation on CE"
COMPRESSED = YES
ARCHIVED = NO

FLAGS = -Oz -Wall -Wextra -Wpedantic -Werror -Wno-error=newline-eof
FLAGS += -Wno-fixed-enum-extension -Wno-gnu-conditional-omitted-operand -Wno-gnu-case-range

CFLAGS = -std=gnu17 $(FLAGS)
CXXFLAGS = -std=gnu++20 $(FLAGS)

HAS_PRINTF = YES

PYTHON = python3

TMPDIR = src/.tmp
FONTDIR = src/fonts
PYTMPFILES =
PYSRCFILES =
DEPS += $(PYTMPFILES)

include $(shell cedev-config --makefile)

define EXTRA_CLEAN
	$(Q)$(call RMDIR,$(TMPDIR))
endef

.PHONY += font

font: $(SRCDIR)/imgtoconvfont.py $(SRCDIR)/fonts.png
	$(Q)$(call RMDIR,$(FONTDIR))
	$(Q)$(MAKE) $(FONTDIR)/font8x8.inc
	$(Q)$(MAKE) $(FONTDIR)/font6x8.inc
	$(Q)$(MAKE) $(FONTDIR)/font4x8.inc
	$(Q)$(MAKE) $(FONTDIR)/font6x6.inc
	$(Q)$(MAKE) $(FONTDIR)/font4x6.inc
	$(Q)$(MAKE) $(FONTDIR)/font12x16.inc

$(SRCDIR)/fonts.c: $(FONTDIR)/font8x8.inc $(FONTDIR)/font6x8.inc $(FONTDIR)/font6x6.inc $(FONTDIR)/font4x8.inc $(FONTDIR)/font4x6.inc

$(FONTDIR)/font8x8.inc: $(SRCDIR)/imgtoconvfont.py $(SRCDIR)/fonts.png
	$(Q)$(call MKDIR,$(@D))
	$(Q)python3 $(SRCDIR)/imgtoconvfont.py $(SRCDIR)/fonts.png 8 8 16 --x 1 --y 1 --xheight 5 --bgcolor \#FFFFFF \#c0c0c0 > $(FONTDIR)/font8x8.convfont --glyph 0x80:130,1 --glyph 0x81:130,9 --glyph 0x82:146,1 --glyph 0x83:146,9
	$(Q)convfont -o carray -t $(FONTDIR)/font8x8.convfont $(FONTDIR)/font8x8.inc

$(FONTDIR)/font6x8.inc: $(SRCDIR)/imgtoconvfont.py $(SRCDIR)/fonts.png
	$(Q)$(call MKDIR,$(@D))
	$(Q)python3 $(SRCDIR)/imgtoconvfont.py $(SRCDIR)/fonts.png 6 8 16 --x 1 --y 50 --xheight 5 --bgcolor \#FFFFFF \#c0c0c0 > $(FONTDIR)/font6x8.convfont --glyph 0x80:138,1 --glyph 0x81:138,9 --glyph 0x82:154,1 --glyph 0x83:154,9
	$(Q)convfont -o carray -t $(FONTDIR)/font6x8.convfont $(FONTDIR)/font6x8.inc

$(FONTDIR)/font6x6.inc: $(SRCDIR)/imgtoconvfont.py $(SRCDIR)/fonts.png
	$(Q)$(call MKDIR,$(@D))
	$(Q)python3 $(SRCDIR)/imgtoconvfont.py $(SRCDIR)/fonts.png 6 6 16 --x 1 --y 99 --xheight 4 --bgcolor \#FFFFFF \#c0c0c0 > $(FONTDIR)/font6x6.convfont --glyph 0x80:138,2 --glyph 0x81:138,10 --glyph 0x82:154,2 --glyph 0x83:154,10
	$(Q)convfont -o carray -t $(FONTDIR)/font6x6.convfont $(FONTDIR)/font6x6.inc

$(FONTDIR)/font4x8.inc: $(SRCDIR)/imgtoconvfont.py $(SRCDIR)/fonts.png
	$(Q)$(call MKDIR,$(@D))
	$(Q)python3 $(SRCDIR)/imgtoconvfont.py $(SRCDIR)/fonts.png 4 8 16 --x 98 --y 50 --xheight 5 --bgcolor \#FFFFFF \#c0c0c0 > $(FONTDIR)/font4x8.convfont
	$(Q)convfont -o carray -t $(FONTDIR)/font4x8.convfont $(FONTDIR)/font4x8.inc

$(FONTDIR)/font4x6.inc: $(SRCDIR)/imgtoconvfont.py $(SRCDIR)/fonts.png
	$(Q)$(call MKDIR,$(@D))
	$(Q)python3 $(SRCDIR)/imgtoconvfont.py $(SRCDIR)/fonts.png 4 6 16 --x 98 --y 99 --xheight 4 --bgcolor \#FFFFFF \#c0c0c0 > $(FONTDIR)/font4x6.convfont
	$(Q)convfont -o carray -t $(FONTDIR)/font4x6.convfont $(FONTDIR)/font4x6.inc

$(FONTDIR)/font12x16.inc: $(SRCDIR)/imgtoconvfont.py $(SRCDIR)/fonts.png
	$(Q)$(call MKDIR,$(@D))
	$(Q)python3 $(SRCDIR)/imgtoconvfont.py $(SRCDIR)/fonts.png 12 16 16 --x 1 --y 136 --xheight 9 --bgcolor \#FFFFFF \#c0c0c0 > $(FONTDIR)/font12x16.convfont --glyph 0x80:194,168 --glyph 0x81:194,184 --glyph 0x82:206,168 --glyph 0x83:206,184
	$(Q)convfont -o carray -t $(FONTDIR)/font12x16.convfont $(FONTDIR)/font12x16.inc

$(PYTMPFILES) &: $(SRCDIR)/build.py $(PYSRCFILES)
	$(Q)$(call MKDIR,$(@D))
	$(Q)echo [py] $<
	$(Q)$(PYTHON) $(SRCDIR)/build.py $(SRCDIR)