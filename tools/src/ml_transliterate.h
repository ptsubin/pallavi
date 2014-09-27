/* 
 * ml_transliterate.h
 *
 * Copyright 2014 Pallavi Project <pallavi.malayalam@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef _ML_TRANSLITERATE_H
#define _ML_TRANSLITERATE_H

#include <inttypes.h>

/* Characters in the table may be of types symbol, schwa, vowels and consonants
 */
enum char_type {
	SYM,
	SCHWA,
	VOW,
	CON
};

/* Structure containing the unicode character code, its corresponding phonems
 * and the type of the character. */
struct phone_map {
	uint16_t code;
	char *phone;
	enum char_type type;
};

/* Map of malayalam unicode characters and their phonems */
extern struct phone_map char_set[128];

#endif
