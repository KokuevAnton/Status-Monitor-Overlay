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

#include <stdint.h>
#include <math.h>
#include "../include/t210.h"

#define CLKRST_PLLD_BASE 0xD0
#define CLKRST_PLLD_MISC_0 0xDC
#define CLKRST_PLLD_MISC_1 0xD8
#define CLK_RST_IO_BASE 0x60006000
#define CLK_RST_IO_SIZE 0x1000
#define SET_BITS(VAL, BITS, SHIFT) ((VAL & BITS) << SHIFT)

#define CLOCK(x) (*(volatile u32 *)(g_clk_base + (x)))

// Generated lookup table for LCD clock rates
typedef struct {
    uint16_t divn;
    int16_t sdm_din;
} LcdClockEntry;

static const LcdClockEntry lcd_clock_table[] = {
    { 32, -3584 }, // 10 Hz
    { 32, -244 }, // 11 Hz
    { 32, 3094 }, // 12 Hz
    { 40, -1786 }, // 13 Hz
    { 40, 1552 }, // 14 Hz
    { 48, -3328 }, // 15 Hz
    { 48, 11 }, // 16 Hz
    { 48, 3350 }, // 17 Hz
    { 56, -1530 }, // 18 Hz
    { 56, 1808 }, // 19 Hz
    { 64, -3072 }, // 20 Hz
    { 64, 266 }, // 21 Hz
    { 64, 3606 }, // 22 Hz
    { 72, -1274 }, // 23 Hz
    { 72, 2064 }, // 24 Hz
    { 80, -2816 }, // 25 Hz
    { 80, 522 }, // 26 Hz
    { 80, 3862 }, // 27 Hz
    { 88, -1018 }, // 28 Hz
    { 88, 2320 }, // 29 Hz
    { 96, -2560 }, // 30 Hz
    { 96, 778 }, // 31 Hz
    { 96, 4118 }, // 32 Hz
    { 104, -762 }, // 33 Hz
    { 104, 2576 }, // 34 Hz
    { 112, -2304 }, // 35 Hz
    { 112, 1034 }, // 36 Hz
    { 120, -3845 }, // 37 Hz
    { 120, -506 }, // 38 Hz
    { 120, 2832 }, // 39 Hz
    { 128, -2048 }, // 40 Hz
    { 128, 1290 }, // 41 Hz
    { 136, -3589 }, // 42 Hz
    { 136, -250 }, // 43 Hz
    { 136, 3088 }, // 44 Hz
    { 144, -1792 }, // 45 Hz
    { 144, 1546 }, // 46 Hz
    { 152, -3333 }, // 47 Hz
    { 152, 5 }, // 48 Hz
    { 152, 3344 }, // 49 Hz
    { 160, -1536 }, // 50 Hz
    { 160, 1802 }, // 51 Hz
    { 168, -3077 }, // 52 Hz
    { 168, 261 }, // 53 Hz
    { 168, 3600 }, // 54 Hz
    { 176, -1280 }, // 55 Hz
    { 176, 2058 }, // 56 Hz
    { 184, -2821 }, // 57 Hz
    { 184, 517 }, // 58 Hz
    { 184, 3856 }, // 59 Hz
    { 192, -1024 }, // 60 Hz
    { 192, 2314 }, // 61 Hz
    { 200, -2565 }, // 62 Hz
    { 200, 773 }, // 63 Hz
    { 200, 4112 }, // 64 Hz
    { 208, -768 }, // 65 Hz
    { 208, 2570 }, // 66 Hz
    { 216, -2310 }, // 67 Hz
    { 216, 1029 }, // 68 Hz
    { 224, -3851 }, // 69 Hz
    { 224, -512 }, // 70 Hz
    { 224, 2826 }, // 71 Hz
    { 232, -2054 }, // 72 Hz
    { 232, 1285 }, // 73 Hz
    { 240, -4430 },  // 74.000 Hz
    { 240, -256 }, // 75 Hz
    { 240, 3082 }, // 76 Hz
    { 248, -1798 }, // 77 Hz
    { 248, 1541 }, // 78 Hz
    { 248,  4879 }, // 79 Hz
    { 248,  8217 }, // 80 Hz
    { 248, 11555 }, // 81 Hz
    { 248, 14893 }, // 82 Hz
    { 248, 18231 }, // 83 Hz
    { 248, 21569 }, // 84 Hz
    { 248, 24907 }, // 85 Hz
    { 248, 28245 }, // 86 Hz
    { 248, 31583 }, // 87 Hz
};

#define LCD_CLOCK_TABLE_SIZE (sizeof(lcd_clock_table) / sizeof(lcd_clock_table[0]))
static uintptr_t g_clk_base = 0;

static inline Result _svcQueryIoMappingFallback(u64* virtaddr, u64 physaddr, u64 size)
{
    if(hosversionAtLeast(10,0,0))
    {
        u64 out_size;
        return svcQueryMemoryMapping(virtaddr, &out_size, physaddr, size);
    }
    else
    {
        return svcLegacyQueryIoMapping(virtaddr, physaddr, size);
    }
}

static inline u32 _readReg(u64 offset)
{
    return *(volatile u32 *)(g_clk_base + offset);
}

static inline void _writeReg(u64 offset, u32 value)
{
    *(volatile u32 *)(g_clk_base + offset) = value;
}

float t210ClkLcdFreq(void)
{
    if (!g_clk_base)
    {
        _svcQueryIoMappingFallback(&g_clk_base, CLK_RST_IO_BASE, CLK_RST_IO_SIZE);
    }
    if (!g_clk_base)
    {
        return 0.0f;
    }

    uint32_t plld_base = _readReg(CLKRST_PLLD_BASE);
    uint32_t plld_misc = _readReg(CLKRST_PLLD_MISC_0);
    
    uint32_t plld_divn = GET_BITS(plld_base, 15, 8);
    uint32_t plld_divp = GET_BITS(plld_base, 22, 20);
    int16_t sdm_din = (int16_t)(plld_misc & 0xFFFF);

    // Constants
    const float divn_unit_hz = 2.4616718f;

    // Calculate base frequency from divn
    float base_freq = (plld_divn / 8.0f) * divn_unit_hz;

    // Calculate additional frequency from sdm_din
    float sdm_freq = 0.0f;
    if (sdm_din != 0)
    {
        sdm_freq = (float)(sdm_din + 4096) * 0.92f / 3072.0f;
    }

    // Total frequency
    float total_freq = plld_divp != 0 ? (base_freq + sdm_freq) / (2 * plld_divp) * 2 : (base_freq + sdm_freq) * 2;

    // Return the frequency multiplied by 1000 as per the original function
    return (uint32_t)(total_freq * 1000.0f);
}

inline void plldSetClockRate(uint32_t rate)
{
    if (!g_clk_base)
    {
        Result rc = _svcQueryIoMappingFallback(&g_clk_base, 0x60006000ul, 0x1000);
        if (R_FAILED(rc))
            return;
    }
    if (!g_clk_base)
    {
        return;
    }
    if ((rate - 10) >= 91)
    {
        return;
    }
    const LcdClockEntry *entry = &lcd_clock_table[rate - 10];
    uint32_t plld_base = _readReg(CLKRST_PLLD_BASE);
    uint32_t plld_misc = _readReg(CLKRST_PLLD_MISC_0);
    
    plld_base = (plld_base & ~(0xFF << 8)) | (entry->divn << 8);
    plld_misc = (plld_misc & 0xFFFF0000) | (uint16_t)entry->sdm_din;
    
    _writeReg(CLKRST_PLLD_BASE, plld_base);
    _writeReg(CLKRST_PLLD_MISC_0, plld_misc);
}

uint32_t GetHzLCD(void)
{
    if (!g_clk_base)
    {
        Result rc = _svcQueryIoMappingFallback(&g_clk_base, CLK_RST_IO_BASE, CLK_RST_IO_SIZE);
        if (R_FAILED(rc))
            return 60;
    }
    if (!g_clk_base)
    {
        return 60;
    }

    uint32_t plld_base = _readReg(CLKRST_PLLD_BASE);
    uint32_t plld_misc = _readReg(CLKRST_PLLD_MISC_0);
    
    uint16_t divn = GET_BITS(plld_base, 15, 8);
    int16_t sdm_din = (int16_t)(plld_misc & 0xFFFF);

    // Поиск точного совпадения в таблице
    for (size_t i = 0; i < LCD_CLOCK_TABLE_SIZE; i++)
    {
        if (lcd_clock_table[i].divn == divn && lcd_clock_table[i].sdm_din == sdm_din)
        {
            return i + 10;  // Таблица начинается с 10 Гц
        }
    }

    // Если точное совпадение не найдено, возвращаем значение по умолчанию
    return 60;
}

