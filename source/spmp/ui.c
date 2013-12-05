/* ui.c - user interface routines; part of the Caprice32 SPMP port
 *
 * Copyright (C) 2012, 2013 Ulrich Hecht <ulrich.hecht@gmail.com>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "ui.h"
#include "text.h"
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>

ge_key_data_t wait_for_key(void)
{
    static int auto_repeat = 0;
    static ge_key_data_t okeys;
    ge_key_data_t keys;
    NativeGE_getKeyInput4Ntv(&keys);
    uint32_t last_press_tick = NativeGE_getTime();
    while (auto_repeat && keys.keys == okeys.keys && NativeGE_getTime() < last_press_tick + 100) {
        NativeGE_getKeyInput4Ntv(&keys);
    }
    if (auto_repeat && keys.keys == okeys.keys)
        return keys;
    auto_repeat = 0;
    last_press_tick = NativeGE_getTime();
    while (keys.keys && keys.keys == okeys.keys) {
        NativeGE_getKeyInput4Ntv(&keys);
        if (NativeGE_getTime() > last_press_tick + 500) {
            auto_repeat = 1;
            okeys = keys;
            return keys;
        }
    }
    okeys = keys;
    while (okeys.keys == keys.keys) {
        NativeGE_getKeyInput4Ntv(&keys);
    }
    okeys = keys;
    return keys;
}

static void invert(uint16_t *start, int size)
{
    while (size) {
        *start = ~(*start);
        start++;
        size--;
    }
}

void memset16(uint16_t *mem, uint16_t val, int count)
{
    int i;
    for (i = 0; i < count; i++)
        mem[i] = val;
}

int select_file(const char *start, const char *extension, char **file, int font_size, uint16_t fg, uint16_t bg)
{
    char wd[256];
    getcwd(wd, 256);
    struct _ecos_dirent *dents;
    struct _ecos_stat *stats;
    dents = malloc(sizeof(struct _ecos_dirent) * 10);
    stats = malloc(sizeof(struct _ecos_stat) * 10);
    int num_dents = 10;

    text_set_fg_color(fg);
    text_set_bg_color(bg);

    if (start) {
        if (_ecos_chdir(start) < 0)
            return -1;
    }

    _ecos_DIR *dir;
reload_dir:
    dir = _ecos_opendir(".");
    if (!dir)
        return -2;
    int dent_count = 0;
    struct _ecos_dirent *de;
    while ((de = _ecos_readdir(dir))) {
        _ecos_stat(de->d_name, &stats[dent_count]);
        if (extension) {
            if (!_ECOS_S_ISDIR(stats[dent_count].st_mode)) {
                char *ext = rindex(de->d_name, '.');
                if (!ext || !strcasestr(extension, ext + 1))
                    continue;
            }
        }
        memcpy(&dents[dent_count], de, sizeof(struct _ecos_dirent));
        dent_count++;
        if (dent_count >= num_dents) {
            num_dents *= 2;
            dents = realloc(dents, sizeof(struct _ecos_dirent) * num_dents);
            stats = realloc(stats, sizeof(struct _ecos_stat) * num_dents);
        }
    }
    _ecos_closedir(dir);

    int screen_width = gDisplayDev->getWidth();
    int screen_height = gDisplayDev->getHeight();
    int max_entries_displayed = screen_height / font_size;
    int current_top = 0;
    int current_entry = 0;

    int old_size = text_get_font_size();
    text_set_font_size(font_size);
    
    for (;;) {
        int i;
        uint16_t *fb = gDisplayDev->getShadowBuffer();
        memset16(fb, bg, screen_width * screen_height);
        for (i = current_top;
             i < current_top + max_entries_displayed && i < current_top + dent_count; i++) {
            if (_ECOS_S_ISDIR(stats[i].st_mode)) {
                int cw = text_draw_character('[', 0, (i - current_top) * font_size);
                cw += text_render(dents[i].d_name, cw, (i - current_top) * font_size);
                text_draw_character(']', cw, (i - current_top) * font_size);
            }
            else {
                text_render(dents[i].d_name, 0, (i - current_top) * font_size);
            }
        }
        invert(fb + (current_entry - current_top) * screen_width * font_size, font_size * screen_width);
        cache_sync();
        gDisplayDev->flip();
        ge_key_data_t keys = wait_for_key();
        if ((keys.keys & GE_KEY_UP) && current_entry > 0) {
            current_entry--;
            if (current_entry < current_top)
                current_top--;
        }
        else if ((keys.keys & GE_KEY_DOWN) && current_entry < dent_count - 1) {
            current_entry++;
            if ((current_entry - current_top + 1) * font_size > screen_height)
                current_top++;
        }
        else if (keys.keys & GE_KEY_O) {
            if (_ECOS_S_ISDIR(stats[current_entry].st_mode)) {
                _ecos_chdir(dents[current_entry].d_name);
                goto reload_dir;
            }
            else
                break;
        }
    }
    char buf[256];
    getcwd(buf, 256);
    char *filename = malloc(strlen(dents[current_entry].d_name) + 1 + strlen(buf) + 1);
    sprintf(filename, "%s/%s", buf, dents[current_entry].d_name);
    *file = filename;
    free(dents);
    free(stats);
    _ecos_chdir(wd);
    text_set_font_size(old_size);
    return 0;
}

static int bit_count(uint32_t val)
{
    int ret = 0;
    do {
        ret += (val & 1);
        val >>= 1;
    } while (val);
    return ret;
}
