#include "klondike.h"

#include <stdbool.h>
#include <stdio.h>

bool Vector_Card_fread(Vector_Card *restrict this, FILE *restrict stream);

bool Vector_MoveHistory_fread(Vector_MoveHistory *restrict this, FILE *restrict stream);

bool KlondikeGame_fread(struct KlondikeGame *restrict game, FILE *restrict stream);


bool Vector_Card_fwrite(Vector_Card const *restrict this, FILE *restrict stream);

bool Vector_MoveHistory_fwrite(Vector_MoveHistory const *restrict this, FILE *restrict stream);

bool KlondikeGame_fwrite(struct KlondikeGame *restrict game, FILE *restrict stream);
