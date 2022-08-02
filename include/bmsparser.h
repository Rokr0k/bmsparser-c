#ifndef __BMSPARSER_BMSPARSER_H__
#define __BMSPARSER_BMSPARSER_H__

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Types of objects.
     */
    typedef enum bms_Obj_Type
    {
        /**
         * Channel 01.
         * Use bgm
         */
        bms_OBJTYPE_BGM,

        /**
         * Channel 04, 06, 07.
         * use bmp
         */
        bms_OBJTYPE_BMP,

        /**
         * Channel 11~19, 21~29, 51~59, 61~69.
         * use note
         */
        bms_OBJTYPE_NOTE,

        /**
         * Channel 31~39, 41~49.
         * use misc
         */
        bms_OBJTYPE_INVISIBLE,

        /**
         * Channel D1~D9, E1~E9.
         * use misc
         */
        bms_OBJTYPE_BOMB,
    } bms_Obj_Type;

    /**
     * Object structure.
     */
    typedef struct bms_Obj
    {
        /**
         * Type of the object.
         */
        bms_Obj_Type type;

        /**
         * Position in measure (Unresolved).
         */
        float fraction;

        /**
         * Time when it will be executed in seconds.
         */
        double time;

        union
        {
            /**
             * Structure for BGM Object.
             */
            struct
            {
                /**
                 * WAV to play.
                 */
                int key;
            } bgm;

            /**
             * Structure for BMP Object.
             */
            struct
            {
                /**
                 * BMP to show.
                 */
                int key;

                /**
                 * Layer of the object.
                 * -1: Poor.
                 * 0: BGA Base.
                 * 1: BGA Layer.
                 */
                int layer;
            } bmp;

            /**
             * Structure for Note Object.
             */
            struct
            {
                /**
                 * Player of the note.
                 */
                int player;

                /**
                 * Line of the note.
                 */
                int line;

                /**
                 * WAV to play.
                 */
                int key;

                /**
                 * Whether it is the end of a long note.
                 */
                int end;
            } note;

            /**
             * Structure for Invisible Object and Bomb Object.
             */
            struct
            {
                /**
                 * Player of the object.
                 */
                int player;

                /**
                 * Line of the object.
                 */
                int line;

                /**
                 * For Invisible Object, WAV to play.
                 * For Bomb Object, amount of the damage.
                 */
                int key;
            } misc;
        };
    } bms_Obj;

    /**
     * Sector Structure.
     */
    typedef struct bms_Sector
    {
        /**
         * Position in measure (Unresolved).
         */
        float fraction;

        /**
         * Time when it will be started in seconds.
         */
        double time;

        /**
         * BPM / 60s / 4
         * Can be zero (stopped).
         */
        double delta;

        /**
         * Whether it is inclusive or not when searching.
         */
        unsigned char inclusive;
    } bms_Sector;

    /**
     * Difficulty
     */
    typedef enum bms_Difficulty
    {
        /**
         * Beginner, Easy, etc.
         */
        bms_DIFFICULTY_BEGINNER = 1,
        /**
         * Normal, Standard, etc.
         */
        bms_DIFFICULTY_NORMAL,
        /**
         * Hyper, Hard, etc.
         */
        bms_DIFFICULTY_HYPER,
        /**
         * Another, Extra, etc.
         */
        bms_DIFFICULTY_ANOTHER,
        /**
         * Insane.
         */
        bms_DIFFICULTY_INSANE,
    } bms_Difficulty;

    /**
     * Rank
     */
    typedef enum bms_Rank
    {
        /**
         * Very Hard.
         * &PlusMinus;8ms in LR2.
         */
        bms_RANK_VERYHARD,

        /**
         * Hard.
         * &PlusMinus;15ms in LR2.
         */
        bms_RANK_HARD,

        /**
         * Normal.
         * &PlusMinus;18ms in LR2.
         */
        bms_RANK_NORMAL,

        /**
         * Easy.
         * &PlusMinus;21ms in LR2.
         */
        bms_RANK_EASY,
    } bms_Rank;

    /**
     * Chart Structure
     */
    typedef struct bms_Chart
    {
        /**
         * Genre
         * Parsed from `#GENRE`
         */
        char *genre;

        /**
         * Title
         * Parsed from `#TITLE`
         */
        char *title;

        /**
         * Subtitle
         * Parsed from `#SUBTITLE` or `#TITLE`
         */
        char *subtitle;

        /**
         * Artist
         * Parsed from `#ARTIST`
         */
        char *artist;

        /**
         * Subrtist
         * Parsed from `#SUBATIST`
         */
        char *subartist;

        /**
         * Stagefile
         * Joined with parent directory.
         * Parsed from `#STAGEFILE`
         */
        char *stagefile;

        /**
         * Banner
         * Joined with parent directory.
         * Parsed from `#BANNER`
         */
        char *banner;

        /**
         * Play Level
         * Parsed from `#PLAYLEVEL`
         */
        int playlevel;

        /**
         * Difficulty
         * Parsed from `#DIFFICULTY`
         */
        bms_Difficulty difficulty;

        /**
         * Total
         * Parsed from `#TOTAL`
         */
        float total;

        /**
         * Rank
         * Parsed from `#RANK`
         */
        bms_Rank rank;

        /**
         * WAV list
         * Parsed from `#WAVxx`
         */
        char **wavs;

        /**
         * BMP list
         * Parsed from `#BMPxx`
         */
        char **bmps;

        /**
         * Signatures
         * Parsed from Channel 02.
         */
        float *signatures;

        /**
         * Objects
         */
        bms_Obj *objs;

        /**
         * Size of the bjects
         */
        size_t objs_size;

        /**
         * Sectors
         * Parsed from Channel 03, 08, 09.
         */
        bms_Sector *sectors;

        /**
         * Size of the sectors
         */
        size_t sectors_size;
    } bms_Chart;

    /**
     * Allocate Chart Structure.
     * @return Allocated chart
     */
    bms_Chart *bms_alloc();

    /**
     * Free Chart Structure.
     * @param chart Chart to free
     */
    void bms_free(bms_Chart *chart);

    /**
     * Parse BMS contents from file.
     * @param chart Chart to fill with BMS contents
     * @param file File to get BMS contents from
     */
    void bms_parse(bms_Chart *chart, FILE *file);

    /**
     * Resolve unresolved fraction.
     * @param chart Chart
     * @param fraction Unresolved fraction
     * @return Resolved fraction
     */
    float bms_resolveFraction(const bms_Chart *chart, const float fraction);

    /**
     * Convert time to fraction
     * @param chart Chart
     * @param time Time
     * @return Resolved fraction
     */
    float bms_timeToFraction(const bms_Chart *chart, const double time);

#ifdef __cplusplus
}
#endif

#endif