#include <bmsparser/convert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "table.h"

void bms_sjis_to_utf8(const char *src, char *dst)
{
    size_t i;
    char *output = malloc(3 * strlen(src));
    for (i = 0; i < 3 * strlen(src); i++)
    {
        output[i] = ' ';
    }
    size_t indexSrc = 0, indexDst = 0;

    while (indexSrc < strlen(src))
    {
        char arraySection = ((uint8_t)src[indexSrc]) >> 4;

        size_t arrayOffset;
        if (arraySection == 0x8)
            arrayOffset = 0x100;
        else if (arraySection == 0x9)
            arrayOffset = 0x1100;
        else if (arraySection == 0xE)
            arrayOffset = 0x2100;
        else
            arrayOffset = 0;

        if (arrayOffset)
        {
            arrayOffset += (((uint8_t)src[indexSrc]) & 0xf) << 8;
            indexSrc++;
            if (indexSrc >= strlen(src))
                break;
        }
        arrayOffset += (uint8_t)src[indexSrc++];
        arrayOffset <<= 1;

        uint16_t unicodeValue = (shiftJIS_convTable[arrayOffset] << 8) | shiftJIS_convTable[arrayOffset + 1];

        if (unicodeValue < 0x80)
        {
            dst[indexDst++] = unicodeValue;
        }
        else if (unicodeValue < 0x800)
        {
            dst[indexDst++] = 0xC0 | (unicodeValue >> 6);
            dst[indexDst++] = 0x80 | (unicodeValue & 0x3f);
        }
        else
        {
            dst[indexDst++] = 0xE0 | (unicodeValue >> 12);
            dst[indexDst++] = 0x80 | ((unicodeValue & 0xfff) >> 6);
            dst[indexDst++] = 0x80 | (unicodeValue & 0x3f);
        }
    }
    dst[indexDst] = '\0';
}