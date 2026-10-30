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

#define CURSOR_WASTE_POS (TABLEAU_PILES)
#define CURSOR_STOCK_POS (TABLEAU_PILES + 1)

uintmax_t game_secs = 0;
clock_t game_start = 0;

int main_errno = 0;
static int main_errline = 0;

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
                                // main_errno = 7, errline = __LINE__;
                                goto open_save_error;
                        }
                        if (strncmp(buf, "KLONDIKE\n", 9)) {
                                // main_errno = 7, errline = __LINE__;
                                goto open_save_error;
                        }
                        switch (buf[9]) {
                                case 0:
                                        break;
                                case 1:
                                        if (!KlondikeGame_fread(&game, save)) {
                                                main_errno = 5, main_errline = __LINE__;
                                        } else {
                                                loaded_save_data = true;
                                        }
                                        break;
                                default:
                                        main_errno = 7, main_errline = __LINE__;
                                        break;
                        }
                        if (main_errno) {
                                remove("KLONDIKE");
                        } if (!KlondikeGame_invariants_hold(&game)) {
                                remove("KLONDIKE");
                                main_errno = 4, main_errline = __LINE__;
                        }
                open_save_error:;
                        if (fclose(save) == EOF) {
                                main_errno = main_errno ? main_errno : 6, main_errline = __LINE__;
                        }
                }
        }
        if (!loaded_save_data && !main_errno) {
                if (!KlondikeGame_init(&game)) {
                        main_errno = 1, main_errline = __LINE__;
                }
        }

        if (!main_errno)
                KlondikeGame_check_status(&game);

        game_start = clock();
        game_secs = (uintmax_t)(game.timer / CLOCKS_PER_SEC);
        bool save_and_quit = false;
        while (!main_errno && !kb_On) {
                if (kb_IsDown(kb_KeyZoom)) {
                        save_and_quit = true;
                        break;
                }
                main_errline = step();
                if (main_errline) {
                        main_errno = main_errno ? main_errno : 2;
                        if (main_errno == 4)
                                draw();
                        break;
                }
                main_errline = draw();
                if (main_errline) {
                        main_errno = main_errno ? main_errno : 3;
                        break;
                }
        }
        if (save_and_quit && !main_errno) {
                game.timer += clock() - game_start;
                FILE *save = fopen("KLONDIKE", "w");
                if (!save) {
                        main_errno = 5, main_errline = __LINE__;
                } else {
                        if (fwrite("KLONDIKE\n\1", sizeof(char), 10, save) != 10) {
                                main_errno = 5, main_errline = __LINE__;
                                goto save_and_quit_error;
                        }
                        if (!KlondikeGame_fwrite(&game, save)) {
                                main_errno = 5, main_errline = __LINE__;
                                goto save_and_quit_error;
                        }
                save_and_quit_error:;
                        if (fclose(save) == EOF) {
                                main_errno = 6, main_errline = main_errline ? main_errline : __LINE__;
                        }
                }
        } else if (!main_errno) {
                FILE *save = fopen("KLONDIKE", "w");
                if (!save) {
                        main_errno = 5, main_errline = __LINE__;
                } else {
                        if (fwrite("KLONDIKE\n\0", sizeof(char), 10, save) != 10) {
                                main_errno = 5, main_errline = __LINE__;
                        }
                        if (fclose(save) == EOF) {
                                main_errno = 6, main_errline = __LINE__;
                        }
                }
        }

        if (main_errno) {
                error_screen(main_errno, main_errline);
        }

        if (!main_errno || main_errno > 1) {
                KlondikeGame_free(&game);
        }

        gfx_End();
        while (kb_AnyKey()) {
                // Wait for key to be released
        }
        return main_errno ? 1 : 0;
}
