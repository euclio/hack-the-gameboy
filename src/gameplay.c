#include <gb/gb.h>
#include <rand.h>

#include "tiles/fallcats.c"
#include "tiles/blank.c"
#include "tiles/stackcats.c"
#include "tiles/background.c"
#include "tiles/bgtiles.c"

/*
 * Tile IDs for each cat sprite.
 */
typedef enum {
    BLANK       = 0x00,
    STRIPED_CAT = 0x04,
    BLACK_CAT   = 0x08,
    FALLING_CAT = 0x0C,
    SIAMESE_CAT = 0x10
} sprite_t;

/*
 * Function prototypes
 */
void setBuckets(UWORD);
void start_row();
void shift_rows();
UBYTE get_cat_tile(UBYTE);
UBYTE pickCat();
void change_cat(UBYTE, UBYTE);
void draw_cat(UBYTE, UBYTE, UBYTE);

/*
 * Constants useful for drawing sprites. Note that SDCC cannot determine
 * constant expressions correctly (!) so we have to do some math to determine
 * what the correct values should be.
 *
 * It is possible to draw sprites off the screen, so the first pixels that show
 * the entire sprite are given by X_START and Y_START. Similarly, the last
 * pixels that a sprite may be drawn at for it to be on the entire screen are
 * given by X_END and Y_END.
 */
#define X_START         8
#define X_END           168
#define Y_START         16
#define Y_END           160

/*
 * Margin gives the spacing between the edges of the screen and where the rows
 * or columns should start. Padding is the spacing between rows or columns.
 */
#define COLUMN_PADDING  8
#define COLUMN_MARGIN   40              /* X_START + 4 * COLUMN_PADDING */
#define ROW_PADDING     8
#define ROW_MARGIN      24              /* Y_START + ROW_PADDING */

/*
 * Room full of cats uses 8x16 sprites, but the cats themselves are represented
 * by 2 8x16 (left and right halves) to make a single 16x16 sprites. "Cat"
 * refers to the 16x16 sprite, while "sprite" refers to the 8x16 tile used by
 * GBDK.
 */
#define SPRITE_WIDTH    8
#define SPRITE_HEIGHT   16
#define CAT_WIDTH       16              /* SPRITE_WIDTH * 2 */
#define CAT_HEIGHT      16              /* SPRITE_HEIGHT */

/*
 * Constants that describe gameplay elements.
 */
#define NUM_ROWS        4
#define NUM_COLUMNS     4
#define NUM_CATS        16              /* NUM_ROWS * NUM_COLUMNS */

/*
 * Constants that affect gameplay.
 */
#define VBLANK_UPDATE   60              /* Vblanks until gameplay update */

UBYTE sprID;
UWORD buckets[4];
UWORD score[4];

void setBuckets(UWORD colNum) {
    if (sprID == STRIPED_CAT) {
        if (buckets[colNum] == 0x03) {
            score[colNum] = 0x03;
        } else {
            buckets[colNum] = 0x03;
            score[colNum] = 0;
        }
    } else if (sprID == BLACK_CAT) {
        if (buckets[colNum] == 0x04) {
            score[colNum] = 0x04;
        } else {
            buckets[colNum] = 0x04;
            score[colNum] = 0;
        }
    } else if (sprID == FALLING_CAT) {
            if (buckets[colNum] == 0x02) {
                score[colNum] = 0x02;
            } else {
                buckets[colNum] = 0x02;
                score[colNum] = 0;
            }
    } else if (sprID == SIAMESE_CAT) {
        if (buckets[colNum] == 0x01) {
            score[colNum] = 0x01;
        } else {
            buckets[colNum] = 0x01;
            score[colNum] = 0;
        }
    }
}

/*
 * Sets the top row of sprites to be a new row of random cats.
 */
void start_row() {
    UBYTE i;
    for (i = 0; i < NUM_COLUMNS; i++) {
        change_cat(i, pickCat());
    }
}

/*
 * Copies the cat sprites from the row above into each row of cats.
 */
void shift_rows() {
    UBYTE i;

    for (i = NUM_CATS - 1; i >= NUM_COLUMNS; i--) {
        change_cat(i, get_cat_tile(i - NUM_COLUMNS));
    }
}

/*
 * Convenience method to get the tile number of a cat.
 */
UBYTE get_cat_tile(UBYTE nb) {
    return get_sprite_tile(nb * 2);
}

/*
 * Returns a random cat ID.
 */
UBYTE pickCat() {
    UINT8 gen = rand();

    if (gen & 1)
        return STRIPED_CAT;
    else if (gen & 4)
        return BLACK_CAT;
    else if (gen & 16)
        return FALLING_CAT;
    else
        return SIAMESE_CAT;
}

/*
 * Change the sprite tiles used by a cat to new sprite tiles.
 */
void change_cat(UBYTE cat_number, UBYTE sprite_tile) {
    set_sprite_tile(cat_number * 2, sprite_tile);
    set_sprite_tile(cat_number * 2 + 1, sprite_tile + 0x2);
}

/*
 * Convenience function to draw a cat sprite to the screen. Each cat_number
 * actually refers to two 8x16 sprites, but the two sprites are drawn together
 * as a unit.
 */
void draw_cat(UBYTE cat_number, UBYTE x, UBYTE y) {
    move_sprite(cat_number * 2, x, y);
    move_sprite(cat_number * 2 + 1, x + 8, y);
}

/*
 * Initializes the state of the game. Should be called before the program
 * enters the game loop. It ensures that the appropriate registers are
 * initialized for displaying graphics and that the random number generator is
 * seeded.
 */
void init_gameplay() {
    UBYTE i, j;
    UBYTE x_pos, y_pos;
    fixed seed;

    disable_interrupts();
    DISPLAY_OFF;

    LCDC_REG = 0x67;
    /*
     * LCD        = Off
     * WindowBank = 0x9C00-0x9FFF
     * Window     = On
     * BG Chr     = 0x8800-0x97FF
     * BG Bank    = 0x9800-9BFF
     * OBJ        = 8x16
     * OBJ        = On
     * BG         = On
     */

    // Set palettes
    BGP_REG = OBP0_REG = OBP1_REG = 0xE4U;

    // Load sprite tiles
    set_sprite_data(0x00, 0x04, blank16);
    set_sprite_data(0x04, 0x04, cat0);
    set_sprite_data(0x08, 0x04, cat1);
    set_sprite_data(0x0C, 0x04, cat2);
    set_sprite_data(0x10, 0x04, cat3);

    // Create all the sprites and make them blank. 2 for each cat.
    for (i = 0; i < NUM_CATS * 2; i ++) {
        set_sprite_tile(i, BLANK);
    }

    SHOW_SPRITES;

    // Load background tiles
    set_bkg_data(0x00, 0x01, blank8);
    set_bkg_data(0x01, 0x04, faces);

    DISPLAY_ON;
    enable_interrupts();

    // Draw cat sprite locations
    for (i = 0; i < NUM_ROWS; i++) {
        for (j = 0; j < NUM_COLUMNS; j++) {
            x_pos = COLUMN_MARGIN;
            x_pos += j * (COLUMN_PADDING + CAT_WIDTH);

            y_pos = ROW_MARGIN;
            y_pos += i * (ROW_PADDING + CAT_HEIGHT);

            draw_cat(i * 4 + j, x_pos, y_pos);
        }
    }

    // Initialize random number generator with contents of DIV_REG
    seed.b.l = DIV_REG;
    seed.b.h = DIV_REG;
    initrand(seed.w);
}

void do_gameplay() {
    static UBYTE vblanks = 0;
    UBYTE buttons;

    vblanks++;

    buttons = joypad();
    switch(buttons) {
        case(J_LEFT):
            change_cat(8, BLANK);
        break;

        case(J_DOWN):
            change_cat(9, BLANK);
        break;

        case(J_UP):
            change_cat(10, BLANK);
        break;

        case(J_RIGHT):
            change_cat(11, BLANK);
        break;
    }

    if (vblanks > VBLANK_UPDATE) {
        vblanks = 0;

        shift_rows();
        start_row();
    }
}
