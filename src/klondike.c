#include "klondike.h"

#include <assert.h>
#include <stdlib.h>

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
static inline bool _KlondikeGame_can_move_xt(struct KlondikeGame *game,
        struct Card *from_card, size_t to_pile, bool empties_pile) {
        if (!from_card || TABLEAU_PILES <= to_pile)
                return false;
        struct Card *to_card = Vector_structCard_last(&game->tableau_piles[to_pile]);
        if (!to_card)
                return from_card->rank == CardRank_KING && !empties_pile;
        return (ranks_match(to_card->rank, from_card->rank)
                && suits_match(to_card->suit, from_card->suit));
}

__attribute__((nonnull(1)))
static inline bool _KlondikeGame_can_move_xf(struct KlondikeGame *game, struct Card *from_card) {
        if (!from_card)
                return false;
        if (!game->foundations[from_card->suit].has_cards)
                return from_card->rank == CardRank_ACE;
        return ranks_match(from_card->rank, game->foundations[from_card->suit].top_rank);
}

bool KlondikeGame_invariants_hold(struct KlondikeGame *game) {
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
                        struct Card *prev = Vector_structCard_at(&game->tableau_piles[c], r);
                        struct Card *next = Vector_structCard_at(&game->tableau_piles[c], r + 1);
                        assert(prev && next);
                        if (!ranks_match(prev->rank, next->rank)
                            || !suits_match(prev->suit, next->suit))
                                return false;
                }
        }
        return true;
}

__attribute__((nonnull(1)))
bool KlondikeGame_is_won(struct KlondikeGame *game) {
        for (enum CardSuit suit = 0; suit < CARDSUIT_COUNT; ++suit) {
                if (!game->foundations[suit].has_cards
                    || game->foundations[suit].top_rank != CardRank_KING) {
                        return false;
                }
        }
        return true;
}

__attribute__((nonnull(1)))
bool KlondikeGame_can_make_move(struct KlondikeGame *game) {
        size_t const waste_start = game->stock_waste.length ? 0 : game->stock_remaining;
        for (size_t ta = 0; ta < TABLEAU_PILES; ++ta) {
                if (KlondikeGame_can_move_tf(game, ta))
                        return true;
                if (game->stock_waste.length) { // prevent divide by zero
                        for (size_t w = waste_start;;) {
                                if (_KlondikeGame_can_move_xt(game,
                                Vector_structCard_at(&game->stock_waste, w),
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
                        Vector_structCard_at(&game->stock_waste, w)))
                                return true;
                        w = (w + STOCK_DRAWN_CARDS) % game->stock_waste.length;
                        if (w == waste_start)
                                break;
                }
        }
        return false;
}

__attribute__((nonnull(1)))
bool KlondikeGame_can_draw_stock(struct KlondikeGame *game, size_t count) {
        (void)count;
        assert(KlondikeGame_invariants_hold(game));
        return game->stock_waste.length;
}

__attribute__((nonnull(1)))
bool KlondikeGame_can_move_tt(struct KlondikeGame *game, size_t from_pile, size_t to_pile) {
        if (TABLEAU_PILES <= from_pile || TABLEAU_PILES <= to_pile || from_pile == to_pile)
                return false;
        assert(KlondikeGame_invariants_hold(game));
        struct Card *from_card = Vector_structCard_at(&game->tableau_piles[from_pile],
                game->tableau_down[from_pile]);
        return _KlondikeGame_can_move_xt(game, from_card, to_pile, !game->tableau_down[from_pile]);
}

__attribute__((nonnull(1)))
bool KlondikeGame_can_move_wt(struct KlondikeGame *game, size_t to_pile) {
        if (TABLEAU_PILES <= to_pile)
                return false;
        assert(KlondikeGame_invariants_hold(game));
        struct Card *from_card = Vector_structCard_at(&game->stock_waste,
                game->stock_remaining);
        return _KlondikeGame_can_move_xt(game, from_card, to_pile, false);
}

__attribute__((nonnull(1)))
bool KlondikeGame_can_move_tf(struct KlondikeGame *game, size_t from_pile) {
        if (TABLEAU_PILES <= from_pile)
                return false;
        assert(KlondikeGame_invariants_hold(game));
        struct Card *from_card = Vector_structCard_last(&game->tableau_piles[from_pile]);
        return _KlondikeGame_can_move_xf(game, from_card);
}

__attribute__((nonnull(1)))
bool KlondikeGame_can_move_wf(struct KlondikeGame *game) {
        assert(KlondikeGame_invariants_hold(game));
        struct Card *from_card = Vector_structCard_at(&game->stock_waste,
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
