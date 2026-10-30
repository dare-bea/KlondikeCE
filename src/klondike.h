#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

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
#define VECTOR_NAME Vector_Card
#include "vector.h"

struct MoveHistory {
        enum MoveHistoryVariant : int8_t {
                MoveHistoryVariant_MOVE_TT = 1,
                MoveHistoryVariant_MOVE_WT,
                MoveHistoryVariant_MOVE_TF,
                MoveHistoryVariant_MOVE_WF,
        } variant;
        size_t stock_remaining;
        union {
                struct {size_t from_pile; size_t to_pile; size_t card_count;} move_tt;
                struct {size_t to_pile;} move_wt;
                struct {size_t from_pile; enum CardSuit suit; bool revealed_card;} move_tf;
                struct {enum CardSuit suit;} move_wf;
        };
};

#define VECTOR_T struct MoveHistory
#define VECTOR_NAME Vector_MoveHistory
#include "vector.h"

enum KlondikeGameStatus {
        KlondikeGameStatus_ONGOING,
        KlondikeGameStatus_WIN,
        KlondikeGameStatus_LOSS,
        KlondikeGameStatus_LOCKOUT,
};

#define TABLEAU_PILES 7
struct KlondikeGame {
        Vector_Card start_deck;
        Vector_MoveHistory move_history;
        Vector_Card tableau_piles[TABLEAU_PILES];
        size_t tableau_down[TABLEAU_PILES];
        Vector_Card stock_waste;
        size_t stock_remaining;
        struct {
                bool has_cards;
                enum CardRank top_rank;
        } foundations[CARDSUIT_COUNT];
        unsigned move_count;
        unsigned undo_count;
        clock_t timer;
        enum KlondikeGameStatus status;
};

#ifndef ONE_CARD_PER_STOCK_DRAW
#define STOCK_DRAWN_CARDS 3
#else
#define STOCK_DRAWN_CARDS 1
#endif


void KlondikeGame_free(struct KlondikeGame *game);
bool KlondikeGame_init(struct KlondikeGame *game);
bool KlondikeGame_invariants_hold(struct KlondikeGame const *game);

__attribute__((nonnull(1)))
bool KlondikeGame_can_draw_stock(struct KlondikeGame const *game, size_t count);
__attribute__((nonnull(1)))
bool KlondikeGame_can_move_tt(struct KlondikeGame const *game, size_t from_pile, size_t to_pile);
__attribute__((nonnull(1)))
bool KlondikeGame_can_move_wt(struct KlondikeGame const *game, size_t to_pile);
__attribute__((nonnull(1)))
bool KlondikeGame_can_move_tf(struct KlondikeGame const *game, size_t from_pile);
__attribute__((nonnull(1)))
bool KlondikeGame_can_move_wf(struct KlondikeGame const *game);

__attribute__((nonnull(1)))
int8_t KlondikeGame_draw_stock(struct KlondikeGame *game, size_t count);
__attribute__((nonnull(1)))
int8_t KlondikeGame_move_tt(struct KlondikeGame *game, size_t from_pile, size_t to_pile);
__attribute__((nonnull(1)))
int8_t KlondikeGame_move_wt(struct KlondikeGame *game, size_t to_pile);
__attribute__((nonnull(1)))
int8_t KlondikeGame_move_tf(struct KlondikeGame *game, size_t from_pile);
__attribute__((nonnull(1)))
int8_t KlondikeGame_move_wf(struct KlondikeGame *game);

__attribute__((nonnull(1)))
int8_t KlondikeGame_undo_move(struct KlondikeGame *game);

__attribute__((nonnull(1)))
bool KlondikeGame_is_won(struct KlondikeGame const *game);
__attribute__((nonnull(1)))
bool KlondikeGame_is_lost(struct KlondikeGame const *game);
__attribute__((nonnull(1)))
bool KlondikeGame_can_make_move(struct KlondikeGame const *game);

__attribute__((nonnull(1)))
static inline enum KlondikeGameStatus KlondikeGame_check_status(struct KlondikeGame *game) {
        enum KlondikeGameStatus status = KlondikeGameStatus_ONGOING;
        if (KlondikeGame_is_won(game)) {
                status = KlondikeGameStatus_WIN;
        } else if (!KlondikeGame_can_make_move(game)) {
                status = KlondikeGameStatus_LOCKOUT;
        } else if (KlondikeGame_is_lost(game)) {
                status = KlondikeGameStatus_LOSS;
        }
        game->status = status;
        return status;
}
