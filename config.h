/*
	Copyright (C) 2022 <alpheratz99@protonmail.com>

	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License version 2 as published by
	the Free Software Foundation.

	This program is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
	more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc., 59
	Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/

#pragma once

#include <stdint.h>

//////////////////////////////////////////
//////////////////////////////////// THEME
//////////////////////////////////////////
const uint32_t dead_color   =    0x000000;
const uint32_t alive_color  =    0x30a3f4;
const uint32_t border_color =    0x191919;
const uint32_t bar_color    =    0xf0f72a;
const uint32_t text_color   =    0x000000;
//////////////////////////////////////////

//////////////////////////////////////////
///////////////////////////////////// MISC
//////////////////////////////////////////
const int generations_per_second =     12;
const int default_columns        =    300;
const int default_rows           =    300;
const int initial_cellsize       =     20;
const int min_cellsize           =      5;
const int max_cellsize           =     50;
//////////////////////////////////////////
