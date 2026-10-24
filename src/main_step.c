#include "klondike.h"

#include <keypadc.h>

extern struct KlondikeGame game;
extern struct {
        size_t pos; // pos == TABLEAU_PILES: Waste
        bool holding_cards;
        size_t from_pos; // from_pos == TABLEAU_PILES: Waste
} cursor;
extern unsigned move_count;

#define INPUT_KEYS_COUNT 6
enum InputKeyIndex {
        IKIdx_Left, IKIdx_Right, IKIdx_Enter, IKIdx_Trace, IKIdx_Graph, IKIdx_Clear
};
static const kb_lkey_t INPUT_KEYS[INPUT_KEYS_COUNT] = {
        kb_KeyLeft, kb_KeyRight, kb_KeyEnter, kb_KeyTrace, kb_KeyGraph, kb_KeyClear
};

static bool pressed_keys[INPUT_KEYS_COUNT] = {0};
static bool new_keys[INPUT_KEYS_COUNT] = {0};

int step(void) {
        for (size_t i = 0; i < INPUT_KEYS_COUNT; ++i) {
                if (pressed_keys[i]) {
                        pressed_keys[i] = kb_IsDown(INPUT_KEYS[i]);
                        new_keys[i] = false;
                } else {
                        pressed_keys[i] = kb_IsDown(INPUT_KEYS[i]);
                        new_keys[i] = pressed_keys[i];
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
                if (cursor.pos != TABLEAU_PILES)
                        ++cursor.pos;
        }
        return 0;
}
