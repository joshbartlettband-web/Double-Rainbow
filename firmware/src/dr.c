/* SPDX-License-Identifier: GPL-3.0-only
 * Double Rainbow (joshbartlettband-web/Double-Rainbow), built on Melodee. Double Rainbow's own code lives here, kept
 * in as few files as possible so Melodee's updates merge easily (DOUBLE-RAINBOW.md).
 *
 * The startup screen: a double moonbow over dark hills, drawn from (x, y) in two canvas strips (no picture stored).
 * True to the real thing: the bows' centre is below the horizon (the point opposite the moon, which is behind you,
 * so no moon in the sky); the primary bow is red outside, violet inside; the secondary, wider and fainter, has its
 * colours the other way round; the sky is a little brighter inside the primary and darker between the two
 * (Alexander's band). Pixel-art finish: ordered dither between the sky's steps. */

#define DR_RGB(r, g, b) RGB(r, g, b)

/* the bows: centre (DR_CX, DR_CY), each band 2 px wide; radii outward (primary violet .. red, secondary red .. violet) */
#define DR_CX 120
#define DR_CY 212
#define DR_R1 100                 /* the primary's inner edge (violet) */
#define DR_R2 132                 /* the secondary's inner edge (red) */
#define DR_BAND 2
#define DR_HORIZON 152            /* the hills rise above this; the title sits on the flat ground below */
#define DR_GROUND DR_RGB(18, 11, 28)

static const uint8_t DR_BOW[6][3] = {      /* violet, blue, green, yellow, orange, red: inside to outside */
    {160, 112, 230}, {100, 150, 236}, {120, 206, 140}, {240, 214, 120}, {240, 160, 96}, {232, 96, 104},
};
static const uint8_t DR_BAYER[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

static uint32_t dr_hash(uint32_t x, uint32_t y)
{
    uint32_t h = x * 374761393u + y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

/* the hills' top edge at column x: two slow waves, never below DR_HORIZON - 2 (the title's ground stays flat) */
static int32_t dr_hill(int32_t x)
{
    static const int8_t SIN[32] = {0, 2, 3, 5, 6, 7, 8, 8, 8, 8, 8, 7, 6, 5, 3, 2,
                                   0, -2, -3, -5, -6, -7, -8, -8, -8, -8, -8, -7, -6, -5, -3, -2};   /* 8 sin */
    int32_t t = DR_HORIZON - 12 + SIN[(x * 32 / 150) & 31] + SIN[((x + 37) * 32 / 70) & 31] / 2;
    return t > DR_HORIZON - 2 ? DR_HORIZON - 2 : t;
}

static uint16_t dr_mix(const uint8_t *a, uint32_t r, uint32_t g, uint32_t b, uint32_t pct)   /* a over (r g b) */
{
    return DR_RGB((a[0] * pct + r * (100u - pct)) / 100u, (a[1] * pct + g * (100u - pct)) / 100u,
                  (a[2] * pct + b * (100u - pct)) / 100u);
}

static uint16_t dr_pixel(int32_t x, int32_t y)
{
    int32_t dx = x - DR_CX, dy = y - DR_CY, d2 = dx * dx + dy * dy, top = dr_hill(x);
    uint32_t r, g, b, step, t;
    if (y > top) {                                       /* the hills: a moonlit rim, then dark */
        if (y == top + 1)
            return DR_RGB(58, 38, 78);
        if (y > top + 1 && y < DR_HORIZON + 4 && dr_hash((uint32_t)x, (uint32_t)y) % 97u == 0u)
            return DR_RGB(255, 214, 110);                /* a firefly */
        return DR_GROUND;
    }
    /* the sky: 12 steps from deep indigo (top) to plum (horizon), dithered between them */
    t = (uint32_t)(y * 12 * 16 / DR_HORIZON);
    step = t / 16u + ((t & 15u) > DR_BAYER[y & 3][x & 3] ? 1u : 0u);
    r = 14u + step * 3u;
    g = 10u + step * 14u / 10u;
    b = 32u + step * 36u / 10u;
    if (d2 < DR_R1 * DR_R1) {                            /* brighter inside the primary bow */
        r += 6u; g += 5u; b += 8u;
    } else if (d2 >= (DR_R1 + 6 * DR_BAND) * (DR_R1 + 6 * DR_BAND) && d2 < DR_R2 * DR_R2) {
        r -= r > 20u ? 5u : 0u; g -= g > 14u ? 4u : 0u; b -= 6u;    /* Alexander's dark band */
    }
    {
        int32_t k;
        for (k = 0; k < 6; k++) {                        /* the primary: violet (inner) .. red (outer) */
            int32_t a = DR_R1 + k * DR_BAND, z = a + DR_BAND;
            if (d2 >= a * a && d2 < z * z)
                return dr_mix(DR_BOW[k], r, g, b, 78u);
        }
        for (k = 0; k < 6; k++) {                        /* the secondary: red (inner) .. violet (outer), fainter */
            int32_t a = DR_R2 + k * DR_BAND, z = a + DR_BAND;
            if (d2 >= a * a && d2 < z * z)
                return dr_mix(DR_BOW[5 - k], r, g, b, 42u);
        }
    }
    {                                                    /* stars, sparser near the horizon and on the bows' glow */
        uint32_t h = dr_hash((uint32_t)x, (uint32_t)y);
        if (h % (240u + (uint32_t)y * 4u) == 0u)
            return (h >> 9) & 1u ? DR_RGB(243, 233, 198) : DR_RGB(150, 136, 180);
    }
    return DR_RGB(r, g, b);
}

/* the startup screen (main.c, in place of Melodee's), drawn once in two strips */
static void dr_splash(void)
{
    uint32_t half, x, y;
    for (half = 0; half < 2u; half++) {
        uint32_t y0 = half * 120u;
        cv_begin(240, 120, DR_GROUND);
        for (y = 0; y < 120u; y++)
            for (x = 0; x < 240u; x++)
                cv_px[y * 240u + x] = swap16(dr_pixel((int32_t)x, (int32_t)(y0 + y)));
        if (half) {                                      /* the title on the flat ground (rows 152..239) */
            cv_text_c(120, 156 - 120, &AF_L, "DOUBLE", DR_RGB(255, 205, 96), DR_GROUND);
            cv_text_c(120, 186 - 120, &AF_L, "RAINBOW", DR_RGB(255, 205, 96), DR_GROUND);
            cv_text_c(120, 221 - 120, &AF_S, "BUILT ON MELODEE", DR_RGB(156, 140, 180), DR_GROUND);
        }
        cv_blit(0, y0);
    }
    lcd_sync();
}
