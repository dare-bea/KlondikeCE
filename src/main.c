#include "klondike.h"

#include <stdlib.h>
#include <time.h>

#include <graphx.h>
#include "gfx/gfx.h"
#include "gamepalette_colors.h"

#include <keypadc.h>

#include <fontlibc.h>
#include "fonts.h"

#ifdef CRASH_DUMP
#include <stdio.h>
#endif

/* -- COMPILE TIME FLAGS --
 * `CRASH_DUMP` - Dump the game struct when some assertion fails
 * `SHOW_MOVE_AVAILABLITY` - Calculate and show if a move is available from the
 *      hovered tableau/waste pile when you are not holding it.
 * `DEBUG_CURSOR_INFO` - Show cursor debug information
 * `ONE_CARD_PER_STOCK_DRAW` - When drawing from the stock, move one card to the waste instead of three.
 */

struct KlondikeGame game;
struct {
        size_t pos; // pos == CURSOR_WASTE_POS: Waste
        bool holding_cards;
        size_t from_pos; // from_pos == CURSOR_WASTE_POS: Waste
} cursor = {0};
enum {
        GAME_ONGOING,
        GAME_WON,
        GAME_LOST,
        GAME_LOCKED,
} game_status = GAME_ONGOING;

#define CURSOR_WASTE_POS (TABLEAU_PILES)
#define CURSOR_STOCK_POS (TABLEAU_PILES + 1)

unsigned move_count = 0;
uintmax_t game_secs = 0;
clock_t game_start = 0;

static int errno = 0;
static int errline = 0;

// Implemented in ./main_step.c
extern int step(void);

// Implemented in ./main_draw.c
extern int draw(void);

// Implemented in ./main_error.c
extern int error_screen(int errno, int errline);

int main(void)
{
        srand(time(NULL));

        gfx_Begin();
        gfx_SetPalette(gamepalette, sizeof_gamepalette, 0);
        gfx_SetDrawBuffer();
        fontlib_SetNewlineOptions(FONTLIB_ENABLE_AUTO_WRAP);
        fontlib_SetLineSpacing(0, 1);
        kb_SetMode(MODE_3_CONTINUOUS);
        kb_EnableOnLatch();
        kb_ClearOnLatch();

        if (!KlondikeGame_init(&game)) {
                errno = 1, errline = __LINE__;
        }

        game_start = clock();
        while (!errno && !kb_On/* && !kb_IsDown(kb_KeyMode)*/) {
                errline = step();
                if (errline) {
                        errno = errno ? errno : 2;
                        break;
                }
                errline = draw();
                if (errline) {
                        errno = errno ? errno : 3;
                        break;
                }
                if (!KlondikeGame_invariants_hold(&game)) {
                        errno = 4, errline = __LINE__;
                        break;
                }
        }

        if (errno) {
                error_screen(errno, errline);
        }

        if (!errno || errno > 1) {
                KlondikeGame_free(&game);
        }

        gfx_End();
        while (kb_AnyKey()) {
                // Wait for key to be released
        }
        return errno ? 1 : 0;
}
