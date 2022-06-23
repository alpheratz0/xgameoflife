/*
	Copyright (C) 2022 <alpheratz99@protonmail.com>

	This program is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License version 2 as published by the
	Free Software Foundation.

	This program is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along with
	this program; if not, write to the Free Software Foundation, Inc., 59 Temple
	Place, Suite 330, Boston, MA 02111-1307 USA

*/

#ifndef __XGAMEOFLIFE_CONFIG_H__
#define __XGAMEOFLIFE_CONFIG_H__

/* global config */
const int frames_per_second = 12;
const int max_cell_size = 100;
const int min_cell_size = 5;
const int status_bar_height = 20;

/* colors */
unsigned int color_alive = 0x30a3f4;
unsigned int color_dead = 0x000000;
unsigned int color_border = 0x191919;
unsigned int color_status_text = 0x000000;
unsigned int color_status_bar = 0xf0f72a;

#endif
