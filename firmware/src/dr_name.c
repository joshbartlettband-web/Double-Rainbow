/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DR_NAME_MAX
/* the kid's name, for the hello: 10 letters at most, in bytes of the settings no engine uses (the retired SLICE's
 * favourites row, bytes 20..30), its letters or spaces, then a check byte. The same place and check as Rainbow
 * mode's (felucca-rainbow kid.c), so one tool reads both. Set on the device (HOME held, ALGORITHM: dr_name_*) or
 * from the website (the editor's NAME, cmd 50: editor.c) */
#define DR_NAME_MAX 10u
#define DR_NAME_AT (&favorites.factory[13][20])
static uint32_t dr_name_check(const uint8_t *b)
{
    uint32_t i, c = 0x5Au;
    for (i = 0; i < DR_NAME_MAX; i++)
        c = ((c << 1) | (c >> 7)) ^ b[i];
    return c & 0xFFu;
}
static int dr_name_ok(const uint8_t *b)              /* letters or spaces, and the check */
{
    uint32_t i;
    for (i = 0; i < DR_NAME_MAX; i++)
        if (b[i] != ' ' && (b[i] < 'A' || b[i] > 'Z'))
            return 0;
    return dr_name_check(b) == b[DR_NAME_MAX];
}
static uint32_t dr_name_load(char *out)              /* the kept name, its spaces at the end left off; its length */
{
    const uint8_t *b = DR_NAME_AT;
    uint32_t i, n = 0;
    out[0] = 0;
    if (!dr_name_ok(b))
        return 0;
    for (i = 0; i < DR_NAME_MAX; i++) {
        out[i] = (char)b[i];
        if (b[i] != ' ')
            n = i + 1u;
    }
    out[n] = 0;
    return n;
}
static int dr_name_store(const uint8_t *s)           /* 10 letters or spaces in (a website's lower case raised); 0 refused */
{
    uint8_t *d = DR_NAME_AT, b[DR_NAME_MAX];
    uint32_t i;
    for (i = 0; i < DR_NAME_MAX; i++) {
        b[i] = s[i] >= 'a' && s[i] <= 'z' ? (uint8_t)(s[i] - 32u) : s[i];
        if (b[i] != ' ' && (b[i] < 'A' || b[i] > 'Z'))
            return 0;
    }
    for (i = 0; i < DR_NAME_MAX; i++)
        d[i] = b[i];
    d[DR_NAME_MAX] = (uint8_t)dr_name_check(d);
    settings_save();                                 /* (written by settings_poll once the beat is stopped) */
    return 1;
}
#endif
