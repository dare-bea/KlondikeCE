#include "klondike.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <graphx.h>
#include "gfx/gfx.h"
#include "gfx/card_front.h"
#include "gfx/card_back.h"
#include "gfx/cursor_pointer_up.h"
#include "gfx/cursor_pointer_down.h"
#include "gamepalette_colors.h"

#include <keypadc.h>

#include <fontlibc.h>
#include "fonts.h"

#include <sys/timers.h>

static const char CARDRANK_SYMBOLS[] = "A23456789\x7FJQK";
static const char CARDSUIT_SYMBOLS[] = "\x80\x81\x82\x83";

#define INPUT_KEYS_COUNT 6
enum InputKeyIndex {
        IKIdx_Left, IKIdx_Right, IKIdx_Enter, IKIdx_Trace, IKIdx_Graph, IKIdx_Clear
};
static const kb_lkey_t INPUT_KEYS[INPUT_KEYS_COUNT] = {
        kb_KeyLeft, kb_KeyRight, kb_KeyEnter, kb_KeyTrace, kb_KeyGraph, kb_KeyClear
};

static int errno = 0;
static int errline = 0;

static struct KlondikeGame game;
static struct {
        size_t pos; // pos == TABLEAU_PILES: Waste
        bool holding_cards;
        size_t from_pos; // from_pos == TABLEAU_PILES: Waste
} cursor = {0};

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
                                cursor.holding_cards = false;
                        } else if (move_res < 0) {
                                return 5;
                        }
                } else if (cursor.from_pos == TABLEAU_PILES && cursor.pos != TABLEAU_PILES) {
                        int8_t move_res = KlondikeGame_move_wt(
                                &game, cursor.pos);
                        if (move_res == 1) {
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
                                // just movement, for now
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
                        // just condition's side effect, for now
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

int draw(void) {
        gfx_FillScreen(GAMEPALETTE_GREEN);
        gfx_SetColor(GAMEPALETTE_WHITE);
        fontlib_SetColors(GAMEPALETTE_WHITE, GAMEPALETTE_BLACK);
        fontlib_SetTransparency(true);

        /* DEBUG */
        fontlib_SetFont(font4x8, 0);
        fontlib_SetCursorPosition(4, GFX_LCD_HEIGHT - 8 - 4);
        fontlib_DrawString("[cursor] hold=");
        fontlib_DrawUInt(cursor.holding_cards, 1);
        fontlib_DrawString(" pos=");
        fontlib_DrawUInt(cursor.pos, 1);
        fontlib_DrawString(" from=");
        fontlib_DrawUInt(cursor.from_pos, 1);
        /* DRAW */
        size_t tab_len = 0;
        for (size_t c = 0; c < TABLEAU_PILES; ++c) {
                if (game.tableau_piles[c].length > tab_len)
                        tab_len = game.tableau_piles[c].length;
        }

        unsigned row_off = (GFX_LCD_HEIGHT - 24 - card_front_height) / (tab_len - 1);
        if (row_off > 24) row_off = 24;

        /* FOUNDATIONS */
        fontlib_SetFont(font8x8, 0);
        for (enum CardSuit suit = 0; suit < CARDSUIT_COUNT; ++suit) {
                if (!game.foundations[suit].has_cards) {
                        gfx_Rectangle(
                                244 + 40 * (suit % 2) + 1,
                                90 + 50 * (suit / 2) + 1,
                                card_front_width - 2,
                                card_front_height - 2);
                        fontlib_SetForegroundColor(GAMEPALETTE_WHITE);
                        fontlib_SetCursorPosition(
                                244 + 40 * (suit % 2) + (card_front_width / 2) - 4,
                                90 + 50 * (suit / 2) + (card_front_height / 2) - 4);
                        fontlib_DrawGlyph(CARDSUIT_SYMBOLS[suit]);
                } else {
                        gfx_RLETSprite(card_front,
                                244 + 40 * (suit % 2),
                                90 + 50 * (suit / 2));
                        fontlib_SetForegroundColor(GAMEPALETTE_SPADES + suit);
                        fontlib_SetCursorPosition(
                                244 + 40 * (suit % 2) + (card_front_width / 2) - 8,
                                90 + 50 * (suit / 2) + 2);
                        fontlib_DrawGlyph(CARDRANK_SYMBOLS[game.foundations[suit].top_rank]);
                        fontlib_DrawGlyph(CARDSUIT_SYMBOLS[suit]);
                }
        }

        /* WASTE */
        struct Card const *waste_card = Vector_structCard_at_const(&
                game.stock_waste, game.stock_remaining);
        if (!waste_card) {
                gfx_Rectangle( 240 + 1, 24 + 1,
                        card_front_width - 2,
                        card_front_height - 2);
                /*fontlib_SetForegroundColor(GAMEPALETTE_WHITE);
                fontlib_SetCursorPosition(
                        240 + 40 * (suit % 2) + (card_front_width / 2) - 4,
                        90 + 50 * (suit / 2) + (card_front_height / 2) - 4);
                fontlib_DrawGlyph(CARDSUIT_SYMBOLS[suit]);*/
        } else {
                gfx_RLETSprite(card_front, 240, 24);
                fontlib_SetForegroundColor(GAMEPALETTE_SPADES + waste_card->suit);
                fontlib_SetCursorPosition(
                        240 + (card_front_width / 2) - 8,
                        24 + 2);
                fontlib_SetFont(font8x8, 0);
                fontlib_DrawGlyph(CARDRANK_SYMBOLS[waste_card->rank]);
                fontlib_DrawGlyph(CARDSUIT_SYMBOLS[waste_card->suit]);
        }
        fontlib_SetCursorPosition(
                240 + 3,
                24 + card_back_height + 2);
        fontlib_SetFont(font6x6, 0);
        fontlib_SetForegroundColor(GAMEPALETTE_WHITE);
        fontlib_DrawUInt(game.stock_waste.length - game.stock_remaining, 1);
        fontlib_DrawGlyph('x');

        /* STOCK */
        fontlib_SetForegroundColor(GAMEPALETTE_WHITE);
        if (!game.stock_remaining) {
                gfx_Rectangle( 288 + 1, 24 + 1,
                        card_front_width - 2,
                        card_front_height - 2);
                /*fontlib_SetForegroundColor(GAMEPALETTE_WHITE);
                fontlib_SetCursorPosition(
                        240 + 40 * (suit % 2) + (card_front_width / 2) - 4,
                        90 + 50 * (suit / 2) + (card_front_height / 2) - 4);
                fontlib_DrawGlyph(CARDSUIT_SYMBOLS[suit]);*/
        } else {
                gfx_RLETSprite(card_back, 288, 24);
        }
        fontlib_SetCursorPosition(
                288 + 3,
                24 + card_back_height + 2);
        fontlib_SetFont(font6x6, 0);
        fontlib_DrawUInt(game.stock_remaining, 1);
        fontlib_DrawGlyph('x');

        /* TABLEAU */
        fontlib_SetFont(font8x8, 0);
        for (size_t c = 0; c < TABLEAU_PILES; ++c) {
                if (!game.tableau_piles[c].length) {
                        gfx_Rectangle(c * 32 + 8 + 1, 16 + 1,
                                card_front_width - 2,
                                card_front_height - 2);
                        continue;
                }
                for (size_t r = 0; r < game.tableau_down[c]; ++r) {
                        gfx_RLETSprite(card_back, c * 32 + 8, r * row_off + 16);
                }
                for (size_t r = game.tableau_down[c]; r < game.tableau_piles[c].length; ++r) {
                        struct Card const *card = Vector_structCard_at_const(
                                &game.tableau_piles[c], r);
                        if (!card) return __LINE__;
                        gfx_RLETSprite(card_front, c * 32 + 8, r * row_off + 16);
                        fontlib_SetForegroundColor(GAMEPALETTE_SPADES + card->suit);
                        fontlib_SetCursorPosition(
                                c * 32 + 8 + (card_front_width / 2) - 8,
                                r * row_off + 16 + 2);
                        fontlib_DrawGlyph(CARDRANK_SYMBOLS[card->rank]);
                        fontlib_DrawGlyph(CARDSUIT_SYMBOLS[card->suit]);
                }
        }

        /* CURSOR */

#ifdef SHOW_MOVE_AVAILABLITY
        bool move_found;
        if (cursor.holding_cards) {
                if (cursor.pos == cursor.from_pos) {
                        move_found = true;
                } else if (cursor.pos == TABLEAU_PILES) {
                        move_found = false;
                } else if (cursor.from_pos == TABLEAU_PILES) {
                        move_found = KlondikeGame_can_move_wt(&game, cursor.pos);
                } else {
                        move_found = KlondikeGame_can_move_tt(&game,
                                cursor.from_pos, cursor.pos);
                }
        } else if (cursor.pos < TABLEAU_PILES) {
                move_found = KlondikeGame_can_move_tf(&game, cursor.pos);
                if (!move_found) for (size_t c = 0; c < TABLEAU_PILES; ++c) {
                        if (KlondikeGame_can_move_tt(&game, cursor.pos, c)) {
                                move_found = true;
                                break;
                        }
                }
        } else {
                move_found = KlondikeGame_can_move_wf(&game);
                if (!move_found) for (size_t c = 0; c < TABLEAU_PILES; ++c) {
                        if (KlondikeGame_can_move_wt(&game, c)) {
                                move_found = true;
                                break;
                        }
                }
        }
#else
        bool move_found = true;
#endif

        if (cursor.pos != TABLEAU_PILES) {
                gfx_RLETSprite(!move_found ? cursor_denied
                        : cursor.holding_cards ? cursor_pointer_down : cursor_pointer_up,
                        cursor.pos * 32 + 8 + card_back_width / 2 - cursor_pointer_down_width / 2,
                        16 - cursor_pointer_down_height - 2);
        } else {
                gfx_RLETSprite(!move_found ? cursor_denied
                        : cursor.holding_cards ? cursor_pointer_down : cursor_pointer_up,
                        240 + card_back_width / 2 - cursor_pointer_down_width / 2,
                        24 - cursor_pointer_down_height - 2);
        }

        fontlib_SetFont(font8x8, 0);
        fontlib_SetTransparency(true);
        fontlib_SetCursorPosition(GFX_LCD_WIDTH / 2, GFX_LCD_HEIGHT - 8 - 4);
        fontlib_SetForegroundColor(GAMEPALETTE_WHITE);
        /* WINNING/LOSS */
        if (KlondikeGame_is_won(&game)) {
                fontlib_DrawString("GAME WON!");
        } else if (!KlondikeGame_can_make_move(&game)) {
                fontlib_DrawString("GAME UNSOLVABLE");
        }

        gfx_SwapDraw();
        return 0;
}

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
                fontlib_SetNewlineOptions(FONTLIB_ENABLE_AUTO_WRAP);
                fontlib_SetLineSpacing(0, 1);
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
