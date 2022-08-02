#include <bmsparser.h>
#include <bmsparser/convert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "sort.h"

static unsigned char sectcomp(void *a, void *b)
{
    bms_Sector *n = (bms_Sector *)a;
    bms_Sector *m = (bms_Sector *)b;
    return n->fraction < m->fraction;
}

static unsigned char objcomp(void *a, void *b)
{
    bms_Obj *n = (bms_Obj *)a;
    bms_Obj *m = (bms_Obj *)b;
    return n->fraction < m->fraction;
}

bms_Chart *bms_alloc()
{
    size_t i;
    bms_Chart *chart = malloc(sizeof(bms_Chart));
    chart->genre = NULL;
    chart->title = NULL;
    chart->subtitle = NULL;
    chart->artist = NULL;
    chart->subartist = NULL;
    chart->stagefile = NULL;
    chart->banner = NULL;
    chart->playlevel = 0;
    chart->difficulty = bms_DIFFICULTY_NORMAL;
    chart->total = 160;
    chart->rank = bms_RANK_NORMAL;
    chart->wavs = malloc(sizeof(char *) * 1296);
    chart->bmps = malloc(sizeof(char *) * 1296);
    for (i = 0; i < 1296; i++)
    {
        chart->wavs[i] = NULL;
        chart->bmps[i] = NULL;
    }
    chart->signatures = malloc(sizeof(float) * 1000);
    for (i = 0; i < 1000; i++)
    {
        chart->signatures[i] = 1.0f;
    }
    chart->objs = NULL;
    chart->objs_size = 0;
    chart->sectors = malloc(sizeof(bms_Sector));
    chart->sectors[0].fraction = 0;
    chart->sectors[0].time = 0;
    chart->sectors[0].delta = 130.0 / 240.0;
    chart->sectors[0].inclusive = 1;
    chart->sectors_size = 1;
    return chart;
}

void bms_free(bms_Chart *chart)
{
    size_t i;
    free(chart->genre);
    free(chart->title);
    free(chart->subtitle);
    free(chart->artist);
    free(chart->subartist);
    free(chart->stagefile);
    free(chart->banner);
    for (i = 0; i < 1296; i++)
    {
        free(chart->bmps[i]);
        free(chart->wavs[i]);
    }
    free(chart->wavs);
    free(chart->bmps);
    free(chart->signatures);
    free(chart->objs);
    free(chart->sectors);
    free(chart);
}

static float fractionDiff(float *signatures, float a, float b)
{
    unsigned char negative = a > b;
    if (negative)
    {
        float temp = a;
        a = b;
        b = temp;
    }
    int aM = (int)a;
    int bM = (int)b;
    float aF = a - aM;
    float bF = b - bM;
    float result = bF * (bM >= 0 ? signatures[bM] : 1) - aF * (aM >= 0 ? signatures[aM] : 1);
    int i;
    for (i = aM; i < bM; i++)
    {
        if (i >= 0)
        {
            result += signatures[i];
        }
        else
        {
            result += 1;
        }
    }
    if (negative)
    {
        result = -result;
    }
    return result;
}

void bms_parse(bms_Chart *chart, FILE *input)
{
    regex_t randomRegex;
    regcomp(&randomRegex, "^\\s*#RANDOM\\s*(\\d+)\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t ifRegex;
    regcomp(&ifRegex, "^\\s*#IF\\s*(\\d+)\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t elseRegex;
    regcomp(&elseRegex, "^\\s*#ELSE\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t endifRegex;
    regcomp(&endifRegex, "^\\s*#ENDIF\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t genreRegex;
    regcomp(&genreRegex, "^\\s*#GENRE\\s*(.*)\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t titleRegex;
    regcomp(&titleRegex, "^\\s*#TITLE\\s*(.*)\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t nestedSubtitleRegex;
    regcomp(&nestedSubtitleRegex, "^\\s*(.*)\\s*\\[(.*)\\]\\s*$", REG_EXTENDED);
    regex_t subtitleRegex;
    regcomp(&subtitleRegex, "^\\s*#SUBTITLE\\s*(.*)\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t artistRegex;
    regcomp(&artistRegex, "^\\s*#ARTIST\\s*(.*)\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t subartistRegex;
    regcomp(&subartistRegex, "^\\s*#SUBARTIST\\s*(.*)\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t stagefileRegex;
    regcomp(&stagefileRegex, "^\\s*#STAGEFILE\\s*(.*)\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t bannerRegex;
    regcomp(&bannerRegex, "^\\s*#BANNER\\s*(.*)\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t playlevelRegex;
    regcomp(&playlevelRegex, "^\\s*#PLAYLEVEL\\s*(\\d+)\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t difficultyRegex;
    regcomp(&difficultyRegex, "^\\s*#DIFFICULTY\\s*([12345])\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t totalRegex;
    regcomp(&totalRegex, "^\\s*#TOTAL\\s*(\\d+(\\.\\d+)?)\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t rankRegex;
    regcomp(&rankRegex, "^\\s*#RANK\\s*([0123])\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t wavsRegex;
    regcomp(&wavsRegex, "^\\s*#WAV([0-9A-Z]{2})\\s*(.*)\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t bmpsRegex;
    regcomp(&bmpsRegex, "^\\s*#BMP([0-9A-Z]{2})\\s*(.*)\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t lnobjRegex;
    regcomp(&lnobjRegex, "^\\s*#LNOBJ\\s*([0-9A-Z]{2})\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t bpmRegex;
    regcomp(&bpmRegex, "^\\s*#BPM\\s*(\\d+(\\.\\d+)?(E\\+\\d+)?)\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t bpmsRegex;
    regcomp(&bpmsRegex, "^\\s*#BPM([0-9A-Z]{2})\\s*(\\d+(\\.\\d+)?(E\\+\\d+)?)\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t stopsRegex;
    regcomp(&stopsRegex, "^\\s*#STOP([0-9A-Z]{2})\\s*(\\d+)\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t signatureRegex;
    regcomp(&signatureRegex, "^\\s*#(\\d{3})02:(\\d+(\\.\\d+)?(E\\+\\d+)?)\\s*$", REG_ICASE | REG_EXTENDED);
    regex_t notesRegex;
    regcomp(&notesRegex, "^\\s*#(\\d{3})([0-9A-Z]{2}):(.*)\\s*$", REG_ICASE | REG_EXTENDED);

    fseek(input, 0, SEEK_END);
    long size = ftell(input);
    char *line = malloc(size);
    fseek(input, 0, SEEK_SET);

    srand(time(NULL));
    int random = 0;

    unsigned char *skip = malloc(sizeof(unsigned char));
    size_t nb_skip = 1;
    size_t skip_length = 1;
    skip[0] = 0;
    int *lnobj = malloc(sizeof(int));
    size_t nb_lnobj = 1;
    size_t lnobj_length = 0;
    double *bpms = malloc(sizeof(double) * 1296);
    float *stops = malloc(sizeof(float) * 1296);

    unsigned char lnflag[20] = {0};

    size_t nb_objs = 0;
    size_t nb_sectors = 1;

    size_t i;

    while (fgets(line, size, input))
    {
        if (strrchr(line, '\r'))
            *strrchr(line, '\r') = '\0';
        else if (strrchr(line, '\n'))
            *strrchr(line, '\n') = '\0';

        regmatch_t match[5];

        if (regexec(&randomRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            char *str = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(str, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            str[match[1].rm_eo - match[1].rm_so] = '\0';
            free(str);
            random = rand() % atoi(str) + 1;
        }
        else if (regexec(&ifRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            char *str = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(str, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            str[match[1].rm_eo - match[1].rm_so] = '\0';
            if (nb_skip == skip_length)
            {
                nb_skip *= 2;
                skip = realloc(skip, sizeof(unsigned char) * nb_skip);
            }
            skip[skip_length++] = atoi(str) != random;
            free(str);
        }
        else if (regexec(&elseRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            skip[skip_length - 1] = !skip[skip_length - 1];
        }
        else if (regexec(&endifRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            skip_length--;
        }

        if (skip[skip_length - 1])
        {
            continue;
        }

        if (regexec(&genreRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            chart->genre = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(chart->genre, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            chart->genre[match[1].rm_eo - match[1].rm_so] = '\0';
        }
        else if (regexec(&titleRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            chart->title = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(chart->title, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            chart->title[match[1].rm_eo - match[1].rm_so] = '\0';
            if (regexec(&nestedSubtitleRegex, chart->title, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
            {
                chart->subtitle = malloc(sizeof(char) * (match[2].rm_eo - match[2].rm_so + 1));
                memcpy(chart->subtitle, chart->title + match[2].rm_so, sizeof(char) * (match[2].rm_eo - match[2].rm_so));
                chart->subtitle[match[2].rm_eo - match[2].rm_so] = '\0';
                chart->title[match[1].rm_eo] = '\0';
            }
        }
        else if (regexec(&subtitleRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            chart->subtitle = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(chart->subtitle, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            chart->subtitle[match[1].rm_eo - match[1].rm_so] = '\0';
        }
        else if (regexec(&artistRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            chart->artist = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(chart->artist, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            chart->artist[match[1].rm_eo - match[1].rm_so] = '\0';
        }
        else if (regexec(&subartistRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            chart->subartist = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(chart->subartist, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            chart->subartist[match[1].rm_eo - match[1].rm_so] = '\0';
        }
        else if (regexec(&stagefileRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            chart->stagefile = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(chart->stagefile, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            chart->stagefile[match[1].rm_eo - match[1].rm_so] = '\0';
        }
        else if (regexec(&bannerRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            chart->banner = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(chart->banner, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            chart->banner[match[1].rm_eo - match[1].rm_so] = '\0';
        }
        else if (regexec(&playlevelRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            char *str = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(str, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            str[match[1].rm_eo - match[1].rm_so] = '\0';
            chart->playlevel = atoi(str);
            free(str);
        }
        else if (regexec(&difficultyRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            char *str = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(str, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            str[match[1].rm_eo - match[1].rm_so] = '\0';
            chart->difficulty = atoi(str);
            free(str);
        }
        else if (regexec(&totalRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            char *str = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(str, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            str[match[1].rm_eo - match[1].rm_so] = '\0';
            chart->total = atof(str);
            free(str);
        }
        else if (regexec(&rankRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            char *str = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(str, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            str[match[1].rm_eo - match[1].rm_so] = '\0';
            chart->rank = atoi(str);
            free(str);
        }
        else if (regexec(&wavsRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            char *str = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(str, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            str[match[1].rm_eo - match[1].rm_so] = '\0';
            int a = strtol(str, NULL, 36);
            chart->wavs[a] = malloc(sizeof(char) * (match[2].rm_eo - match[2].rm_so + 1));
            memcpy(chart->wavs[a], line + match[2].rm_so, sizeof(char) * (match[2].rm_eo - match[2].rm_so));
            chart->wavs[a][match[2].rm_eo - match[2].rm_so] = '\0';
            free(str);
        }
        else if (regexec(&bmpsRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            char *str = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(str, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            str[match[1].rm_eo - match[1].rm_so] = '\0';
            int a = strtol(str, NULL, 36);
            chart->bmps[a] = malloc(sizeof(char) * (match[2].rm_eo - match[2].rm_so + 1));
            memcpy(chart->bmps[a], line + match[2].rm_so, sizeof(char) * (match[2].rm_eo - match[2].rm_so));
            chart->bmps[a][match[2].rm_eo - match[2].rm_so] = '\0';
            free(str);
        }
        else if (regexec(&lnobjRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            char *str = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(str, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            str[match[1].rm_eo - match[1].rm_so] = '\0';
            if (nb_lnobj == lnobj_length)
            {
                nb_lnobj *= 2;
                lnobj = realloc(lnobj, sizeof(int) * nb_lnobj);
            }
            lnobj[lnobj_length++] = strtol(str, NULL, 36);
            free(str);
        }
        else if (regexec(&bpmRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            char *str = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(str, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            str[match[1].rm_eo - match[1].rm_so] = '\0';
            chart->sectors[0].delta = atof(str) / 240.0;
            free(str);
        }
        else if (regexec(&bpmsRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            char *str = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(str, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            str[match[1].rm_eo - match[1].rm_so] = '\0';
            int a = strtol(str, NULL, 36);
            free(str);
            str = malloc(sizeof(char) * (match[2].rm_eo - match[2].rm_so + 1));
            memcpy(str, line + match[2].rm_so, sizeof(char) * (match[2].rm_eo - match[2].rm_so));
            str[match[2].rm_eo - match[2].rm_so] = '\0';
            bpms[a] = atof(str);
            free(str);
        }
        else if (regexec(&stopsRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            char *str = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(str, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            str[match[1].rm_eo - match[1].rm_so] = '\0';
            int a = strtol(str, NULL, 36);
            free(str);
            str = malloc(sizeof(char) * (match[2].rm_eo - match[2].rm_so + 1));
            memcpy(str, line + match[2].rm_so, sizeof(char) * (match[2].rm_eo - match[2].rm_so));
            str[match[2].rm_eo - match[2].rm_so] = '\0';
            stops[a] = atoi(str) / 192.0f;
            free(str);
        }
        else if (regexec(&signatureRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            char *str = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(str, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            str[match[1].rm_eo - match[1].rm_so] = '\0';
            int measure = atoi(str);
            free(str);
            str = malloc(sizeof(char) * (match[2].rm_eo - match[2].rm_so + 1));
            memcpy(str, line + match[2].rm_so, sizeof(char) * (match[2].rm_eo - match[2].rm_so));
            str[match[2].rm_eo - match[2].rm_so] = '\0';
            chart->signatures[measure] = atof(str);
            free(str);
        }
        else if (regexec(&signatureRegex, line, sizeof(match) / sizeof(regmatch_t), match, 0) == 0)
        {
            char *str = malloc(sizeof(char) * (match[1].rm_eo - match[1].rm_so + 1));
            memcpy(str, line + match[1].rm_so, sizeof(char) * (match[1].rm_eo - match[1].rm_so));
            str[match[1].rm_eo - match[1].rm_so] = '\0';
            int measure = atoi(str);
            free(str);
            str = malloc(sizeof(char) * (match[2].rm_eo - match[2].rm_so + 1));
            memcpy(str, line + match[2].rm_so, sizeof(char) * (match[2].rm_eo - match[2].rm_so));
            str[match[2].rm_eo - match[2].rm_so] = '\0';
            int channel = strtol(str, NULL, 36);
            free(str);
            str = malloc(sizeof(char) * (match[3].rm_eo - match[3].rm_so + 1));
            memcpy(str, line + match[3].rm_so, sizeof(char) * (match[3].rm_eo - match[3].rm_so));
            str[match[3].rm_eo - match[3].rm_so] = '\0';
            size_t len = strlen(str) / 2;
            for (i = 0; i < len; i++)
            {
                char j[3] = {str[i * 2], str[i * 2 + 1], '\0'};
                int key = strtol(j, NULL, 36);
                if (key)
                {
                    size_t k;
                    float fraction = measure + (float)i / len;
                    switch (channel)
                    {
                    case 1:
                        while (chart->objs_size + 1 > nb_objs)
                        {
                            nb_objs *= 2;
                            chart->objs = realloc(chart->objs, nb_objs);
                        }
                        chart->objs[chart->objs_size].type = bms_OBJTYPE_BGM;
                        chart->objs[chart->objs_size].fraction = fraction;
                        chart->objs[chart->objs_size].time = 0;
                        chart->objs[chart->objs_size].bgm.key = key;
                        chart->objs_size++;
                        break;
                    case 3:
                        while (chart->sectors_size + 1 > nb_sectors)
                        {
                            nb_sectors *= 2;
                            chart->sectors = realloc(chart->sectors, nb_sectors);
                        }
                        chart->sectors[chart->sectors_size].fraction = fraction;
                        chart->sectors[chart->sectors_size].time = 0;
                        chart->sectors[chart->sectors_size].delta = strtol(j, NULL, 16) / 240.0;
                        chart->sectors[chart->sectors_size].inclusive = 1;
                        chart->sectors_size++;
                        break;
                    case 4:
                        while (chart->objs_size + 1 > nb_objs)
                        {
                            nb_objs *= 2;
                            chart->objs = realloc(chart->objs, nb_objs);
                        }
                        chart->objs[chart->objs_size].type = bms_OBJTYPE_BMP;
                        chart->objs[chart->objs_size].fraction = fraction;
                        chart->objs[chart->objs_size].time = 0;
                        chart->objs[chart->objs_size].bmp.key = key;
                        chart->objs[chart->objs_size].bmp.layer = 0;
                        chart->objs_size++;
                        break;
                    case 6:
                        while (chart->objs_size + 1 > nb_objs)
                        {
                            nb_objs *= 2;
                            chart->objs = realloc(chart->objs, nb_objs);
                        }
                        chart->objs[chart->objs_size].type = bms_OBJTYPE_BMP;
                        chart->objs[chart->objs_size].fraction = fraction;
                        chart->objs[chart->objs_size].time = 0;
                        chart->objs[chart->objs_size].bmp.key = key;
                        chart->objs[chart->objs_size].bmp.layer = -1;
                        chart->objs_size++;
                        break;
                    case 7:
                        while (chart->objs_size + 1 > nb_objs)
                        {
                            nb_objs *= 2;
                            chart->objs = realloc(chart->objs, nb_objs);
                        }
                        chart->objs[chart->objs_size].type = bms_OBJTYPE_BMP;
                        chart->objs[chart->objs_size].fraction = fraction;
                        chart->objs[chart->objs_size].time = 0;
                        chart->objs[chart->objs_size].bmp.key = key;
                        chart->objs[chart->objs_size].bmp.layer = 1;
                        chart->objs_size++;
                        break;
                    case 8:
                        while (chart->sectors_size + 1 > nb_sectors)
                        {
                            nb_sectors *= 2;
                            chart->sectors = realloc(chart->sectors, nb_sectors);
                        }
                        chart->sectors[chart->sectors_size].fraction = fraction;
                        chart->sectors[chart->sectors_size].time = 0;
                        chart->sectors[chart->sectors_size].delta = bpms[key] / 240.0;
                        chart->sectors[chart->sectors_size].inclusive = 1;
                        chart->sectors_size++;
                        break;
                    case 9:
                        while (chart->sectors_size + 2 > nb_sectors)
                        {
                            nb_sectors *= 2;
                            chart->sectors = realloc(chart->sectors, nb_sectors);
                        }
                        chart->sectors[chart->sectors_size].fraction = fraction;
                        chart->sectors[chart->sectors_size].time = 0;
                        chart->sectors[chart->sectors_size].delta = 0;
                        chart->sectors[chart->sectors_size].inclusive = 1;
                        chart->sectors_size++;
                        chart->sectors[chart->sectors_size].fraction = fraction;
                        chart->sectors[chart->sectors_size].time = stops[key];
                        chart->sectors[chart->sectors_size].delta = chart->sectors[chart->sectors_size - 2].delta;
                        chart->sectors[chart->sectors_size].inclusive = 0;
                        chart->sectors_size++;
                        break;
                    case 37:
                    case 38:
                    case 39:
                    case 40:
                    case 41:
                    case 42:
                    case 43:
                    case 44:
                    case 45:
                    case 73:
                    case 74:
                    case 75:
                    case 76:
                    case 77:
                    case 78:
                    case 79:
                    case 80:
                    case 81:
                        while (chart->objs_size + 1 > nb_objs)
                        {
                            nb_objs *= 2;
                            chart->objs = realloc(chart->objs, nb_objs);
                        }
                        chart->objs[chart->objs_size].type = bms_OBJTYPE_NOTE;
                        chart->objs[chart->objs_size].fraction = fraction;
                        chart->objs[chart->objs_size].time = 0;
                        chart->objs[chart->objs_size].note.player = channel / 36;
                        chart->objs[chart->objs_size].note.line = channel % 36;
                        chart->objs[chart->objs_size].note.key = key;
                        chart->objs[chart->objs_size].note.end = 0;
                        for (k = 0; k < lnobj_length; k++)
                        {
                            if (lnobj[k] == key)
                            {
                                chart->objs[chart->objs_size].note.end = 1;
                                break;
                            }
                        }
                        chart->objs_size++;
                        break;
                    case 109:
                    case 110:
                    case 111:
                    case 112:
                    case 113:
                    case 114:
                    case 115:
                    case 116:
                    case 117:
                    case 145:
                    case 146:
                    case 147:
                    case 148:
                    case 149:
                    case 150:
                    case 151:
                    case 152:
                    case 153:
                        while (chart->objs_size + 1 > nb_objs)
                        {
                            nb_objs *= 2;
                            chart->objs = realloc(chart->objs, nb_objs);
                        }
                        chart->objs[chart->objs_size].type = bms_OBJTYPE_INVISIBLE;
                        chart->objs[chart->objs_size].fraction = fraction;
                        chart->objs[chart->objs_size].time = 0;
                        chart->objs[chart->objs_size].misc.player = channel / 36 - 2;
                        chart->objs[chart->objs_size].misc.line = channel % 36;
                        chart->objs[chart->objs_size].misc.key = key;
                        chart->objs_size++;
                        break;
                    case 181:
                    case 182:
                    case 183:
                    case 184:
                    case 185:
                    case 186:
                    case 187:
                    case 188:
                    case 189:
                    case 217:
                    case 218:
                    case 219:
                    case 220:
                    case 221:
                    case 222:
                    case 223:
                    case 224:
                    case 225:
                        while (chart->objs_size + 1 > nb_objs)
                        {
                            nb_objs *= 2;
                            chart->objs = realloc(chart->objs, nb_objs);
                        }
                        chart->objs[chart->objs_size].type = bms_OBJTYPE_NOTE;
                        chart->objs[chart->objs_size].fraction = fraction;
                        chart->objs[chart->objs_size].time = 0;
                        chart->objs[chart->objs_size].note.player = channel / 36 - 4;
                        chart->objs[chart->objs_size].note.line = channel % 36;
                        chart->objs[chart->objs_size].note.key = key;
                        chart->objs[chart->objs_size].note.end = lnflag[(channel / 36 - 5) * 10 + channel % 36 - 1];
                        lnflag[(channel / 36 - 5) * 10 + channel % 36 - 1] = !lnflag[(channel / 36 - 5) * 10 + channel % 36 - 1];
                        chart->objs_size++;
                        break;
                    case 469:
                    case 470:
                    case 471:
                    case 472:
                    case 473:
                    case 474:
                    case 475:
                    case 476:
                    case 477:
                    case 505:
                    case 506:
                    case 507:
                    case 508:
                    case 509:
                    case 510:
                    case 511:
                    case 512:
                    case 513:
                        while (chart->objs_size + 1 > nb_objs)
                        {
                            nb_objs *= 2;
                            chart->objs = realloc(chart->objs, nb_objs);
                        }
                        chart->objs[chart->objs_size].type = bms_OBJTYPE_BOMB;
                        chart->objs[chart->objs_size].fraction = fraction;
                        chart->objs[chart->objs_size].time = 0;
                        chart->objs[chart->objs_size].misc.player = channel / 36 - 12;
                        chart->objs[chart->objs_size].misc.line = channel % 36;
                        chart->objs[chart->objs_size].misc.key = key;
                        chart->objs_size++;
                        break;
                    }
                }
            }
            free(str);
        }
    }

    free(line);

    free(skip);
    free(lnobj);
    free(bpms);
    free(stops);

    regfree(&randomRegex);
    regfree(&ifRegex);
    regfree(&elseRegex);
    regfree(&endifRegex);
    regfree(&genreRegex);
    regfree(&titleRegex);
    regfree(&nestedSubtitleRegex);
    regfree(&subtitleRegex);
    regfree(&artistRegex);
    regfree(&subartistRegex);
    regfree(&stagefileRegex);
    regfree(&bannerRegex);
    regfree(&playlevelRegex);
    regfree(&difficultyRegex);
    regfree(&totalRegex);
    regfree(&rankRegex);
    regfree(&wavsRegex);
    regfree(&bmpsRegex);
    regfree(&lnobjRegex);
    regfree(&bpmRegex);
    regfree(&bpmsRegex);
    regfree(&stopsRegex);
    regfree(&signatureRegex);
    regfree(&notesRegex);

    sort(chart->sectors, chart->sectors_size, sizeof(bms_Sector), sectcomp);
    for (i = 1; i < chart->sectors_size; i++)
    {
        bms_Sector *sector = chart->sectors + i;
        bms_Sector *prev = chart->sectors + i - 1;
        while (prev > chart->sectors && (prev->fraction < sector->fraction || prev->inclusive && prev->fraction == sector->fraction) && prev->delta > 0)
            prev--;
        sector->time = prev->time + (fractionDiff(chart->signatures, prev->fraction, sector->fraction) + sector->time) / prev->delta;
    }

    sort(chart->objs, chart->objs_size, sizeof(bms_Obj), objcomp);
    for (i = 0; i < chart->objs_size; i++)
    {
        bms_Obj *obj = chart->objs + i;
        bms_Sector *sector = chart->sectors + chart->sectors_size - 1;
        while (sector > chart->sectors && (sector->fraction < obj->fraction || sector->inclusive && sector->fraction == obj->fraction))
            sector--;
        obj->time = sector->time + fractionDiff(chart->signatures, sector->fraction, obj->fraction) / sector->delta;
    }
}

float bms_resolveFraction(const bms_Chart *chart, const float fraction)
{
    return fractionDiff(chart->signatures, 0, fraction);
}

float bms_timeToFraction(const bms_Chart *chart, const double time)
{
    bms_Sector *sector = chart->sectors + chart->sectors_size - 1;
    while (sector > chart->sectors && (sector->time < time || sector->inclusive && sector->time == time))
        sector--;
    return bms_resolveFraction(chart, sector->fraction) + (time - sector->time) * sector->delta;
}