#include <graphx.h>
#include "gfx/gfx.h"
#include "gamepalette_colors.h"

#include <fontlibc.h>
#include "fonts.h"

#include "keypadc.h"

#include <sys/timers.h>

#ifdef CRASH_DUMP
#include "klondike.h"

#include <stdio.h>

extern struct KlondikeGame game;

static int dump_snprintf(char *__restrict__ buffer, size_t count, size_t *index, const char *__restrict__ format, ...) {
        va_list va;
        va_start(va, format);
        int retval = vsnprintf(buffer + *index, count - *index, format, va);
        if (retval < 0) return retval;
        *index += retval;
        return retval;
}
#endif

void error_screen(int errno, int errline) {
        gfx_FillScreen(GAMEPALETTE_HEART);
        fontlib_SetFont(font8x8, 0);
        fontlib_SetTransparency(true);
        fontlib_SetForegroundColor(GAMEPALETTE_WHITE);
        fontlib_SetCursorPosition(8, 8);
        fontlib_DrawString("Something bad happened!");
        fontlib_SetCursorPosition(8, 18);
        fontlib_DrawString("Error ");
        fontlib_DrawInt(errno, 1);
        fontlib_SetCursorPosition(8, 28);
        fontlib_DrawString("Line ");
        fontlib_DrawInt(errline, 1);
        fontlib_SetFont(font6x8, 0);
        fontlib_SetCursorPosition(8, GFX_LCD_HEIGHT - 8 - 6);
        fontlib_DrawString(
                "Press ON to quit. Press GRAPH to see last frame.");
#ifdef CRASH_DUMP
        fontlib_SetFont(font4x6, 0);
        fontlib_SetCursorPosition(0, 48);
        char buf[512];
        size_t bufidx = 0;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-pedantic"
        __builtin_dump_struct(&game, dump_snprintf, buf, sizeof(buf)/sizeof(char), &bufidx);
#pragma clang diagnostic pop
        fontlib_DrawStringL(buf, sizeof(buf)/sizeof(char));
#endif
        gfx_SwapDraw();
        msleep(1500);
        bool swap_pressed = false;
        while (!kb_On) {
                kb_Scan();
                bool down = kb_IsDown(kb_KeyGraph);
                if (!swap_pressed && down) {
                        gfx_SwapDraw();
                        gfx_Wait();
                }
                swap_pressed = down;
        }
}
