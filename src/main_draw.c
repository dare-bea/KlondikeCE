#include "klondike.h"

#include <graphx.h>
#include "gfx/gfx.h"
#include "gamepalette_colors.h"

#include <fontlibc.h>
#include "fonts.h"

extern struct KlondikeGame game;
extern struct {
        size_t pos; // pos == CURSOR_WASTE_POS: Waste
        bool holding_cards;
        size_t from_pos; // from_pos == CURSOR_WASTE_POS: Waste
} cursor;
extern enum {
        GAME_WAITING,
        GAME_ONGOING,
        GAME_WON,
        GAME_LOST
} game_status;

#define CURSOR_WASTE_POS (TABLEAU_PILES)
#define CURSOR_STOCK_POS (TABLEAU_PILES + 1)

extern unsigned move_count;
extern uintmax_t game_secs;

static const char CARDRANK_SYMBOLS[CARDRANK_COUNT + 1] = "A23456789\x7FJQK";
static const char CARDSUIT_SYMBOLS[CARDSUIT_COUNT + 1] = "\x80\x81\x82\x83";

#define DIM_TABLE_X 8
#define DIM_TABLE_Y 16
#define DIM_TABLE_DX 32
#define DIM_TABLE_MAX_DY 24
#define DIM_TABLE_HEIGHT (GFX_LCD_HEIGHT - 16 /* top */ - 12 /* bottom */)

#define DIM_WASTE_X 240
#define DIM_WASTE_Y 24

#define DIM_STOCK_X 288
#define DIM_STOCK_Y 24

#define DIM_FOUND_X 244
#define DIM_FOUND_Y 90
#define DIM_FOUND_DX 40
#define DIM_FOUND_DY 50

#define DIM_CARDINFO_YOFF 3
#define DIM_HOLDCARD_YOFF 4

#define DIM_CONTROLINFO_X (DIM_TABLE_X + DIM_TABLE_DX * TABLEAU_PILES - 3)
#define DIM_CONTROLINFO_Y (DIM_FOUND_Y + DIM_FOUND_DY * 2 - 10)

__attribute__((nonnull(1)))
static inline void render_card(struct Card const *card, unsigned x, uint8_t y) {
        fontlib_SetFont(font8x8, 0);
        fontlib_SetForegroundColor(GAMEPALETTE_SPADES + card->suit);
        fontlib_SetCursorPosition(x + (card_front_width / 2) - 8, y + DIM_CARDINFO_YOFF);
        gfx_RLETSprite(card_front, x, y);
        fontlib_DrawGlyph(CARDRANK_SYMBOLS[card->rank]);
        fontlib_DrawGlyph(CARDSUIT_SYMBOLS[card->suit]);
}

int draw(void) {
        /* CALCULATE TABLEAU LENGTH */
        size_t tab_len = 0;
        for (size_t c = 0; c < TABLEAU_PILES; ++c) {
                size_t len = game.tableau_piles[c].length;
                if (cursor.holding_cards && cursor.pos != cursor.from_pos && cursor.pos == c) {
                        if (cursor.from_pos >= CURSOR_WASTE_POS) {
                                ++len;
                        } else {
                                len += game.tableau_piles[cursor.from_pos].length - game.tableau_down[cursor.from_pos];
                        }
                }
                if (len > tab_len)
                        tab_len = len;
        }
        unsigned row_off = ((DIM_TABLE_HEIGHT) - card_front_height) / (tab_len - 1);
        if (row_off > DIM_TABLE_MAX_DY)
                row_off = DIM_TABLE_MAX_DY;

        /* SETUP COLORS */
        fontlib_SetColors(GAMEPALETTE_WHITE, GAMEPALETTE_BLACK);
        fontlib_SetTransparency(true);
        gfx_SetColor(GAMEPALETTE_WHITE);
        gfx_FillScreen(GAMEPALETTE_GREEN);

        /* CONTROL INFORMATION */
        fontlib_SetFont(font4x8, 0);
        fontlib_SetLineSpacing(0, 1);
        fontlib_SetWindow(DIM_CONTROLINFO_X, DIM_CONTROLINFO_Y, GFX_LCD_WIDTH - DIM_CONTROLINFO_X, GFX_LCD_HEIGHT - DIM_CONTROLINFO_Y);
        fontlib_SetCursorPosition(DIM_CONTROLINFO_X, DIM_CONTROLINFO_Y);
        fontlib_DrawString(
                "LEFT/RIGHT: Cursor\n"
                "ENTER: Select/move\n"
                "CLEAR: Deselect pile\n"
                "F1: Play to foundation\n"
                "F2: Draw 3 from stock\n"
                "ON: Quit");
        fontlib_SetWindowFullScreen();

        /* DRAW BLANK LOCATIONS */
        fontlib_SetFont(font8x8, 0);

        // Foundations
        for (enum CardSuit suit = 0; suit < CARDSUIT_COUNT; ++suit) {
                gfx_Rectangle(
                        DIM_FOUND_X + DIM_FOUND_DX * (suit % 2) + 1,
                        DIM_FOUND_Y + DIM_FOUND_DY * (suit / 2) + 1,
                        card_front_width - 2,
                        card_front_height - 2);
                fontlib_SetCursorPosition(
                        DIM_FOUND_X + DIM_FOUND_DX * (suit % 2) + (card_front_width / 2) - 4,
                        DIM_FOUND_Y + DIM_FOUND_DY * (suit / 2) + (card_front_height / 2) - 4);
                fontlib_DrawGlyph(CARDSUIT_SYMBOLS[suit]);
        }

        // Waste
        gfx_Rectangle( DIM_WASTE_X + 1, DIM_WASTE_Y + 1,
                card_front_width - 2, card_front_height - 2);

        // Stock
        gfx_Rectangle(DIM_STOCK_X + 1, DIM_STOCK_Y + 1,
                card_front_width - 2, card_front_height - 2);

        // Tableau
        for (size_t c = 0; c < TABLEAU_PILES; ++c) {
                gfx_Rectangle(c * DIM_TABLE_DX + DIM_TABLE_X + 1, DIM_TABLE_Y + 1,
                        card_front_width - 2, card_front_height - 2);
        }

        /* DRAW DECK COUNTS */
        fontlib_SetFont(font6x6, 0);

        // Waste
        fontlib_SetCursorPosition(DIM_WASTE_X + 3, DIM_WASTE_Y + card_back_height + 2);
        fontlib_DrawUInt(game.stock_waste.length - game.stock_remaining, 1);
        fontlib_DrawGlyph('x');

        // Stock
        fontlib_SetCursorPosition(DIM_STOCK_X + 3, DIM_WASTE_Y + card_back_height + 2);
        fontlib_DrawUInt(game.stock_remaining, 1);
        fontlib_DrawGlyph('x');

        /* CARDS */
        fontlib_SetFont(font8x8, 0);

        // Foundations
        for (enum CardSuit suit = 0; suit < CARDSUIT_COUNT; ++suit) {
                if (game.foundations[suit].has_cards) {
                        render_card(&(struct Card) {
                                        .rank = game.foundations[suit].top_rank,
                                        .suit = suit,
                                },
                                DIM_FOUND_X + DIM_FOUND_DX * (suit % 2),
                                DIM_FOUND_Y + DIM_FOUND_DY * (suit / 2));
                }
        }

        // Waste
        if (cursor.holding_cards && cursor.from_pos == CURSOR_WASTE_POS && cursor.pos <= CURSOR_WASTE_POS) {
                struct Card const *waste_card = Vector_structCard_at_const(
                        &game.stock_waste,
                        game.stock_remaining + (cursor.pos == CURSOR_WASTE_POS ? 0 : 1));
                if (waste_card) {
                        if (game.stock_waste.length - game.stock_remaining > 1) {
                                gfx_RLETSprite(card_front, DIM_WASTE_X, DIM_WASTE_Y);
                        }
                        render_card(waste_card,
                                DIM_WASTE_X,
                                DIM_WASTE_Y - (cursor.pos == CURSOR_WASTE_POS ? DIM_HOLDCARD_YOFF : 0));
                }
        } else {
                struct Card const *waste_card = Vector_structCard_at_const(
                        &game.stock_waste, game.stock_remaining);
                if (waste_card) {
                        render_card(waste_card, DIM_WASTE_X, DIM_WASTE_Y);
                }
        }

        // Stock
        if (game.stock_remaining) {
                gfx_RLETSprite(card_back, DIM_STOCK_X, DIM_STOCK_Y);
        }

        // Tableau
        for (size_t c = 0; c < TABLEAU_PILES; ++c) {
                for (size_t r = 0; r < game.tableau_down[c]; ++r) {
                        gfx_RLETSprite(card_back,
                                DIM_TABLE_X + DIM_TABLE_DX * c,
                                DIM_TABLE_Y + row_off * r);
                }
                if (!cursor.holding_cards || c != cursor.from_pos || cursor.pos >= CURSOR_WASTE_POS) {
                        for (size_t r = game.tableau_down[c]; r < game.tableau_piles[c].length; ++r) {
                                struct Card const *card = Vector_structCard_at_const(
                                        &game.tableau_piles[c], r);
                                if (!card) return __LINE__;
                                render_card(card,
                                        DIM_TABLE_X + DIM_TABLE_DX * c,
                                        DIM_TABLE_Y + row_off * r);
                        }
                }
        }

        // Held cards in tableau
        if (cursor.holding_cards && cursor.pos < TABLEAU_PILES) {
                if (cursor.from_pos < TABLEAU_PILES) {
                        for (size_t r = game.tableau_down[cursor.from_pos]; r < game.tableau_piles[cursor.from_pos].length; ++r) {
                                size_t r_adjusted = cursor.from_pos == cursor.pos ? r
                                        : r - game.tableau_down[cursor.from_pos] + game.tableau_piles[cursor.pos].length;
                                struct Card const *card = Vector_structCard_at_const(
                                        &game.tableau_piles[cursor.from_pos], r);
                                if (!card) return __LINE__;
                                render_card(card,
                                        DIM_TABLE_X + DIM_TABLE_DX * cursor.pos,
                                        DIM_TABLE_Y + row_off * r_adjusted - DIM_HOLDCARD_YOFF);
                        }
                } else {
                        size_t r = game.tableau_piles[cursor.pos].length;
                        struct Card const *card = Vector_structCard_at_const(&game.stock_waste,
                                game.stock_remaining);
                        if (!card) return __LINE__;
                        render_card(card,
                                DIM_TABLE_X + DIM_TABLE_DX * cursor.pos,
                                DIM_TABLE_Y + row_off * r - DIM_HOLDCARD_YOFF);
                }
        }

        /* CHECK IF MOVE IS AVAILABLE */
        bool move_found = true;
        if (cursor.holding_cards) {
                if (cursor.pos == cursor.from_pos) {
                        move_found = true;
                } else if (cursor.pos >= CURSOR_WASTE_POS) {
                        move_found = false;
                } else if (cursor.from_pos == CURSOR_WASTE_POS) {
                        move_found = KlondikeGame_can_move_wt(&game, cursor.pos);
                } else {
                        move_found = KlondikeGame_can_move_tt(&game,
                                cursor.from_pos, cursor.pos);
                }
        }
#ifndef SHOW_MOVE_AVAILABLITY
        else if (cursor.pos < TABLEAU_PILES) {
                move_found = game.tableau_piles[cursor.pos].length;
        } else if (cursor.pos == CURSOR_WASTE_POS) {
                move_found = game.stock_waste.length != game.stock_remaining;
        } else if (cursor.pos == CURSOR_STOCK_POS) {
                move_found = game.stock_waste.length;
        }
#else
        else if (cursor.pos < TABLEAU_PILES) {
                move_found = KlondikeGame_can_move_tf(&game, cursor.pos);
                if (!move_found) for (size_t c = 0; c < TABLEAU_PILES; ++c) {
                        if (KlondikeGame_can_move_tt(&game, cursor.pos, c)) {
                                move_found = true;
                                break;
                        }
                }
        } else if (cursor.pos == CURSOR_WASTE_POS) {
                move_found = KlondikeGame_can_move_wf(&game);
                if (!move_found) for (size_t c = 0; c < TABLEAU_PILES; ++c) {
                        if (KlondikeGame_can_move_wt(&game, c)) {
                                move_found = true;
                                break;
                        }
                }
        } else if (cursor.pos == CURSOR_STOCK_POS) {
                move_found = game.stock_waste.length;
        }
#endif

        /* RENDER CURSOR */
        if (cursor.pos < TABLEAU_PILES) {
                unsigned cursor_x = DIM_TABLE_X + DIM_TABLE_DX * cursor.pos
                        + card_back_width / 2 - cursor_pointer_down_width / 2;
                unsigned cursor_y = DIM_TABLE_Y - 2 - (
                                !move_found ? cursor_denied_height
                                : cursor.holding_cards ? cursor_pointer_down_height
                                : cursor_pointer_up_height);
                if (cursor.holding_cards && cursor.pos == cursor.from_pos
                    && game.tableau_down[cursor.pos] == 0)
                        cursor_y -= DIM_HOLDCARD_YOFF;
                else if (cursor.holding_cards && cursor.pos != cursor.from_pos
                    && game.tableau_piles[cursor.pos].length == 0)
                        cursor_y -= DIM_HOLDCARD_YOFF;
                gfx_RLETSprite(!move_found ? cursor_denied
                                : cursor.holding_cards ? cursor_pointer_down
                                : cursor_pointer_up,
                        cursor_x,
                        cursor_y);
        } else {
                unsigned cursor_x = (cursor.pos == CURSOR_STOCK_POS
                                ? DIM_STOCK_X : DIM_WASTE_X)
                        + card_back_width / 2 - cursor_pointer_down_width / 2;
                unsigned cursor_y = DIM_WASTE_Y - 2 - (
                        !move_found ? cursor_denied_height
                        : cursor.holding_cards ? cursor_pointer_down_height
                        : cursor.pos == CURSOR_STOCK_POS && !game.stock_remaining
                                ? cursor_pointer_down_height
                        : cursor_pointer_up_height);
                if (cursor.holding_cards && cursor.pos == cursor.from_pos)
                        cursor_y -= DIM_HOLDCARD_YOFF;
                gfx_RLETSprite(!move_found ? cursor_denied
                                : cursor.holding_cards ? cursor_pointer_down
                                : cursor.pos == CURSOR_STOCK_POS && !game.stock_remaining
                                        ? cursor_pointer_down
                                : cursor_pointer_up,
                        cursor_x,
                        cursor_y);
        }

        fontlib_SetForegroundColor(GAMEPALETTE_WHITE);

        /* MOVE COUNTER */
        fontlib_SetCursorPosition(4, GFX_LCD_HEIGHT - 8 - 4);
        fontlib_SetFont(font6x8, 0);
        fontlib_DrawUInt(move_count, 1);
        fontlib_DrawString(" Moves  ");
        fontlib_DrawUInt(game_secs / 60, 1);
        fontlib_DrawGlyph(':');
        fontlib_DrawUInt(game_secs % 60, 2);

        /* SHOW MESSAGE FOR WINNING OR LOSING */
        fontlib_SetFont(font8x8, 0);
        fontlib_ShiftCursorPosition(12, 0);
        if (game_status == GAME_WON) {
                fontlib_DrawString("GAME WON!");
        } else if (game_status == GAME_LOST) {
                fontlib_DrawString("GAME UNSOLVABLE!");
        }
#ifdef DEBUG_CURSOR_INFO
        else {
                fontlib_SetFont(font4x8, 0);
                fontlib_DrawString(" -- [cursor] hold=");
                fontlib_DrawUInt(cursor.holding_cards, 1);
                fontlib_DrawString(" pos=");
                fontlib_DrawUInt(cursor.pos, 1);
                fontlib_DrawString(" from=");
                fontlib_DrawUInt(cursor.from_pos, 1);
        }
#endif

        gfx_SwapDraw();
        return 0;
}
