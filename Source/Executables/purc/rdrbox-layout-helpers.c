/*
** @file rdrbox-layout-helpers.c
** @author Vincent Wei
** @date 2022/12/21
** @brief The implementation of layout helpers.
**
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of purc, which is an HVML interpreter with
** a command line interface (CLI).
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#undef NDEBUG

#include "rdrbox.h"
#include "rdrbox-internal.h"
#include "udom.h"

#include "unicode/unicode.h"

#include <stdio.h>
#include <assert.h>

struct _block_fmt_ctxt *foil_rdrbox_block_fmt_ctxt_new(foil_block_heap *heap,
        int width, int height)
{
    struct _block_fmt_ctxt *ctxt = calloc(1, sizeof(*ctxt));
    if (ctxt) {
        foil_region_init(&ctxt->region, heap);
        foil_rect rc = { 0, 0, width, height };
        if (height < 0)
            rc.bottom = INT_MAX;
        foil_region_set(&ctxt->region, &rc);

        ctxt->max_height = rc.bottom;
    }

    return ctxt;
}

void foil_rdrbox_block_fmt_ctxt_delete(struct _block_fmt_ctxt *ctxt)
{
    foil_region_empty(&ctxt->region);
    free(ctxt);
}

struct _inline_fmt_ctxt *foil_rdrbox_inline_fmt_ctxt_new(void)
{
    struct _inline_fmt_ctxt *ctxt = calloc(1, sizeof(*ctxt));

    return ctxt;
}

void foil_rdrbox_inline_fmt_ctxt_delete(struct _inline_fmt_ctxt *ctxt)
{
    for (size_t i = 0; i < ctxt->nr_lines; i++) {
        if (ctxt->lines[i].segs)
            free(ctxt->lines[i].segs);
    }

    if (ctxt->lines)
        free(ctxt->lines);
    free(ctxt);
}

#define SZ_IN_STACK_BUFF    128

int foil_rdrbox_inline_calc_preferred_width(foil_rdrbox *box)
{
    assert(box->is_inline_box);

    const uint32_t render_flags =
        FOIL_GRF_WRITING_MODE_HORIZONTAL_TB |
        FOIL_GRF_TEXT_ORIENTATION_UPRIGHT |
        FOIL_GRF_SPACES_REMOVE_START |
        FOIL_GRF_OVERFLOW_WRAP_NORMAL;
    const int max_extent = -1;

    foil_glyph_pos  gps_in_stack[SZ_IN_STACK_BUFF];

    struct _inline_box_data *inline_data = box->inline_data;
    struct text_paragraph *p;
    int x = 0, y = 0;
    list_for_each_entry(p, &inline_data->paras, ln) {
        assert(p->nr_ucs > 0);

        foil_glyph_pos *gps = gps_in_stack;
        if (p->nr_ucs > SZ_IN_STACK_BUFF) {
            gps = malloc(sizeof(*gps) * p->nr_ucs);
            if (gps == NULL) {
                goto failed;
            }
        }

        foil_ustr_get_glyphs_extent_simple(p->ucs, p->nr_ucs,
                p->break_oppos, render_flags,
                x, y, box->letter_spacing, box->word_spacing, 0,
                max_extent, NULL, NULL, gps);

        x = gps[p->nr_ucs - 1].x;
        y = gps[p->nr_ucs - 1].y;
        if (p->break_oppos[p->nr_ucs] == FOIL_BOV_LB_MANDATORY) {
            x = 0;
            y += box->line_height;
        }
        else {
            x += gps[p->nr_ucs - 1].advance;
        }

        if (gps != gps_in_stack)
            free(gps);
    }

    return x;

failed:
    return -1;
}

int foil_rdrbox_inline_calc_preferred_minimum_width(foil_rdrbox *box)
{
    assert(box->is_inline_box);

    const uint32_t render_flags =
        FOIL_GRF_WRITING_MODE_HORIZONTAL_TB |
        FOIL_GRF_TEXT_ORIENTATION_UPRIGHT |
        FOIL_GRF_SPACES_REMOVE_START |
        FOIL_GRF_OVERFLOW_WRAP_ANYWHERE;
    const int max_extent = FOIL_PX_GRID_CELL_W;

    int width = 0;
    foil_size line_size;
    foil_glyph_pos      gps_in_stack[SZ_IN_STACK_BUFF];

    struct _inline_box_data *inline_data = box->inline_data;
    struct text_paragraph *p;
    list_for_each_entry(p, &inline_data->paras, ln) {
        foil_glyph_pos     *gps = gps_in_stack;
        if (p->nr_ucs > SZ_IN_STACK_BUFF) {
            gps = malloc(sizeof(*gps) * p->nr_ucs);
            if (gps == NULL) {
                if (gps) free(gps);
                goto failed;
            }
        }

        size_t nr_laid = 0;
        while (nr_laid < p->nr_ucs) {
            size_t n =
                foil_ustr_get_glyphs_extent_simple(p->ucs + nr_laid,
                        p->nr_ucs - nr_laid, p->break_oppos + nr_laid,
                        render_flags, 0, 0, 0, 0, 0, max_extent,
                        &line_size, NULL, gps + nr_laid);

            if (line_size.cx > width)
                width = line_size.cx;
            nr_laid += n;
        }

        if (gps != gps_in_stack)
            free(gps);
    }

    return width;

failed:
    return -1;
}

struct _inline_segment *
foil_rdrbox_line_allocate_new_segment(struct _inline_fmt_ctxt *fmt_ctxt)
{
    struct _line_info *line = fmt_ctxt->lines + (fmt_ctxt->nr_lines - 1);
    line->segs = realloc(line->segs,
            sizeof(struct _inline_segment) * (line->nr_segments + 1));

    if (line->segs == NULL)
        return NULL;

    struct _inline_segment *seg = line->segs + line->nr_segments;
    memset(seg, 0, sizeof(struct _inline_segment));

    line->nr_segments++;
    return seg;
}

struct _line_info *foil_rdrbox_layout_inline(foil_layout_ctxt *ctxt,
        foil_rdrbox *block, foil_rdrbox *box)
{
    assert(block->is_block_level && box->is_inline_box);

    struct _inline_fmt_ctxt *fmt_ctxt = block->block_data->lfmt_ctxt;
    assert(fmt_ctxt->lines && fmt_ctxt->nr_lines > 1);
    struct _line_info *line = fmt_ctxt->lines + (fmt_ctxt->nr_lines - 1);

    struct _inline_box_data *inline_data = box->inline_data;
    if (inline_data->nr_paras == 0)
        return line;

    const uint32_t render_flags =
        FOIL_GRF_WRITING_MODE_HORIZONTAL_TB |
        FOIL_GRF_TEXT_ORIENTATION_UPRIGHT |
        FOIL_GRF_OVERFLOW_WRAP_NORMAL;

    struct text_paragraph *p;
    list_for_each_entry(p, &inline_data->paras, ln) {
        assert(p->nr_ucs > 0);

        if (p->glyph_poses == NULL) {
            p->glyph_poses = malloc(sizeof(foil_glyph_pos) * p->nr_ucs);
            if (p->glyph_poses == NULL)
                goto failed;
        }

        size_t nr_laid = 0;
        while (nr_laid < p->nr_ucs) {
            foil_size seg_size;
            size_t n =
                foil_ustr_get_glyphs_extent_simple(p->ucs + nr_laid,
                        p->nr_ucs + nr_laid,
                        p->break_oppos + nr_laid, render_flags,
                        line->x, line->y,
                        box->letter_spacing, box->word_spacing, 0,
                        line->left_extent, &seg_size, NULL,
                        p->glyph_poses + nr_laid);
            assert(n > 0);
            if (seg_size.cx > line->left_extent &&
                    fmt_ctxt->poss_extent > line->left_extent) {
                /* try to allocate a new line */
                line = foil_rdrbox_block_allocate_new_line(ctxt, block);
                if (line == NULL)
                    goto failed;
                continue;
            }

            struct _inline_segment *seg;
            seg = foil_rdrbox_line_allocate_new_segment(fmt_ctxt);
            if (seg == NULL)
                goto failed;

            seg->box = box;
            seg->span = p;
            seg->first_uc = nr_laid;
            seg->nr_ucs = n;
            foil_rect_set(&seg->rc, line->x, line->y,
                    line->x + seg_size.cx, line->y + seg_size.cy);
            foil_rdrbox_line_set_size(line, seg_size.cx, seg_size.cy);

            nr_laid += n;
            if (seg_size.cx >= line->left_extent) {
                line = foil_rdrbox_block_allocate_new_line(ctxt, block);
                if (line == NULL)
                    goto failed;
            }
            else {
                line->left_extent -= seg_size.cx;
                line->x += seg_size.cx;
            }
        }

        if (p->break_oppos[p->nr_ucs] == FOIL_BOV_LB_MANDATORY) {
            line = foil_rdrbox_block_allocate_new_line(ctxt, block);
        }
    }

    return line;

failed:
    return NULL;
}
