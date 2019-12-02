/*****************************************************************************
*                     INSTITUTO POLITÉCNICO NACIONAL
* UNIDAD PROFESIONAL INTERDISCIPLINARIA EN INGENIERÍA Y TECNOLOGÍAS AVANZADAS
*
*                   SISTEMAS OPERATIVOS EN TIEMPO REAL.
*                             Periodo 20/1
*
* Port de juego Fly'n'Shoot (Quantum Leaps) para GNU/Linux con ncurses
*
* Autores de port:
* José Fausto Romero Lujambio
* Pastor Alan Rodríguez Echeverría
*
******************************************************************************
* Product:  VGA screen output
* Last Updated for Version: 4.1.01
* Date of the Last Update:  Nov 02, 2009
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) 2002-2009 Quantum Leaps, LLC. All rights reserved.
*
* This software may be distributed and modified under the terms of the GNU
* General Public License version 2 (GPL) as published by the Free Software
* Foundation and appearing in the file GPL.TXT included in the packaging of
* this file. Please note that GPL Section 2[b] requires that all works based
* on this software must also be made publicly available under the terms of
* the GPL ("Copyleft").
*
* Alternatively, this software may be distributed and modified under the
* terms of Quantum Leaps commercial licenses, which expressly supersede
* the GPL and are specifically designed for licensees interested in
* retaining the proprietary status of their code.
*
* Contact information:
* Quantum Leaps Web site:  http://www.quantum-leaps.com
* e-mail:                  info@quantum-leaps.com
*****************************************************************************/
#include "qp_port.h"
#include "video.h"

#include <stdlib.h>
#include <stdio.h>

#define VIDEO_WIDTH  80
#define VIDEO_HEIGHT 25
#define VIDEO_BLINK_TICKS 15

typedef struct {
    uint8_t color;
    char letter;
} term_char_t;

/* variables locales */
static term_char_t terminal_buffer[VIDEO_HEIGHT][VIDEO_WIDTH];
static int pair_count; /* numero máximo de pares de color (generado) */

static inline short get_color_pair(uint8_t color);

/*..........................................................................*/
void init_terminal_buffer(void) {
    clear_terminal_buffer(0, 0, VIDEO_WIDTH, VIDEO_HEIGHT);
}
/*..........................................................................*/
void clear_terminal_buffer(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    int i, j;
    for (i = x1; i < x2; ++i) {
        for (j = y1; j < y2; ++j) {
            terminal_buffer[j][i].color = 0;
            terminal_buffer[j][i].letter = ' ';
        }
    }
}
/*..........................................................................*/
void Video_render(void) {
    static int blink_timer = 0;
    static bool blink_state = true;
    int i, j;

    for (j = 0; j < VIDEO_HEIGHT; ++j)
        for (i = 0; i < VIDEO_WIDTH; ++i) {
            uint8_t color = terminal_buffer[j][i].color;
            bool blinking = (color & 0x80) && 1;

            // pintar fondo y dibujar caracter considerando parpadeo
            attron(COLOR_PAIR(get_color_pair(color)));
            mvaddch(j,i,(blink_state || !blinking) ? terminal_buffer[j][i].letter : ' ');
        }

    // generador de frecuencia para blinker
    if ( (++blink_timer % VIDEO_BLINK_TICKS) == 0 )
        blink_state ^= true;

    refresh();
}
/*..........................................................................*/
void Video_clearScreen(uint8_t bgColor) {
    Video_clearRect(0,  0, VIDEO_WIDTH, VIDEO_HEIGHT, bgColor);
}
/*..........................................................................*/
void Video_clearRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
                     uint8_t bgColor)
{
    int i, j;

    clear_terminal_buffer(x1,y1,x2,y2);
    for (i = x1; i < x2; ++i) {
        for (j = y1; j < y2; ++j) {
            terminal_buffer[j][i].color = bgColor;
        }
    }
}
/*..........................................................................*/
void Video_printStrAt(uint8_t x, uint8_t y, uint8_t color,
                      char const *str)
{
    while (*str != '\0') {
        uint8_t current_color = terminal_buffer[y][x].color;
        terminal_buffer[y][x].letter = *str;
        terminal_buffer[y][x].color |= (current_color & 0xF0) | (color & 0x0F);
        ++str;
        ++x;
    }
}
/*..........................................................................*/
void Video_printNumAt(uint8_t x, uint8_t y, uint8_t color, uint32_t num) {
    char str[8]; str[7] = '\0';
    snprintf(str, sizeof(str), "%4d", num);
    Video_printStrAt(x, y, color, str);
}
/*..........................................................................*/
void Video_drawBitmapAt(uint8_t x, uint8_t y,
                        uint8_t const *bitmap, uint8_t width, uint8_t height)
{
    int i, j;
    uint8_t w = width;
    uint8_t h = height;

    /* perform the clipping */
    if (x > VIDEO_WIDTH) {
        x = VIDEO_WIDTH;
    }
    if (y > VIDEO_HEIGHT) {
        y = VIDEO_HEIGHT;
    }
    if (w > VIDEO_WIDTH - x) {
        w = VIDEO_WIDTH - x;
    }
    if (h > VIDEO_HEIGHT - y) {
        h = VIDEO_HEIGHT - y;
    }

    for (i = x; i < x+w; ++i) {
        for (j = y; j < y+h; ++j) {
            static uint8_t const pixel[2] = {
                VIDEO_BGND_BLACK,
                VIDEO_BGND_LIGHT_GRAY
            };
            uint8_t byte = bitmap[(i-x) + ((j-y) >> 3)*width];
            terminal_buffer[j][i].color = pixel[(byte >> (j & 0x7)) & 1];
        }
    }
}
/*..........................................................................*/
void Video_drawStringAt(uint8_t x, uint8_t y, char const *str) {
    static uint8_t const font5x7[95][5] = {
        { 0x00, 0x00, 0x00, 0x00, 0x00 },                            /* ' ' */
        { 0x00, 0x00, 0x4F, 0x00, 0x00 },                              /* ! */
        { 0x00, 0x07, 0x00, 0x07, 0x00 },                              /* " */
        { 0x14, 0x7F, 0x14, 0x7F, 0x14 },                              /* # */
        { 0x24, 0x2A, 0x7F, 0x2A, 0x12 },                              /* $ */
        { 0x23, 0x13, 0x08, 0x64, 0x62 },                              /* % */
        { 0x36, 0x49, 0x55, 0x22, 0x50 },                              /* & */
        { 0x00, 0x05, 0x03, 0x00, 0x00 },                              /* ' */
        { 0x00, 0x1C, 0x22, 0x41, 0x00 },                              /* ( */
        { 0x00, 0x41, 0x22, 0x1C, 0x00 },                              /* ) */
        { 0x14, 0x08, 0x3E, 0x08, 0x14 },                              /* * */
        { 0x08, 0x08, 0x3E, 0x08, 0x08 },                              /* + */
        { 0x00, 0x50, 0x30, 0x00, 0x00 },                              /* , */
        { 0x08, 0x08, 0x08, 0x08, 0x08 },                              /* - */
        { 0x00, 0x60, 0x60, 0x00, 0x00 },                              /* . */
        { 0x20, 0x10, 0x08, 0x04, 0x02 },                              /* / */
        { 0x3E, 0x51, 0x49, 0x45, 0x3E },                              /* 0 */
        { 0x00, 0x42, 0x7F, 0x40, 0x00 },                              /* 1 */
        { 0x42, 0x61, 0x51, 0x49, 0x46 },                              /* 2 */
        { 0x21, 0x41, 0x45, 0x4B, 0x31 },                              /* 3 */
        { 0x18, 0x14, 0x12, 0x7F, 0x10 },                              /* 4 */
        { 0x27, 0x45, 0x45, 0x45, 0x39 },                              /* 5 */
        { 0x3C, 0x4A, 0x49, 0x49, 0x30 },                              /* 6 */
        { 0x01, 0x71, 0x09, 0x05, 0x03 },                              /* 7 */
        { 0x36, 0x49, 0x49, 0x49, 0x36 },                              /* 8 */
        { 0x06, 0x49, 0x49, 0x29, 0x1E },                              /* 9 */
        { 0x00, 0x36, 0x36, 0x00, 0x00 },                              /* : */
        { 0x00, 0x56, 0x36, 0x00, 0x00 },                              /* ; */
        { 0x08, 0x14, 0x22, 0x41, 0x00 },                              /* < */
        { 0x14, 0x14, 0x14, 0x14, 0x14 },                              /* = */
        { 0x00, 0x41, 0x22, 0x14, 0x08 },                              /* > */
        { 0x02, 0x01, 0x51, 0x09, 0x06 },                              /* ? */
        { 0x32, 0x49, 0x79, 0x41, 0x3E },                              /* @ */
        { 0x7E, 0x11, 0x11, 0x11, 0x7E },                              /* A */
        { 0x7F, 0x49, 0x49, 0x49, 0x36 },                              /* B */
        { 0x3E, 0x41, 0x41, 0x41, 0x22 },                              /* C */
        { 0x7F, 0x41, 0x41, 0x22, 0x1C },                              /* D */
        { 0x7F, 0x49, 0x49, 0x49, 0x41 },                              /* E */
        { 0x7F, 0x09, 0x09, 0x09, 0x01 },                              /* F */
        { 0x3E, 0x41, 0x49, 0x49, 0x7A },                              /* G */
        { 0x7F, 0x08, 0x08, 0x08, 0x7F },                              /* H */
        { 0x00, 0x41, 0x7F, 0x41, 0x00 },                              /* I */
        { 0x20, 0x40, 0x41, 0x3F, 0x01 },                              /* J */
        { 0x7F, 0x08, 0x14, 0x22, 0x41 },                              /* K */
        { 0x7F, 0x40, 0x40, 0x40, 0x40 },                              /* L */
        { 0x7F, 0x02, 0x0C, 0x02, 0x7F },                              /* M */
        { 0x7F, 0x04, 0x08, 0x10, 0x7F },                              /* N */
        { 0x3E, 0x41, 0x41, 0x41, 0x3E },                              /* O */
        { 0x7F, 0x09, 0x09, 0x09, 0x06 },                              /* P */
        { 0x3E, 0x41, 0x51, 0x21, 0x5E },                              /* Q */
        { 0x7F, 0x09, 0x19, 0x29, 0x46 },                              /* R */
        { 0x46, 0x49, 0x49, 0x49, 0x31 },                              /* S */
        { 0x01, 0x01, 0x7F, 0x01, 0x01 },                              /* T */
        { 0x3F, 0x40, 0x40, 0x40, 0x3F },                              /* U */
        { 0x1F, 0x20, 0x40, 0x20, 0x1F },                              /* V */
        { 0x3F, 0x40, 0x38, 0x40, 0x3F },                              /* W */
        { 0x63, 0x14, 0x08, 0x14, 0x63 },                              /* X */
        { 0x07, 0x08, 0x70, 0x08, 0x07 },                              /* Y */
        { 0x61, 0x51, 0x49, 0x45, 0x43 },                              /* Z */
        { 0x00, 0x7F, 0x41, 0x41, 0x00 },                              /* [ */
        { 0x02, 0x04, 0x08, 0x10, 0x20 },                              /* \ */
        { 0x00, 0x41, 0x41, 0x7F, 0x00 },                              /* ] */
        { 0x04, 0x02, 0x01, 0x02, 0x04 },                              /* ^ */
        { 0x40, 0x40, 0x40, 0x40, 0x40 },                              /* _ */
        { 0x00, 0x01, 0x02, 0x04, 0x00 },                              /* ` */
        { 0x20, 0x54, 0x54, 0x54, 0x78 },                              /* a */
        { 0x7F, 0x48, 0x44, 0x44, 0x38 },                              /* b */
        { 0x38, 0x44, 0x44, 0x44, 0x20 },                              /* c */
        { 0x38, 0x44, 0x44, 0x48, 0x7F },                              /* d */
        { 0x38, 0x54, 0x54, 0x54, 0x18 },                              /* e */
        { 0x08, 0x7E, 0x09, 0x01, 0x02 },                              /* f */
        { 0x0C, 0x52, 0x52, 0x52, 0x3E },                              /* g */
        { 0x7F, 0x08, 0x04, 0x04, 0x78 },                              /* h */
        { 0x00, 0x44, 0x7D, 0x40, 0x00 },                              /* i */
        { 0x20, 0x40, 0x44, 0x3D, 0x00 },                              /* j */
        { 0x7F, 0x10, 0x28, 0x44, 0x00 },                              /* k */
        { 0x00, 0x41, 0x7F, 0x40, 0x00 },                              /* l */
        { 0x7C, 0x04, 0x18, 0x04, 0x78 },                              /* m */
        { 0x7C, 0x08, 0x04, 0x04, 0x78 },                              /* n */
        { 0x38, 0x44, 0x44, 0x44, 0x38 },                              /* o */
        { 0x7C, 0x14, 0x14, 0x14, 0x08 },                              /* p */
        { 0x08, 0x14, 0x14, 0x18, 0x7C },                              /* q */
        { 0x7C, 0x08, 0x04, 0x04, 0x08 },                              /* r */
        { 0x48, 0x54, 0x54, 0x54, 0x20 },                              /* s */
        { 0x04, 0x3F, 0x44, 0x40, 0x20 },                              /* t */
        { 0x3C, 0x40, 0x40, 0x20, 0x7C },                              /* u */
        { 0x1C, 0x20, 0x40, 0x20, 0x1C },                              /* v */
        { 0x3C, 0x40, 0x30, 0x40, 0x3C },                              /* w */
        { 0x44, 0x28, 0x10, 0x28, 0x44 },                              /* x */
        { 0x0C, 0x50, 0x50, 0x50, 0x3C },                              /* y */
        { 0x44, 0x64, 0x54, 0x4C, 0x44 },                              /* z */
        { 0x00, 0x08, 0x36, 0x41, 0x00 },                              /* { */
        { 0x00, 0x00, 0x7F, 0x00, 0x00 },                              /* | */
        { 0x00, 0x41, 0x36, 0x08, 0x00 },                              /* } */
        { 0x02, 0x01, 0x02, 0x04, 0x02 },                              /* ~ */
    };

    while (*str != '\0') {
        Video_drawBitmapAt(x, y, font5x7[*str - ' '], 5, 8);
        ++str;
        x += 6;
    }
}
/*..........................................................................*/
void init_color_pairs(void) {
    int i, j;
    start_color();

    // inicializar todos los pares de color
    for (i = 0; i < 8; i++)
        for (j = 0; j < 16; j++)
        {
           init_pair (pair_count, j, i);
           pair_count++;
        }
}
/*..........................................................................*/
static inline short get_color_pair(uint8_t color)
{
    int i = 0;
    short fore, back;
    short wanted_fore = color & 0x0F, wanted_back = (color & 0x70) >> 4;

    for (i = 0; i < pair_count; ++ i)
    {
        pair_content(i, &fore, &back);
        if ( (fore == wanted_fore) && (back == wanted_back) )
            break;
    }
    return i;
}
