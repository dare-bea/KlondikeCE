#include "klondike.h"

#include <time.h>

#include <keypadc.h>

extern struct KlondikeGame game;
extern struct {
        size_t pos; // pos == CURSOR_WASTE_POS: Waste
        bool holding_cards;
        size_t from_pos; // from_pos == CURSOR_WASTE_POS: Waste
} cursor;
extern enum {
        GAME_ONGOING,
        GAME_WON,
        GAME_LOST
} game_status;

#define CURSOR_WASTE_POS (TABLEAU_PILES)
#define CURSOR_STOCK_POS (TABLEAU_PILES + 1)

extern unsigned move_count;
extern uintmax_t game_secs;
extern clock_t game_start;

#define INPUT_KEYS_COUNT 7
enum InputKeyIndex {
        IKIdx_Left, IKIdx_Right, IKIdx_Yequ, IKIdx_Window, IKIdx_Clear, IKIdx_Enter, IKIdx_Graph
};
static const kb_lkey_t INPUT_KEYS[INPUT_KEYS_COUNT] = {
        kb_KeyLeft, kb_KeyRight, kb_KeyYequ, kb_KeyWindow, kb_KeyClear, kb_KeyEnter, kb_KeyGraph
};

static clock_t auto_repeat[INPUT_KEYS_COUNT] = {0};
static bool new_keys[INPUT_KEYS_COUNT] = {0};

#define CURSOR_AUTOREPEAT_RATE 0.125
#define CURSOR_AUTOREPEAT_DELAY 0.333

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
        for (enum InputKeyIndex i = 0; i < INPUT_KEYS_COUNT; ++i) {
                bool pressed = kb_IsDown(INPUT_KEYS[i]);
                if (auto_repeat[i]) {
                        if (!pressed) {
                                auto_repeat[i] = 0;
                                new_keys[i] = false;
                        } else if (auto_repeat[i] < now
                                   && (i == IKIdx_Left || i == IKIdx_Right || i == IKIdx_Yequ)) {
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
        if (new_keys[IKIdx_Enter] || new_keys[IKIdx_Graph]) {
                if (!cursor.holding_cards) {
                        if (cursor.pos == CURSOR_STOCK_POS) {
                                int8_t move_res = KlondikeGame_draw_stock(&game);
                                if (move_res == 1) {
                                        // ++move_count;
                                } else if (move_res < 0) {
                                        return __LINE__;
                                }
                        } else if (cursor.pos == CURSOR_WASTE_POS
                                   ? game.stock_waste.length != game.stock_remaining
                                   : game.tableau_piles[cursor.pos].length) {
                                cursor.holding_cards = true;
                                cursor.from_pos = cursor.pos;
                        }
                } else if (cursor.from_pos == cursor.pos) {
                        cursor.holding_cards = false;
                } else if (cursor.from_pos < TABLEAU_PILES && cursor.pos < TABLEAU_PILES) {
                        int8_t move_res = KlondikeGame_move_tt(
                                &game, cursor.from_pos, cursor.pos);
                        if (move_res == 1) {
                                ++move_count;
                                cursor.holding_cards = false;
                        } else if (move_res < 0) {
                                return __LINE__;
                        }
                } else if (cursor.from_pos == CURSOR_WASTE_POS && cursor.pos < TABLEAU_PILES) {
                        int8_t move_res = KlondikeGame_move_wt(
                                &game, cursor.pos);
                        if (move_res == 1) {
                                ++move_count;
                                cursor.holding_cards = false;
                        } else if (move_res < 0) {
                                return __LINE__;
                        }
                }
        }
        if (new_keys[IKIdx_Clear]) {
                cursor.holding_cards = false;
        }
        if (new_keys[IKIdx_Yequ]) {
                size_t pos = cursor.holding_cards ? cursor.from_pos : cursor.pos;
                if (pos == CURSOR_WASTE_POS) {
                        int8_t move_res = KlondikeGame_move_wf(&game);
                        if (move_res == 1) {
                                ++move_count;
                                cursor.holding_cards = false;
                        } else if (move_res < 0) {
                                return __LINE__;
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
                                return __LINE__;
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
                                        cursor.pos = CURSOR_WASTE_POS;
                                }
                        }
                }
        }
        if (new_keys[IKIdx_Window]) {
                int8_t move_res = KlondikeGame_draw_stock(&game);
                if (move_res == 1) {
                        // ++move_count;
                } else if (move_res < 0) {
                        return __LINE__;
                }
        }
        if (new_keys[IKIdx_Left] && !new_keys[IKIdx_Right]) {
                if (0 < cursor.pos)
                        --cursor.pos;
                else
                        cursor.pos = CURSOR_STOCK_POS;
        } else if (!new_keys[IKIdx_Left] && new_keys[IKIdx_Right]) {
                if (cursor.pos < CURSOR_STOCK_POS)
                        ++cursor.pos;
                else
                        cursor.pos = 0;
        }
        return 0;
}
