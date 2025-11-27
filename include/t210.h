/*
 * Copyright (c) 2020-2023 CTCaer
 * Copyright (c) 2023 p-sam
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <switch.h>

#define REG(OFFSET) (*(volatile u32 *)(OFFSET))
#define GET_BITS(VAL, HIGH, LOW) ((VAL & ((1UL << (HIGH + 1UL)) - 1UL)) >> LOW)

float t210ClkLcdFreq(void);
uint32_t GetHzLCD(void);
void plldSetClockRate(u32 rate);

#ifdef __cplusplus
}
#endif

