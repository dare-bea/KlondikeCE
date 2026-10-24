#include "klondike.h"

#include <stdlib.h>
#include <time.h>

#include <graphx.h>
#include "gfx/gfx.h"
#include "gamepalette_colors.h"

#include <keypadc.h>

#include <fontlibc.h>
#include "fonts.h"

#include <sys/timers.h>

#ifdef CRASH_DUMP
#include <stdio.h>
#endif

/* -- COMPILE TIME FLAGS --
 * `CRASH_DUMP` - Dump the game struct when some assertion fails
 * `SHOW_MOVE_AVAILABLITY` - Calculate and show if a move is available from the
 *      hovered tableau/waste pile when you are not holding it.
 * `DEBUG_CURSOR_INFO` - Show cursor debug information
 */

#define INPUT_KEYS_COUNT 6
enum InputKeyIndex {
        IKIdx_Left, IKIdx_Right, IKIdx_Enter, IKIdx_Trace, IKIdx_Graph, IKIdx_Clear
};
static const kb_lkey_t INPUT_KEYS[INPUT_KEYS_COUNT] = {
        kb_KeyLeft, kb_KeyRight, kb_KeyEnter, kb_KeyTrace, kb_KeyGraph, kb_KeyClear
};

static int errno = 0;
static int errline = 0;

struct KlondikeGame game;
struct {
        size_t pos; // pos == TABLEAU_PILES: Waste
        bool holding_cards;
        size_t from_pos; // from_pos == TABLEAU_PILES: Waste
} cursor = {0};
unsigned move_count;

static bool pressed_keys[INPUT_KEYS_COUNT] = {0};
static bool new_keys[INPUT_KEYS_COUNT] = {0};

int step(void) {
        if (new_keys[IKIdx_Enter]) {
                if (!cursor.holding_cards) {
                        if (cursor.pos == TABLEAU_PILES
                            ? game.stock_waste.length != game.stock_remaining
                            : game.tableau_piles[cursor.pos].length) {
                                cursor.holding_cards = true;
                                cursor.from_pos = cursor.pos;
                        }
                } else if (cursor.from_pos == cursor.pos) {
                        cursor.holding_cards = false;
                } else if (cursor.from_pos != TABLEAU_PILES && cursor.pos != TABLEAU_PILES) {
                        int8_t move_res = KlondikeGame_move_tt(
                                &game, cursor.from_pos, cursor.pos);
                        if (move_res == 1) {
                                ++move_count;
                                cursor.holding_cards = false;
                        } else if (move_res < 0) {
                                return 5;
                        }
                } else if (cursor.from_pos == TABLEAU_PILES && cursor.pos != TABLEAU_PILES) {
                        int8_t move_res = KlondikeGame_move_wt(
                                &game, cursor.pos);
                        if (move_res == 1) {
                                ++move_count;
                                cursor.holding_cards = false;
                        } else if (move_res < 0) {
                                return 5;
                        }
                }
        }
        if (new_keys[IKIdx_Clear]) {
                cursor.holding_cards = false;
        }
        if (new_keys[IKIdx_Trace]) {
                size_t pos = cursor.holding_cards ? cursor.from_pos : cursor.pos;
                if (pos == TABLEAU_PILES) {
                        int8_t move_res = KlondikeGame_move_wf(&game);
                        if (move_res == 1) {
                                ++move_count;
                                cursor.holding_cards = false;
                        } else if (move_res < 0) {
                                return 5;
                        } else if (!cursor.holding_cards) {
                                for (size_t c = 0; c < TABLEAU_PILES; ++c) {
                                        if (KlondikeGame_can_move_tf(&game, c)) {
                                                cursor.pos = c;
                                                break;
                                        }
                                }
                        }
                } else {
                        int8_t move_res = KlondikeGame_move_tf(&game, pos);
                        if (move_res == 1) {
                                ++move_count;
                        } else if (move_res < 0) {
                                return 5;
                        } else if (!cursor.holding_cards) {
                                bool found_foundation_movable = false;
                                for (size_t c = 0; c < TABLEAU_PILES; ++c) {
                                        if (KlondikeGame_can_move_tf(&game, c)) {
                                                found_foundation_movable = true;
                                                cursor.pos = c;
                                                break;
                                        }
                                }
                                if (!found_foundation_movable && KlondikeGame_can_move_wf(&game)) {
                                        found_foundation_movable = true;
                                        cursor.pos = TABLEAU_PILES;
                                }
                        }
                }
        }
        if (new_keys[IKIdx_Graph]) {
                int8_t move_res = KlondikeGame_draw_stock(&game);
                if (move_res == 1) {
                        ++move_count;
                } else if (move_res < 0) {
                        return 5;
                }
        }
        if (new_keys[IKIdx_Left] && !new_keys[IKIdx_Right]) {
                if (0 < cursor.pos)
                        --cursor.pos;
        } else if (!new_keys[IKIdx_Left] && new_keys[IKIdx_Right]) {
                if (cursor.pos != TABLEAU_PILES)
                        ++cursor.pos;
        }
        return 0;
}

// Implemented in ./main_draw.c
int draw(void);

#ifdef CRASH_DUMP
int dump_snprintf(char *__restrict__ buffer, size_t count, size_t *index, const char *__restrict__ format, ...) {
        va_list va;
        va_start(va, format);
        int retval = vsnprintf(buffer + *index, count - *index, format, va);
        if (retval < 0) return retval;
        *index += retval;
        return retval;
}
#endif

int main(void)
{
        srand(time(NULL));

        gfx_Begin();
        gfx_SetPalette(gamepalette, sizeof_gamepalette, 0);
        gfx_SetDrawBuffer();
        fontlib_SetNewlineOptions(FONTLIB_ENABLE_AUTO_WRAP);
        fontlib_SetLineSpacing(0, 1);
        kb_SetMode(MODE_3_CONTINUOUS);
        kb_EnableOnLatch();
        kb_ClearOnLatch();

        if (!KlondikeGame_init(&game)) {
                errno = 1, errline = __LINE__;
        }

        while (!errno && !kb_On && !kb_IsDown(kb_KeyMode)) {
                for (size_t i = 0; i < INPUT_KEYS_COUNT; ++i) {
                        if (pressed_keys[i]) {
                                pressed_keys[i] = kb_IsDown(INPUT_KEYS[i]);
                                new_keys[i] = false;
                        } else {
                                pressed_keys[i] = kb_IsDown(INPUT_KEYS[i]);
                                new_keys[i] = pressed_keys[i];
                        }
                }
                errline = step();
                if (errline) {
                        errno = errno ? errno : 2;
                        break;
                }
                errline = draw();
                if (errline) {
                        errno = errno ? errno : 3;
                        break;
                }
                if (!KlondikeGame_invariants_hold(&game)) {
                        errno = 4, errline = __LINE__;
                        break;
                }
        }

        if (errno) {
                gfx_FillScreen(GAMEPALETTE_HEARTS);
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

        if (!errno || errno > 1) {
                KlondikeGame_free(&game);
        }

        gfx_End();
        return errno ? 1 : 0;
}
