#include "klondike.h"

#include <assert.h>
#include <stdlib.h>
#include <debug.h>

static inline bool ranks_match(enum CardRank hi, enum CardRank lo) {
        return (   (hi == CardRank_KING  && lo == CardRank_QUEEN)
                || (hi == CardRank_QUEEN && lo == CardRank_JACK )
                || (hi == CardRank_JACK  && lo == CardRank_TEN  )
                || (hi == CardRank_TEN   && lo == CardRank_NINE )
                || (hi == CardRank_NINE  && lo == CardRank_EIGHT)
                || (hi == CardRank_EIGHT && lo == CardRank_SEVEN)
                || (hi == CardRank_SEVEN && lo == CardRank_SIX  )
                || (hi == CardRank_SIX   && lo == CardRank_FIVE )
                || (hi == CardRank_FIVE  && lo == CardRank_FOUR )
                || (hi == CardRank_FOUR  && lo == CardRank_THREE)
                || (hi == CardRank_THREE && lo == CardRank_TWO  )
                || (hi == CardRank_TWO   && lo == CardRank_ACE  ));
}

static inline bool suits_match(enum CardSuit hi, enum CardSuit lo) {
        return (   (hi == CardSuit_SPADE   && lo == CardSuit_HEART  )
                || (hi == CardSuit_SPADE   && lo == CardSuit_DIAMOND)
                || (hi == CardSuit_HEART   && lo == CardSuit_SPADE  )
                || (hi == CardSuit_HEART   && lo == CardSuit_CLUB   )
                || (hi == CardSuit_CLUB    && lo == CardSuit_HEART  )
                || (hi == CardSuit_CLUB    && lo == CardSuit_DIAMOND)
                || (hi == CardSuit_DIAMOND && lo == CardSuit_SPADE  )
                || (hi == CardSuit_DIAMOND && lo == CardSuit_CLUB   ));
}
static inline bool matches_t(struct Card const *from_card, struct Card const *to_card, bool empties_pile) {
        if (!from_card)
                return false;
        if (!to_card)
                return from_card->rank == CardRank_KING && !empties_pile;
        return (ranks_match(to_card->rank, from_card->rank)
                && suits_match(to_card->suit, from_card->suit));
}

static inline bool matches_f(struct Card const *from_card, bool has_cards, enum CardRank top_rank) {
        if (!from_card)
                return false;
        if (!has_cards)
                return from_card->rank == CardRank_ACE;
        return ranks_match(from_card->rank, top_rank);
}

void KlondikeGame_free(struct KlondikeGame *game) {
        if (!game) return;
        for (size_t i = 0; i < TABLEAU_PILES; ++i) {
                game->tableau_down[i] = 0;
                Vector_structCard_free(&game->tableau_piles[i]);
        }
        for (size_t i = 0; i < CARDSUIT_COUNT; ++i) {
                game->foundations[i].has_cards = false;
        }
        Vector_structCard_free(&game->stock_waste);
        game->stock_remaining = 0;
}

bool _KlondikeGame_attempt_init(struct KlondikeGame *game) {
        if (!game) return false;
        KlondikeGame_free(game);
        if (!Vector_structCard_reserve_exact(&game->stock_waste, CARDRANK_COUNT * CARDSUIT_COUNT))
                return false;
        for (enum CardRank r = 0; r < CARDRANK_COUNT; ++r)
        for (enum CardSuit s = 0; s < CARDSUIT_COUNT; ++s) {
                if (!Vector_structCard_push(&game->stock_waste,
                    (struct Card){
                    .rank = r, .suit = s,
                    }))
                        return false;
        }
        for (size_t i = 1; i < game->stock_waste.length; ++i) {
                size_t j = i == SIZE_MAX ? rand() : rand() % (i + 1);
                struct Card *ip = Vector_structCard_at(&game->stock_waste, i);
                struct Card *jp = Vector_structCard_at(&game->stock_waste, j);
                if (!ip || !jp)
                        return false;
                struct Card tmp = *ip;
                *ip = *jp;
                *jp = tmp;
        }
        for (size_t i = 0; i < TABLEAU_PILES; ++i) {
                if (!Vector_structCard_reserve_exact(&game->tableau_piles[i], 8))
                        return false;
                if (0 <= i) for (size_t j = 0;; ++j) {
                        struct Card tmp;
                        if (!Vector_structCard_pop(&game->stock_waste, &tmp))
                                return false;
                        if (!Vector_structCard_push(&game->tableau_piles[i], tmp))
                                return false;
                        if (j >= i) break;
                }
                game->tableau_down[i] = 0 < game->tableau_piles[i].length
                        ? game->tableau_piles[i].length - 1 : 0;
        }
        game->stock_remaining = game->stock_waste.length;
        return true;
}

bool KlondikeGame_init(struct KlondikeGame *game) {
        if (_KlondikeGame_attempt_init(game))
                return true;
        KlondikeGame_free(game);
        return false;
}

__attribute__((nonnull(1)))
static inline bool _KlondikeGame_can_move_xt(struct KlondikeGame const *game,
        struct Card const *from_card, size_t to_pile, bool empties_pile) {
        if (TABLEAU_PILES <= to_pile)
                return false;
        struct Card const *to_card = Vector_structCard_last_const(
                &game->tableau_piles[to_pile]);
        return matches_t(from_card, to_card, empties_pile);
}

__attribute__((nonnull(1)))
static inline bool _KlondikeGame_can_move_xf(struct KlondikeGame const *game, struct Card const *from_card) {
        return matches_f(from_card, game->foundations[from_card->suit].has_cards,
                game->foundations[from_card->suit].top_rank);
}

bool KlondikeGame_invariants_hold(struct KlondikeGame const *game) {
        if (!game)
                return false;
        if (game->stock_waste.length < game->stock_remaining)
                return false;
        for (size_t c = 0; c < TABLEAU_PILES; ++c) {
                if (game->tableau_piles[c].length == 0) {
                        if (game->tableau_down[c] != 0)
                                return false;
                        continue;
                }
                if (game->tableau_piles[c].length <= game->tableau_down[c])
                        return false;
                for (size_t r = game->tableau_down[c]; r < game->tableau_piles[c].length - 1; ++r) {
                        struct Card const *prev = Vector_structCard_at_const(&game->tableau_piles[c], r);
                        struct Card const *next = Vector_structCard_at_const(&game->tableau_piles[c], r + 1);
                        assert(prev && next);
                        if (!ranks_match(prev->rank, next->rank)
                            || !suits_match(prev->suit, next->suit))
                                return false;
                }
        }
        return true;
}

__attribute__((nonnull(1)))
bool KlondikeGame_is_won(struct KlondikeGame const *game) {
        for (enum CardSuit suit = 0; suit < CARDSUIT_COUNT; ++suit) {
                if (!game->foundations[suit].has_cards
                    || game->foundations[suit].top_rank != CardRank_KING) {
                        return false;
                }
        }
        return true;
}

__attribute__((nonnull(1)))
bool KlondikeGame_is_lost(struct KlondikeGame const *game) {
        assert(KlondikeGame_invariants_hold(game));
        /**
         * Count any moves, except:
         * [x] TT moves that make tableau empty (without available King)
         * [x] TF moves that make tableau empty (without available King or next foundation)
         *     [x] Unless all cards in the foundation are Queen or King
         * [x] WT moves without available connections
         * [x] WF moves without available next foundation
         */
        enum CardUses {
                CardUses_NONE = 0,
                CardUses_TABLEAU_EMPTIES = 1,
                CardUses_TABLEAU_REVEALS = 2,
                CardUses_TABLEAU_TOP = 4,
                CardUses_STOCKWASTE_AVAILABLE = 8,
                CardUses_CAN_FOUNDATION = 16,
                CardUses_FOUNDATION_TOP = 32,
                CardUses_FOUNDATION_BELOW = 64,
        };
        enum CardUses card_uses[CARDRANK_COUNT][CARDSUIT_COUNT] = {0};
        size_t const waste_start = game->stock_remaining % game->stock_waste.length;

        /* SETUP CARD USES */
        bool all_royal_foundations = true;
        for (enum CardSuit suit = 0; suit < CARDSUIT_COUNT; ++suit) {
                if (!game->foundations[suit].has_cards)
                        continue;
                card_uses[game->foundations[suit].top_rank][suit] |= CardUses_FOUNDATION_TOP;
                for (enum CardRank rank = 0; rank < game->foundations[suit].top_rank; ++rank)
                        card_uses[rank][suit] |= CardUses_FOUNDATION_BELOW;
                if (game->foundations[suit].top_rank < CardRank_QUEEN)
                        all_royal_foundations = false;
        }
        if (game->stock_waste.length) for (size_t w = waste_start;;) {
                struct Card const *card = Vector_structCard_at_const(
                        &game->stock_waste, w);
                if (card)
                        card_uses[card->rank][card->suit] |= CardUses_STOCKWASTE_AVAILABLE;
                w = (w + STOCK_DRAWN_CARDS) % game->stock_waste.length;
                if (w == waste_start)
                        break;
        }
        for (size_t t = 0; t < TABLEAU_PILES; ++t) {
                if (!game->tableau_piles[t].length)
                        continue;
                struct Card const *bot = Vector_structCard_at_const(
                        &game->tableau_piles[t], game->tableau_down[t]);
                if (bot)
                        card_uses[bot->rank][bot->suit] |= game->tableau_down[t]
                                ? CardUses_TABLEAU_REVEALS : CardUses_TABLEAU_EMPTIES;
                struct Card const *top = Vector_structCard_last_const(
                        &game->tableau_piles[t]);
                if (top)
                        card_uses[top->rank][top->suit] |= CardUses_TABLEAU_TOP;
        }
        bool king_for_empty_tableau = false;
        for (enum CardSuit suit = 0; suit < CARDSUIT_COUNT; ++suit) {
                if (card_uses[CardRank_KING][suit]
                    & (CardUses_TABLEAU_REVEALS | CardUses_STOCKWASTE_AVAILABLE)) {
                        king_for_empty_tableau = true;
                        break;
                }
        }
        dbg_printf("---\n");
        dbg_printf("Free King %d | Royal Foundations %d\n", king_for_empty_tableau, all_royal_foundations);
        for (enum CardSuit suit = 0; suit < CARDSUIT_COUNT; ++suit) {
                dbg_printf("USES %c |", "shcd"[suit]);
                for (enum CardRank rank = 0; rank < CARDRANK_COUNT; ++rank) {
                        dbg_printf(" (%c)%c%c%c%c%c%c%c",
                                "A23456789TJQK"[rank],
                                (card_uses[rank][suit] & CardUses_TABLEAU_EMPTIES) ? 'E' : '.',
                                (card_uses[rank][suit] & CardUses_TABLEAU_REVEALS) ? 'R' : '.',
                                (card_uses[rank][suit] & CardUses_TABLEAU_TOP) ? 'T' : '.',
                                (card_uses[rank][suit] & CardUses_STOCKWASTE_AVAILABLE) ? 's' : '.',
                                (card_uses[rank][suit] & CardUses_CAN_FOUNDATION) ? 'f' : '.',
                                (card_uses[rank][suit] & CardUses_FOUNDATION_TOP) ? 'l' : '.',
                                (card_uses[rank][suit] & CardUses_FOUNDATION_BELOW) ? 'b' : '.');
                }
                dbg_printf("\n");
        }

        /* CHECK TT MOVES */
        for (size_t t = 0; t < TABLEAU_PILES; ++t) {
                dbg_printf("TT %zu [d=%zu, n=%zu] | ", t, game->tableau_down[t], game->tableau_piles[t].length);
                if (!game->tableau_down[t] && !king_for_empty_tableau) {
                        dbg_printf("NDNK\n");
                        continue;
                }
                struct Card const *bot = Vector_structCard_at_const(
                        &game->tableau_piles[t], game->tableau_down[t]);
                if (!bot) {
                        dbg_printf("bot=NULL\n");
                        continue;
                }
                dbg_printf("bot=%c%c | ", "A23456789TJQK"[bot->rank], "shcd"[bot->suit]);
                for (size_t t2 = 0; t2 < TABLEAU_PILES; ++t2) {
                        dbg_printf(".");
                        if (_KlondikeGame_can_move_xt(game, bot, t2,
                                    game->tableau_down[t])) {
                                dbg_printf("\nFALSE (TT)\n");
                                return false;
                        }
                }
                dbg_printf("\n");
        }

        /* CHECK TF MOVES */
        for (size_t t = 0; t < TABLEAU_PILES; ++t) {
                dbg_printf("TF %zu [d=%zu, n=%zu] | top=", t, game->tableau_down[t], game->tableau_piles[t].length);
                struct Card const *top = Vector_structCard_last_const(
                        &game->tableau_piles[t]);
                if (!top) {
                        dbg_printf("NULL\n");
                        continue;
                }
                dbg_printf("%c%c\n", "A23456789TJQK"[top->rank], "shcd"[top->suit]);
                if (game->tableau_piles[t].length <= 1
                    && !game->tableau_down[t]
                    && !king_for_empty_tableau
                    && !all_royal_foundations
                    && (top->rank == CardRank_KING
                        || !(card_uses[top->rank + 1][top->suit]
                             & (CardUses_TABLEAU_TOP | CardUses_STOCKWASTE_AVAILABLE))))
                        continue;
                if (_KlondikeGame_can_move_xf(game, top)) {
                        dbg_printf("FALSE (TF)\n");
                        return false;
                }
        }

        /* CHECK WT, WF MOVES */

        if (game->stock_waste.length) for (size_t w1 = waste_start;;) {
                dbg_printf("WX %zu [c=%zu, n=%zu] | card=", w1, game->stock_remaining, game->stock_waste.length);
                struct Card const *w1_card = Vector_structCard_at_const(&game->stock_waste, w1);
                if (!w1_card) {
                        dbg_printf("NULL\n");
                        goto w1_continue;
                }
                dbg_printf("%c%c\n", "A23456789TJQK"[w1_card->rank], "shcd"[w1_card->suit]);
                size_t w2_start = w1 % (game->stock_waste.length - 1);
                for (size_t t1 = 0; t1 < TABLEAU_PILES; ++t1) {
                        if (!_KlondikeGame_can_move_xt(game, w1_card, t1, false))
                                continue;
                        dbg_printf("  WT %zu | T ", t1);
                        for (size_t t2 = 0; t2 < TABLEAU_PILES; ++t2) {
                                dbg_printf(".");
                                if (t1 == t2)
                                        continue;
                                struct Card const *t2_card = Vector_structCard_at_const(&game->tableau_piles[t2], game->tableau_down[t2]);
                                if (!t2_card)
                                        continue;
                                if (matches_t(t2_card, w1_card, !game->tableau_down[t2])) {
                                        dbg_printf("\nFALSE (WT:TT)\n");
                                        return false;
                                }
                        }
                        dbg_printf(" | W ");
                        if (game->stock_waste.length > 1) for (size_t w2 = w2_start;;) {
                                size_t w2_index = w2 >= w1 ? w2 + 1 : w2;
                                dbg_printf("[%zu:", w2_index);
                                struct Card const *w2_card = Vector_structCard_at_const(&game->stock_waste, w2_index);
                                if (!w2_card) {
                                        dbg_printf("NULL]");
                                        goto w1t_w2_continue;
                                }
                                dbg_printf("%c%c:", "A23456789TJQK"[w2_card->rank], "shcd"[w2_card->suit]);
                                for (size_t t2 = 0; t2 < TABLEAU_PILES; ++t2) {
                                        dbg_printf(".");
                                        if (t1 == t2
                                            ? matches_t(w2_card, w1_card, false)
                                            : _KlondikeGame_can_move_xt(game, w2_card, t2, false)) {
                                                dbg_printf("]\nFALSE (WT:WT)\n");
                                                return false;
                                        }
                                }
                                dbg_printf("f]");
                                if (_KlondikeGame_can_move_xf(game, w2_card)) {
                                        dbg_printf("\nFALSE (WT:WF)\n");
                                        return false;
                                }
                        w1t_w2_continue:;
                                w2 = (w2 + STOCK_DRAWN_CARDS) % (game->stock_waste.length - 1);
                                if (w2 == w2_start)
                                        break;
                        }
                        dbg_printf("\n");
                }
                dbg_printf("  WF | T ");
                if (_KlondikeGame_can_move_xf(game, w1_card)) {
                        for (size_t t2 = 0; t2 < TABLEAU_PILES; ++t2) {
                                dbg_printf(".");
                                struct Card const *t2_card = Vector_structCard_last_const(&game->tableau_piles[t2]);
                                if (!t2_card || t2_card->suit != w1_card->suit)
                                        continue;
                                if (matches_f(t2_card, true, w1_card->rank)) {
                                        dbg_printf("\nFALSE (WF:TF)\n");
                                        return false;
                                }
                        }
                        dbg_printf(" | W ");
                        if (game->stock_waste.length > 1) for (size_t w2 = w2_start;;) {
                                size_t w2_index = w2 >= w1 ? w2 + 1 : w2;
                                dbg_printf("[%zu:", w2_index);
                                struct Card const *w2_card = Vector_structCard_at_const(&game->stock_waste, w2_index);
                                if (!w2_card) {
                                        dbg_printf("NULL]");
                                        goto w1f_w2_continue;
                                }
                                dbg_printf("%c%c:", "A23456789TJQK"[w2_card->rank], "shcd"[w2_card->suit]);
                                for (size_t t2 = 0; t2 < TABLEAU_PILES; ++t2) {
                                        dbg_printf(".");
                                        if (_KlondikeGame_can_move_xt(game, w2_card, t2, false)) {
                                                dbg_printf("]\nFALSE (WF:WT)\n");
                                                return false;
                                        }
                                }
                                dbg_printf("f]");
                                if (w2_card->suit == w1_card->suit
                                    ? matches_f(w2_card, true, w1_card->rank)
                                    : _KlondikeGame_can_move_xf(game, w2_card)) {
                                        dbg_printf("\nFALSE (WF:WF)\n");
                                        return false;
                                }
                        w1f_w2_continue:;
                                w2 = (w2 + STOCK_DRAWN_CARDS) % (game->stock_waste.length - 1);
                                if (w2 == w2_start)
                                        break;
                        }
                        dbg_printf("\n");
                }
        w1_continue:;
                w1 = (w1 + STOCK_DRAWN_CARDS) % game->stock_waste.length;
                if (w1 == waste_start)
                        break;
        }
        dbg_printf("TRUE\n");
        return true;
}

__attribute__((nonnull(1)))
bool KlondikeGame_can_make_move(struct KlondikeGame const *game) {
        size_t const waste_start = game->stock_remaining == game->stock_waste.length
                ? 0 : game->stock_remaining;
        for (size_t ta = 0; ta < TABLEAU_PILES; ++ta) {
                if (KlondikeGame_can_move_tf(game, ta))
                        return true;
                if (game->stock_waste.length) { // prevent divide by zero
                        for (size_t w = waste_start;;) {
                                if (_KlondikeGame_can_move_xt(game,
                                Vector_structCard_at_const(&game->stock_waste, w),
                                ta, false))
                                        return true;
                                w = (w + STOCK_DRAWN_CARDS) % game->stock_waste.length;
                                if (w == waste_start)
                                        break;
                        }
                }
                for (size_t tb = 0; tb < TABLEAU_PILES; ++tb) {
                        if (KlondikeGame_can_move_tt(game, ta, tb))
                                return true;
                }
        }
        if (game->stock_waste.length) {
                for (size_t w = waste_start;;) {
                        if (_KlondikeGame_can_move_xf(game,
                        Vector_structCard_at_const(&game->stock_waste, w)))
                                return true;
                        w = (w + STOCK_DRAWN_CARDS) % game->stock_waste.length;
                        if (w == waste_start)
                                break;
                }
        }
        return false;
}

__attribute__((nonnull(1)))
bool KlondikeGame_can_draw_stock(struct KlondikeGame const *game, size_t count) {
        (void)count;
        assert(KlondikeGame_invariants_hold(game));
        return game->stock_waste.length;
}

__attribute__((nonnull(1)))
bool KlondikeGame_can_move_tt(struct KlondikeGame const *game, size_t from_pile, size_t to_pile) {
        if (TABLEAU_PILES <= from_pile || TABLEAU_PILES <= to_pile || from_pile == to_pile)
                return false;
        assert(KlondikeGame_invariants_hold(game));
        struct Card const *from_card = Vector_structCard_at_const(&game->tableau_piles[from_pile],
                game->tableau_down[from_pile]);
        return _KlondikeGame_can_move_xt(game, from_card, to_pile, !game->tableau_down[from_pile]);
}

__attribute__((nonnull(1)))
bool KlondikeGame_can_move_wt(struct KlondikeGame const *game, size_t to_pile) {
        if (TABLEAU_PILES <= to_pile)
                return false;
        assert(KlondikeGame_invariants_hold(game));
        struct Card const *from_card = Vector_structCard_at_const(&game->stock_waste,
                game->stock_remaining);
        return _KlondikeGame_can_move_xt(game, from_card, to_pile, false);
}

__attribute__((nonnull(1)))
bool KlondikeGame_can_move_tf(struct KlondikeGame const *game, size_t from_pile) {
        if (TABLEAU_PILES <= from_pile)
                return false;
        assert(KlondikeGame_invariants_hold(game));
        struct Card const *from_card = Vector_structCard_last_const(&game->tableau_piles[from_pile]);
        return _KlondikeGame_can_move_xf(game, from_card);
}

__attribute__((nonnull(1)))
bool KlondikeGame_can_move_wf(struct KlondikeGame const *game) {
        assert(KlondikeGame_invariants_hold(game));
        struct Card const *from_card = Vector_structCard_at_const(&game->stock_waste,
                game->stock_remaining);
        return _KlondikeGame_can_move_xf(game, from_card);
}

__attribute__((nonnull(1)))
int8_t KlondikeGame_draw_stock(struct KlondikeGame *game, size_t count) {
        if (!KlondikeGame_can_draw_stock(game, count))
                return 0;
        for (size_t i = 0; i < count; ++i)
                game->stock_remaining = game->stock_remaining
                        ? game->stock_remaining - 1
                        : game->stock_waste.length - 1;
        return 1;
}

__attribute__((nonnull(1)))
int8_t KlondikeGame_move_tt(struct KlondikeGame *game, size_t from_pile, size_t to_pile) {
        if (!KlondikeGame_can_move_tt(game, from_pile, to_pile))
                return 0;
        if (!Vector_structCard_extend(&game->tableau_piles[to_pile],
                Vector_structCard_at(&game->tableau_piles[from_pile],
                        game->tableau_down[from_pile]),
                game->tableau_piles[from_pile].length - game->tableau_down[from_pile]))
                return -1;
        game->tableau_piles[from_pile].length = game->tableau_down[from_pile];
        if (game->tableau_down[from_pile]) {
                --game->tableau_down[from_pile];
        }
        return 1;
}

__attribute__((nonnull(1)))
int8_t KlondikeGame_move_wt(struct KlondikeGame *game, size_t to_pile) {
        if (!KlondikeGame_can_move_wt(game, to_pile))
                return 0;
        struct Card from_card;
        if (!Vector_structCard_remove(&game->stock_waste, game->stock_remaining, &from_card))
                return -1;
        if (!Vector_structCard_push(&game->tableau_piles[to_pile], from_card))
                return -1;
        return 1;
}

__attribute__((nonnull(1)))
int8_t KlondikeGame_move_tf(struct KlondikeGame *game, size_t from_pile) {
        if (!KlondikeGame_can_move_tf(game, from_pile))
                return 0;
        struct Card from_card;
        if (!Vector_structCard_pop(&game->tableau_piles[from_pile], &from_card))
                return -1;
        game->foundations[from_card.suit].has_cards = true;
        game->foundations[from_card.suit].top_rank = from_card.rank;
        if (game->tableau_down[from_pile]
            && game->tableau_down[from_pile] == game->tableau_piles[from_pile].length) {
                --game->tableau_down[from_pile];
        }
        return 1;
}

__attribute__((nonnull(1)))
int8_t KlondikeGame_move_wf(struct KlondikeGame *game) {
        if (!KlondikeGame_can_move_wf(game))
                return 0;
        struct Card from_card;
        if (!Vector_structCard_remove(&game->stock_waste, game->stock_remaining, &from_card))
                return -1;
        game->foundations[from_card.suit].has_cards = true;
        game->foundations[from_card.suit].top_rank = from_card.rank;
        return 1;
}
