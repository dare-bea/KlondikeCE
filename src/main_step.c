#include "klondike.h"

#include <time.h>

#include <keypadc.h>

extern struct KlondikeGame game;
extern struct {
        size_t pos; // pos == TABLEAU_PILES: Waste
        bool holding_cards;
        size_t from_pos; // from_pos == TABLEAU_PILES: Waste
} cursor;
extern enum {
        GAME_ONGOING,
        GAME_WON,
        GAME_LOST
} game_status;

extern unsigned move_count;
extern uintmax_t game_secs;
extern clock_t game_start;

#define INPUT_KEYS_COUNT 6
enum InputKeyIndex {
        IKIdx_Left, IKIdx_Right, IKIdx_Enter, IKIdx_Trace, IKIdx_Graph, IKIdx_Clear
};
static const kb_lkey_t INPUT_KEYS[INPUT_KEYS_COUNT] = {
        kb_KeyLeft, kb_KeyRight, kb_KeyEnter, kb_KeyTrace, kb_KeyGraph, kb_KeyClear
};

static clock_t auto_repeat[INPUT_KEYS_COUNT] = {0};
static bool new_keys[INPUT_KEYS_COUNT] = {0};

#define CURSOR_AUTOREPEAT_RATE 0.125
#define CURSOR_AUTOREPEAT_DELAY 0.500

int step(void) {
        clock_t const now = clock();
        if (game_status == GAME_ONGOING) {
                game_secs = (uintmax_t)((now - game_start) / CLOCKS_PER_SEC);
                if (KlondikeGame_is_won(&game)) {
                        game_status = GAME_WON;
                } else if (!KlondikeGame_can_make_move(&game)) {
                        game_status = GAME_LOST;
                }
        }
        for (enum InputKeyIndex i = IKIdx_Left; i < INPUT_KEYS_COUNT; ++i) {
                bool pressed = kb_IsDown(INPUT_KEYS[i]);
                if (auto_repeat[i]) {
                        if (!pressed) {
                                auto_repeat[i] = 0;
                                new_keys[i] = false;
                        } else if (auto_repeat[i] < now
                                   && (i == IKIdx_Left || i == IKIdx_Right)) {
                                auto_repeat[i] = now + (clock_t)(CLOCKS_PER_SEC * (CURSOR_AUTOREPEAT_RATE));
                                new_keys[i] = true;
                        } else {
                                new_keys[i] = false;
                        }
                } else {
                        auto_repeat[i] = pressed ? now + (clock_t)(CLOCKS_PER_SEC * (CURSOR_AUTOREPEAT_DELAY)) : 0;
                        new_keys[i] = pressed;
                }
        }
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
                if (cursor.pos < TABLEAU_PILES)
                        ++cursor.pos;
        }
        return 0;
}
