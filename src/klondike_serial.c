#include "klondike.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

bool Vector_Card_fread(Vector_Card *restrict this, FILE *restrict stream) {
        size_t length;
        if (fread(&length, sizeof(length), 1, stream) != 1)
                return false;
        if (!Vector_Card_reserve(this, this->length + length))
                return false;
        size_t written = fread(&this->items[this->length], sizeof(struct Card), length, stream);
        this->length += written;
        return written == length;
}

bool Vector_MoveHistory_fread(Vector_MoveHistory *restrict this, FILE *restrict stream) {
        size_t length;
        if (fread(&length, sizeof(length), 1, stream) != 1)
                return false;
        if (!Vector_MoveHistory_reserve(this, this->length + length))
                return false;
        size_t written = fread(&this->items[this->length], sizeof(struct MoveHistory), length, stream);
        this->length += written;
        return written == length;
}

bool KlondikeGame_fread(struct KlondikeGame *restrict game, FILE *restrict stream) {
        KlondikeGame_free(game);
        if (!Vector_Card_fread(&game->start_deck, stream))
                return false;
        if (!Vector_MoveHistory_fread(&game->move_history, stream))
                return false;
        for (size_t i = 0; i < TABLEAU_PILES; ++i)
                if (!Vector_Card_fread(&game->tableau_piles[i], stream))
                        return false;
        if (fread(game->tableau_down, sizeof(*game->tableau_down), TABLEAU_PILES, stream) != TABLEAU_PILES)
                return false;
        if (!Vector_Card_fread(&game->stock_waste, stream))
                return false;
        if (fread(&game->stock_remaining, sizeof(game->stock_remaining), 1, stream) != 1)
                return false;
        if (fread(game->foundations, sizeof(*game->foundations), CARDSUIT_COUNT, stream) != CARDSUIT_COUNT)
                return false;
        if (fread(&game->move_count, sizeof(game->move_count), 1, stream) != 1)
                return false;
        if (fread(&game->undo_count, sizeof(game->undo_count), 1, stream) != 1)
                return false;
        if (fread(&game->timer, sizeof(game->timer), 1, stream) != 1)
                return false;
        return true;
}

////////////////

bool Vector_Card_fwrite(Vector_Card const *restrict this, FILE *restrict stream) {
        if (fwrite(&this->length, sizeof(this->length), 1, stream) != 1)
                return false;
        if (fwrite(this->items, sizeof(struct Card), this->length, stream) != this->length)
                return false;
        return true;
}

bool Vector_MoveHistory_fwrite(Vector_MoveHistory const *restrict this, FILE *restrict stream) {
        if (fwrite(&this->length, sizeof(this->length), 1, stream) != 1)
                return false;
        if (fwrite(this->items, sizeof(struct MoveHistory), this->length, stream) != this->length)
                return false;
        return true;
}

bool KlondikeGame_fwrite(struct KlondikeGame const *restrict game, FILE *restrict stream) {
        if (!Vector_Card_fwrite(&game->start_deck, stream))
                return false;
        if (!Vector_MoveHistory_fwrite(&game->move_history, stream))
                return false;
        for (size_t i = 0; i < TABLEAU_PILES; ++i)
                if (!Vector_Card_fwrite(&game->tableau_piles[i], stream))
                        return false;
        if (fwrite(game->tableau_down, sizeof(*game->tableau_down), TABLEAU_PILES, stream) != TABLEAU_PILES)
                return false;
        if (!Vector_Card_fwrite(&game->stock_waste, stream))
                return false;
        if (fwrite(&game->stock_remaining, sizeof(game->stock_remaining), 1, stream) != 1)
                return false;
        if (fwrite(game->foundations, sizeof(*game->foundations), CARDSUIT_COUNT, stream) != CARDSUIT_COUNT)
                return false;
        if (fwrite(&game->move_count, sizeof(game->move_count), 1, stream) != 1)
                return false;
        if (fwrite(&game->undo_count, sizeof(game->undo_count), 1, stream) != 1)
                return false;
        if (fwrite(&game->timer, sizeof(game->timer), 1, stream) != 1)
                return false;
        return true;
}
