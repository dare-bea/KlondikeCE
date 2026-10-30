#include "klondike.h"
#include "klondike_serial.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

uintmax_t game_secs = 0;
clock_t game_start = 0;

int errno = 0;
static int errline = 0;

// Implemented in ./main_step.c
extern int step(void);

// Implemented in ./main_draw.c
extern int draw(void);

// Implemented in ./main_error.c
extern int error_screen(int errno, int errline);

#pragma region main
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

        bool loaded_save_data = false;
        if (!kb_IsDown(kb_KeyPrgm)) {
                FILE *save = fopen("KLONDIKE", "r");
                if (save) {
                        char buf[11] = {0};
                        if (fread(buf, sizeof(char), 10, save) != 10) {
                                // errno = 7, errline = __LINE__;
                                goto open_save_error;
                        }
                        if (strncmp(buf, "KLONDIKE\n", 9)) {
                                // errno = 7, errline = __LINE__;
                                goto open_save_error;
                        }
                        switch (buf[9]) {
                                case 0:
                                        break;
                                case 1:
                                        if (!KlondikeGame_fread(&game, save)) {
                                                errno = 5, errline = __LINE__;
                                        } else {
                                                loaded_save_data = true;
                                        }
                                        break;
                                default:
                                        errno = 7, errline = __LINE__;
                                        break;
                        }
                open_save_error:;
                        if (fclose(save) == EOF) {
                                errno = 6, errline = __LINE__;
                        }
                }
                if (!KlondikeGame_invariants_hold(&game)) {
                        remove("KLONDIKE");
                        errno = 4, errline = __LINE__;
                }
        }
        if (!loaded_save_data && !errno) {
                if (!KlondikeGame_init(&game)) {
                        errno = 1, errline = __LINE__;
                }
        }

        if (!errno) {
                if (KlondikeGame_is_won(&game)) {
                        game_status = GAME_WON;
                } else if (!KlondikeGame_can_make_move(&game)) {
                        game_status = GAME_LOCKED;
                } else if (KlondikeGame_is_lost(&game)) {
                        game_status = GAME_LOST;
                }
        }

        game_start = clock();
        bool save_and_quit = false;
        while (!errno && !kb_On) {
                if (kb_IsDown(kb_KeyZoom) && game_status != GAME_WON) {
                        save_and_quit = true;
                        break;
                }
                errline = step();
                if (errline) {
                        errno = errno ? errno : 2;
                        if (errno == 4)
                                draw();
                        break;
                }
                errline = draw();
                if (errline) {
                        errno = errno ? errno : 3;
                        break;
                }
        }
        if (save_and_quit && !errno) {
                game.timer += clock() - game_start;
                FILE *save = fopen("KLONDIKE", "w");
                if (!save) {
                        errno = 5, errline = __LINE__;
                } else {
                        if (fwrite("KLONDIKE\n\1", sizeof(char), 10, save) != 10) {
                                errno = 5, errline = __LINE__;
                                goto save_and_quit_error;
                        }
                        if (!KlondikeGame_fwrite(&game, save)) {
                                errno = 5, errline = __LINE__;
                                goto save_and_quit_error;
                        }
                save_and_quit_error:;
                        if (fclose(save) == EOF) {
                                errno = 6, errline = errline ? errline : __LINE__;
                        }
                }
        } else if (!errno) {
                FILE *save = fopen("KLONDIKE", "w");
                if (!save) {
                        errno = 5, errline = __LINE__;
                } else {
                        if (fwrite("KLONDIKE\n\0", sizeof(char), 10, save) != 10) {
                                errno = 5, errline = __LINE__;
                        }
                        if (fclose(save) == EOF) {
                                errno = 6, errline = __LINE__;
                        }
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
