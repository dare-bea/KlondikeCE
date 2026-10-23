#include <fonts.h>

/* This contains the raw data for the font. */
static const uint8_t font8x8_data[] = {
    #include "fonts/font8x8.inc"
};
/* However, C89 does not allow us to typecast a byte array into a
fontlib_font_t pointer directly, so we have to use a second statement to do it,
though helpfully we can at least do it in the global scope. */
const fontlib_font_t *font8x8 = (fontlib_font_t *)font8x8_data;

static const uint8_t font6x8_data[] = {
    #include "fonts/font6x8.inc"
};
const fontlib_font_t *font6x8 = (fontlib_font_t *)font6x8_data;

static const uint8_t font6x6_data[] = {
    #include "fonts/font6x6.inc"
};
const fontlib_font_t *font6x6 = (fontlib_font_t *)font6x6_data;

static const uint8_t font4x8_data[] = {
    #include "fonts/font4x8.inc"
};
const fontlib_font_t *font4x8 = (fontlib_font_t *)font4x8_data;

static const uint8_t font4x6_data[] = {
    #include "fonts/font4x6.inc"
};
const fontlib_font_t *font4x6 = (fontlib_font_t *)font4x6_data;
