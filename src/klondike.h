#pragma once
#include <stdbool.h>
#include <stdint.h>

enum CardRank : int8_t {
        CardRank_ACE,  CardRank_TWO, CardRank_THREE, CardRank_FOUR,
        CardRank_FIVE, CardRank_SIX, CardRank_SEVEN, CardRank_EIGHT,
        CardRank_NINE, CardRank_TEN, CardRank_JACK,  CardRank_QUEEN,
        CardRank_KING,
};
#define CARDRANK_COUNT 13

enum CardSuit : int8_t {
        CardSuit_SPADE, CardSuit_HEART,
        CardSuit_CLUB,  CardSuit_DIAMOND,
};
#define CARDSUIT_COUNT 4

struct Card {
        enum CardRank rank;
        enum CardSuit suit;
};

#define VECTOR_T struct Card
#define VECTOR_NAME Vector_structCard
#include "vector.h"

#define TABLEAU_PILES 7
struct KlondikeGame {
        size_t tableau_down[TABLEAU_PILES];
        Vector_structCard tableau_piles[TABLEAU_PILES];
        struct {
                bool has_cards;
                enum CardRank top_rank;
        } foundations[CARDSUIT_COUNT];
        Vector_structCard stock_waste;
        size_t stock_remaining;
};

#ifndef ONE_CARD_PER_STOCK_DRAW
#define STOCK_DRAWN_CARDS 3
#else
#define STOCK_DRAWN_CARDS 1
#endif


void KlondikeGame_free(struct KlondikeGame *game);
bool KlondikeGame_init(struct KlondikeGame *game);
bool KlondikeGame_invariants_hold(struct KlondikeGame *game);

__attribute__((nonnull(1)))
bool KlondikeGame_can_draw_stock(struct KlondikeGame *game);
__attribute__((nonnull(1)))
bool KlondikeGame_can_move_tt(struct KlondikeGame *game, size_t from_pile, size_t to_pile);
__attribute__((nonnull(1)))
bool KlondikeGame_can_move_wt(struct KlondikeGame *game, size_t to_pile);
__attribute__((nonnull(1)))
bool KlondikeGame_can_move_tf(struct KlondikeGame *game, size_t from_pile);
__attribute__((nonnull(1)))
bool KlondikeGame_can_move_wf(struct KlondikeGame *game);

__attribute__((nonnull(1)))
int8_t KlondikeGame_draw_stock(struct KlondikeGame *game);
__attribute__((nonnull(1)))
int8_t KlondikeGame_move_tt(struct KlondikeGame *game, size_t from_pile, size_t to_pile);
__attribute__((nonnull(1)))
int8_t KlondikeGame_move_wt(struct KlondikeGame *game, size_t to_pile);
__attribute__((nonnull(1)))
int8_t KlondikeGame_move_tf(struct KlondikeGame *game, size_t from_pile);
__attribute__((nonnull(1)))
int8_t KlondikeGame_move_wf(struct KlondikeGame *game);

__attribute__((nonnull(1)))
bool KlondikeGame_is_won(struct KlondikeGame *game);
__attribute__((nonnull(1)))
bool KlondikeGame_can_make_move(struct KlondikeGame *game);
