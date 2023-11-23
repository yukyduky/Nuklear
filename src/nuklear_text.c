#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                              TEXT
 *
 * ===============================================================*/
NK_LIB void
nk_widget_text(struct nk_command_buffer *o, struct nk_rect b,
    const char *string, int len, const struct nk_text *t,
    nk_flags a, const struct nk_user_font *f)
{
    struct nk_rect label;
    float text_width;

    NK_ASSERT(o);
    NK_ASSERT(t);
    if (!o || !t) return;

    b.h = NK_MAX(b.h, 2 * t->padding.y);
    label.x = 0; label.w = 0;
    label.y = b.y + t->padding.y;
    label.h = NK_MIN(f->height, b.h - 2 * t->padding.y);

    text_width = f->width(f->userdata, f->height, (const char*)string, len);
    text_width += (2.0f * t->padding.x);

    /* align in x-axis */
    if (a & NK_TEXT_ALIGN_LEFT) {
        label.x = b.x + t->padding.x;
        label.w = NK_MAX(0, b.w - 2 * t->padding.x);
    } else if (a & NK_TEXT_ALIGN_CENTERED) {
        label.w = NK_MAX(1, 2 * t->padding.x + (float)text_width);
        label.x = (b.x + t->padding.x + ((b.w - 2 * t->padding.x) - label.w) / 2);
        label.x = NK_MAX(b.x + t->padding.x, label.x);
        label.w = NK_MIN(b.x + b.w, label.x + label.w);
        if (label.w >= label.x) label.w -= label.x;
    } else if (a & NK_TEXT_ALIGN_RIGHT) {
        label.x = NK_MAX(b.x + t->padding.x, (b.x + b.w) - (2 * t->padding.x + (float)text_width));
        label.w = (float)text_width + 2 * t->padding.x;
    } else return;

    /* align in y-axis */
    if (a & NK_TEXT_ALIGN_MIDDLE) {
        label.y = b.y + b.h/2.0f - (float)f->height/2.0f;
        label.h = NK_MAX(b.h/2.0f, b.h - (b.h/2.0f + f->height/2.0f));
    } else if (a & NK_TEXT_ALIGN_BOTTOM) {
        label.y = b.y + b.h - f->height;
        label.h = f->height;
    }
    nk_draw_text(o, label, (const char*)string, len, f, t->background, t->text);
}
NK_LIB void
nk_widget_text_wrap(struct nk_context *ctx, struct nk_command_buffer *o, struct nk_rect b,
	const char *string, int len, const struct nk_text *t,
	const struct nk_user_font *f)
{
	float width;
	int glyphs = 0;
	int fitting = 0;
	int done = 0;
	int rows = 0;
	struct nk_rect line;
	struct nk_text text;
	NK_INTERN nk_rune seperator[] = {' '};

	NK_ASSERT(o);
	NK_ASSERT(t);
	if (!o || !t) return;

	text.padding = nk_vec2(0,0);
	text.background = t->background;
	text.text = t->text;

	b.w = NK_MAX(b.w, 2 * t->padding.x);
	b.h = NK_MAX(b.h, 2 * t->padding.y);
	b.h = b.h - 2 * t->padding.y;

	line.x = b.x + t->padding.x;
	line.y = b.y + t->padding.y;
	line.w = b.w - 2 * t->padding.x;
	line.h = 2 * t->padding.y + f->height;

	fitting = nk_text_clamp(f, string, len, line.w, &glyphs, &width, seperator,NK_LEN(seperator));
	while (done < len) {
		if (!fitting) break;

		rows++;

		nk_bool newline_found = nk_false;
		for (int i = 0; i < fitting; i++) {
			if (string[done + i] == '\n') {
				fitting = i;
				newline_found = nk_true;
			}
		}

		nk_widget_text(o, line, &string[done], fitting, &text, NK_TEXT_LEFT, f);

		if (newline_found) {
			fitting++;
		}

		done += fitting;
		line.y += f->height + 2 * t->padding.y;
		fitting = nk_text_clamp(f, &string[done], len - done, line.w, &glyphs, &width, seperator,NK_LEN(seperator));
	}
	nk_layout_extend_label_height(ctx, rows);
}
NK_LIB void
nk_widget_text_wrap_coded(struct nk_context* ctx, struct nk_command_buffer* o, struct nk_rect b,
    const char* string, int len, const struct nk_text* t, const struct nk_user_font* f,
    struct nk_label_link* links, int* num_links, struct nk_label_icon* icons, int* num_icons)
{
    float width;
    int glyphs = 0;
    int fitting = 0;
    int done = 0;
    struct nk_rect line;
    struct nk_text text;
    NK_INTERN nk_rune seperator[] = { ' ' };

    NK_ASSERT(o);
    NK_ASSERT(t);
    if (!o || !t) return;

    text.padding = nk_vec2(0, 0);
    text.background = t->background;
    text.text = t->text;
    struct nk_color defaultColor = t->text;

    b.w = NK_MAX(b.w, 2 * t->padding.x);
    b.h = NK_MAX(b.h, 2 * t->padding.y);
    b.h = b.h - 2 * t->padding.y;

    line.x = b.x + t->padding.x;
    line.y = b.y + t->padding.y;
    line.w = b.w - 2 * t->padding.x;
    line.h = 2 * t->padding.y + f->height;

#ifndef __clang__
    char* clean_text = (char*)malloc(len);
#else
    char clean_text[len];
#endif

    nk_text_remove_code(string, &len, clean_text);

    int max_icons = *num_icons;
    *num_icons = 0;
    int max_links = *num_links;
    *num_links = 0;
    int rows = 0;
    int tags_found = 0;
    int colors_found = 0;
    int hex_code_len = 6;
    int keyword_total_len = 0;
    int link_start = 0;
    int code_offset = 0;
    nk_bool end_of_color = nk_true;

    while (done < len) {
        /* Find the the number of characters of the string that can fit in the width of the window, only separating at spaces */
        fitting = nk_text_clamp(f, &clean_text[done], len - done, line.w, &glyphs, &width, seperator, NK_LEN(seperator));
        /* if fitting is 0 or the height of the allocated space of the row has been reached: break */
        if (!fitting) break;

        rows++;

        int row_done = 0;
        float text_width = 0;
        int fitting_extended = 0;
        code_offset = colors_found * hex_code_len + keyword_total_len + tags_found;
        nk_bool newline_found = nk_false;

        /* Find the COLOR_DELIM characters and color the text according to the hex color */
        /* Example with COLOR_DELIM character set as '#': "This #FFFFFFword# will be colored white" */
        /* Find the linkdelim characters and calculate the bounds of the words they surround */
        /* Example with linkdelim characters set as '[' and ']': "This [word's] bounds will be sent back and 'words' will be the keywords" */
        for (int i = done; i < done + fitting + fitting_extended; i++) {
            if (string[i + code_offset] == NEWLINE_CHAR) {
                newline_found = nk_true;
                fitting = i - done - fitting_extended;
                fitting = i - done - fitting_extended;
            }
            else if (string[i + code_offset] == COLOR_DELIM) {
                text_width = f->width(f->userdata, f->height, &clean_text[done], row_done);
                struct nk_rect sub_line = line;
                sub_line.x += text_width;

                nk_widget_text(o, sub_line, &clean_text[done + row_done], i - done - row_done, &text, NK_TEXT_LEFT, f);

                row_done = i - done;

                struct nk_color textColor = defaultColor;
                if (end_of_color) {
                    colors_found++;
                    textColor = nk_rgb_hex(&string[i + code_offset + 1]);
                    textColor = nk_rgb_factor(textColor, ctx->style.text.color_factor);
                }

                end_of_color = !end_of_color;
                tags_found++;

                text.text = textColor;
                i--;
            }
            else if (string[i + code_offset] == LINK_DELIM_START) {
                tags_found++;

                link_start = i;
                text_width = f->width(f->userdata, f->height, &clean_text[done], i - done);
                struct nk_rect sub_line = line;
                sub_line.x += text_width;

                if (*num_links < max_links) {
                    links[*num_links].bounds.x = sub_line.x;
                    links[*num_links].bounds.y = sub_line.y;
                    links[*num_links].bounds.h = sub_line.h;
                }

                i--;
            }
            else if (string[i + code_offset] == LINK_DELIM_END) {

                text_width = f->width(f->userdata, f->height, &clean_text[link_start], i - link_start);
                struct nk_rect sub_line = line;
                sub_line.w = text_width;

                int kw_len = 0;
                const int link_tags = 2;
                do {
                    if (*num_links < max_links)
                        links[*num_links].keyword[kw_len] = string[i + code_offset + kw_len + link_tags];
                    kw_len++;
                } while (kw_len < len && string[i + code_offset + kw_len + link_tags] != LINK_KEY_DELIM_END);

                if (*num_links < max_links) {
                    links[*num_links].bounds.w = sub_line.w;
                    links[*num_links].keyword_len = kw_len;
                    links[*num_links].keyword[links[*num_links].keyword_len] = '\0';
                }
                keyword_total_len += kw_len;
                tags_found += link_tags + 1;
                (*num_links)++;
                i--;
            }
            else if (string[i + code_offset] == ICON_DELIM_START) {
                clean_text[i] = ' ';

                text_width = f->width(f->userdata, f->height, &clean_text[done], i - done);
                struct nk_rect sub_line = line;
                sub_line.x += text_width;

                int kw_len = 0;
                const int icon_tag = 1;
                do {
                    if (*num_icons < max_icons)
                        icons[*num_icons].keyword[kw_len] = string[i + code_offset + kw_len + icon_tag];
                    kw_len++;
                } while (kw_len < len && string[i + code_offset + kw_len + icon_tag] != ICON_DELIM_END);

                if (*num_icons < max_icons) {
                    icons[*num_icons].bounds.x = sub_line.x;
                    icons[*num_icons].bounds.y = sub_line.y;
                    icons[*num_icons].bounds.h = sub_line.h;
                    icons[*num_icons].bounds.w = sub_line.h;
                    icons[*num_icons].keyword_len = kw_len;
                    icons[*num_icons].keyword[kw_len] = '\0';
                }
                i++;
                clean_text[i] = ' ';
                keyword_total_len += kw_len;
                (*num_icons)++;
            }

            code_offset = colors_found * hex_code_len + keyword_total_len + tags_found;

            /* Check if the ends are the next characters and extend the for loop to make sure we finish them or else we break them */
            if (string[i + 1 + code_offset] == LINK_DELIM_END || string[i + 1 + code_offset] == COLOR_DELIM)
                fitting_extended++;
            else
                fitting_extended = 0;
        }
        if (row_done != fitting) {
            text_width = f->width(f->userdata, f->height, &clean_text[done], fitting - (fitting - row_done));
            struct nk_rect sub_line = line;
            sub_line.x += text_width;
            nk_widget_text(o, sub_line, &clean_text[done + row_done], fitting - row_done, &text, NK_TEXT_LEFT, f);
        }

        /* Skip drawing the newline character */
        if (newline_found) {
            fitting++;
        }

        done += fitting;
        line.y += f->height + 2 * t->padding.y;
    }
#ifndef __clang__
    free(clean_text);
#endif

    nk_layout_extend_label_height(ctx, rows);
}
NK_LIB
void nk_text_remove_code(const char* text, int* len, char* clean_text)
{
    int tags_found = 0;
    int colors_found = 0;
    int hex_code_len = 6;
    int link_keyword_len = 0;
    int icon_keyword_len = 0;
    nk_bool end_of_color = nk_true;


    int code_offset = 0;
    for (int i = 0; i < *len; i++) {
        if (text[i] != COLOR_DELIM && text[i] != LINK_DELIM_START && text[i] != LINK_DELIM_END) {
            clean_text[i - code_offset] = text[i];
        }
        else if (end_of_color && text[i] == COLOR_DELIM) {
            end_of_color = nk_false;
            tags_found++;
            colors_found++;
            i += hex_code_len;
        }
        else if (text[i] == COLOR_DELIM) {
            end_of_color = nk_true;
            tags_found++;
        }
        else if (text[i] == LINK_DELIM_START) {
            tags_found++;
        }
        else if (text[i] == LINK_DELIM_END) {
            tags_found += 3;
            i += 2;
            int kw_len = 0;
            while (text[i] != LINK_KEY_DELIM_END) {
                kw_len++;
                i++;
            }
            link_keyword_len += kw_len;
        }

        if (text[i] == ICON_DELIM_START) {
            i++;
            int kw_len = 0;
            while (text[i] != ICON_DELIM_END) {
                kw_len++;
                i++;
            }
            icon_keyword_len += kw_len;
            clean_text[i - code_offset - kw_len] = text[i];
        }

        code_offset = colors_found * hex_code_len + tags_found + icon_keyword_len + link_keyword_len;
    }

    (*len) = (*len) - code_offset;
    clean_text[*len] = '\0';
}
NK_API void
nk_text_colored(struct nk_context *ctx, const char *str, int len,
    nk_flags alignment, struct nk_color color)
{
    struct nk_window *win;
    const struct nk_style *style;

    struct nk_vec2 item_padding;
    struct nk_rect bounds;
    struct nk_text text;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return;

    win = ctx->current;
    style = &ctx->style;
    nk_panel_alloc_space(&bounds, ctx);
    item_padding = style->text.padding;

    text.padding.x = item_padding.x;
    text.padding.y = item_padding.y;
    text.background = style->window.background;
    text.text = nk_rgb_factor(color, style->text.color_factor);

    nk_widget_text(&win->buffer, bounds, str, len, &text, alignment, style->font);
}
NK_API void
nk_text_wrap_colored(struct nk_context *ctx, const char *str,
	int len, struct nk_color color)
{
	struct nk_window *win;
	const struct nk_style *style;

	struct nk_vec2 item_padding;
	struct nk_rect bounds;
	struct nk_text text;

	NK_ASSERT(ctx);
	NK_ASSERT(ctx->current);
	NK_ASSERT(ctx->current->layout);
	if (!ctx || !ctx->current || !ctx->current->layout) return;

	win = ctx->current;
	style = &ctx->style;
	nk_panel_alloc_space(&bounds, ctx);
	item_padding = style->text.padding;

	text.padding.x = item_padding.x;
	text.padding.y = item_padding.y;
	text.background = style->window.background;
	text.text = nk_rgb_factor(color, style->text.color_factor);

	nk_widget_text_wrap(ctx, &win->buffer, bounds, str, len, &text, style->font);
}
NK_API void
nk_text_wrap_coded(struct nk_context *ctx, const char *str,
	int len, struct nk_color color, struct nk_label_link *links, int* num_links, struct nk_label_icon *icons, int *num_icons)
{
	struct nk_window *win;
	const struct nk_style *style;

	struct nk_vec2 item_padding;
	struct nk_rect bounds;
	struct nk_text text;

	NK_ASSERT(ctx);
	NK_ASSERT(ctx->current);
	NK_ASSERT(ctx->current->layout);
	if (!ctx || !ctx->current || !ctx->current->layout) return;

	win = ctx->current;
	style = &ctx->style;
	nk_panel_alloc_space(&bounds, ctx);
	item_padding = style->text.padding;

	text.padding.x = item_padding.x;
	text.padding.y = item_padding.y;
	text.background = style->window.background;
    text.text = nk_rgb_factor(color, style->text.color_factor);

	nk_widget_text_wrap_coded(ctx, &win->buffer, bounds, str, len, &text, style->font, links, num_links, icons, num_icons);
}
#ifdef NK_INCLUDE_STANDARD_VARARGS
NK_API void
nk_labelf_colored(struct nk_context *ctx, nk_flags flags,
    struct nk_color color, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    nk_labelfv_colored(ctx, flags, color, fmt, args);
    va_end(args);
}
NK_API void
nk_labelf_colored_wrap(struct nk_context *ctx, struct nk_color color,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    nk_labelfv_colored_wrap(ctx, color, fmt, args);
    va_end(args);
}
NK_API void
nk_labelf(struct nk_context *ctx, nk_flags flags, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    nk_labelfv(ctx, flags, fmt, args);
    va_end(args);
}
NK_API void
nk_labelf_wrap(struct nk_context *ctx, const char *fmt,...)
{
    va_list args;
    va_start(args, fmt);
    nk_labelfv_wrap(ctx, fmt, args);
    va_end(args);
}
NK_API void
nk_labelfv_colored(struct nk_context *ctx, nk_flags flags,
    struct nk_color color, const char *fmt, va_list args)
{
    char buf[256];
    nk_strfmt(buf, NK_LEN(buf), fmt, args);
    nk_label_colored(ctx, buf, flags, color);
}

NK_API void
nk_labelfv_colored_wrap(struct nk_context *ctx, struct nk_color color,
    const char *fmt, va_list args)
{
    char buf[256];
    nk_strfmt(buf, NK_LEN(buf), fmt, args);
    nk_label_colored_wrap(ctx, buf, color);
}

NK_API void
nk_labelfv(struct nk_context *ctx, nk_flags flags, const char *fmt, va_list args)
{
    char buf[256];
    nk_strfmt(buf, NK_LEN(buf), fmt, args);
    nk_label(ctx, buf, flags);
}

NK_API void
nk_labelfv_wrap(struct nk_context *ctx, const char *fmt, va_list args)
{
    char buf[256];
    nk_strfmt(buf, NK_LEN(buf), fmt, args);
    nk_label_wrap(ctx, buf);
}

NK_API void
nk_value_bool(struct nk_context *ctx, const char *prefix, int value)
{
    nk_labelf(ctx, NK_TEXT_LEFT, "%s: %s", prefix, ((value) ? "true": "false"));
}
NK_API void
nk_value_int(struct nk_context *ctx, const char *prefix, int value)
{
    nk_labelf(ctx, NK_TEXT_LEFT, "%s: %d", prefix, value);
}
NK_API void
nk_value_uint(struct nk_context *ctx, const char *prefix, unsigned int value)
{
    nk_labelf(ctx, NK_TEXT_LEFT, "%s: %u", prefix, value);
}
NK_API void
nk_value_float(struct nk_context *ctx, const char *prefix, float value)
{
    double double_value = (double)value;
    nk_labelf(ctx, NK_TEXT_LEFT, "%s: %.3f", prefix, double_value);
}
NK_API void
nk_value_color_byte(struct nk_context *ctx, const char *p, struct nk_color c)
{
    nk_labelf(ctx, NK_TEXT_LEFT, "%s: (%d, %d, %d, %d)", p, c.r, c.g, c.b, c.a);
}
NK_API void
nk_value_color_float(struct nk_context *ctx, const char *p, struct nk_color color)
{
    double c[4]; nk_color_dv(c, color);
    nk_labelf(ctx, NK_TEXT_LEFT, "%s: (%.2f, %.2f, %.2f, %.2f)",
        p, c[0], c[1], c[2], c[3]);
}
NK_API void
nk_value_color_hex(struct nk_context *ctx, const char *prefix, struct nk_color color)
{
    char hex[16];
    nk_color_hex_rgba(hex, color);
    nk_labelf(ctx, NK_TEXT_LEFT, "%s: %s", prefix, hex);
}
#endif
NK_API void
nk_text(struct nk_context *ctx, const char *str, int len, nk_flags alignment)
{
    NK_ASSERT(ctx);
    if (!ctx) return;
    nk_text_colored(ctx, str, len, alignment, ctx->style.text.color);
}
NK_API void
nk_text_wrap(struct nk_context *ctx, const char *str, int len)
{
    NK_ASSERT(ctx);
    if (!ctx) return;
    nk_text_wrap_colored(ctx, str, len, ctx->style.text.color);
}
NK_API void
nk_label(struct nk_context *ctx, const char *str, nk_flags alignment)
{
    nk_text(ctx, str, nk_strlen(str), alignment);
}
NK_API void
nk_label_colored(struct nk_context *ctx, const char *str, nk_flags align,
    struct nk_color color)
{
    nk_text_colored(ctx, str, nk_strlen(str), align, color);
}
NK_API void
nk_label_wrap(struct nk_context *ctx, const char *str)
{
    nk_text_wrap(ctx, str, nk_strlen(str));
}
NK_API void
nk_label_colored_wrap(struct nk_context *ctx, const char *str, struct nk_color color)
{
    nk_text_wrap_colored(ctx, str, nk_strlen(str), color);
}

NK_API void
nk_label_coded_wrap(struct nk_context *ctx, const char *str, struct nk_color color, struct nk_label_link *links, int *num_links, struct nk_label_icon *icons, int *num_icons)
{
	nk_text_wrap_coded(ctx, str, nk_strlen(str), color, links, num_links, icons, num_icons);
}
