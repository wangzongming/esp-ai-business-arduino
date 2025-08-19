/**
 * @file lv_conf.h
 * Configuration file for LVGL v8.x
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   Graphical settings
 *====================*/

#define LV_HOR_RES_MAX          (240)
#define LV_VER_RES_MAX          (240) 
#define LV_COLOR_DEPTH          16
#define LV_COLOR_16_SWAP        0
#define LV_COLOR_SCREEN_TRANSP  0
#define LV_USE_GIF 1


/*====================
   Memory settings
 *====================*/
#define LV_MEM_CUSTOM         1 // 显示 GIF 时需要开起来 
#define LV_MEM_SIZE           (8 * 1024)         // 设置 LVGL 内部堆为 8KB 

/*====================
   HAL settings
 *====================*/

#define LV_TICK_CUSTOM          0
#define LV_DRAW_BUF_DOUBLE      0   /* Use double buffering? */

/*====================
   Feature usage
 *====================*/

#define LV_USE_LOG              1
#define LV_LOG_LEVEL            LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF           1
#define LV_USE_TIMER            1

 

 
/*====================
   Font usage
 *====================*/

// #define LV_FONT_MONTSERRAT_12   1
// #define LV_FONT_MONTSERRAT_14   1
// #define LV_FONT_MONTSERRAT_16   1 
// #define LV_FONT_DEFAULT &lv_font_montserrat_14
#define LV_USE_FONT_COMPRESSED 1  /* 启用字体压缩支持 */ 


#define LV_FONT_MONTSERRAT_8     0
#define LV_FONT_MONTSERRAT_10    1
#define LV_FONT_MONTSERRAT_12    0
#define LV_FONT_MONTSERRAT_14    1
#define LV_FONT_MONTSERRAT_16    0
#define LV_FONT_MONTSERRAT_18    0
#define LV_FONT_MONTSERRAT_20    0
#define LV_FONT_MONTSERRAT_22    0
#define LV_FONT_MONTSERRAT_24    1
#define LV_FONT_MONTSERRAT_26    0
#define LV_FONT_MONTSERRAT_28    0
#define LV_FONT_MONTSERRAT_30    0
#define LV_FONT_MONTSERRAT_32    0
#define LV_FONT_MONTSERRAT_34    0
#define LV_FONT_MONTSERRAT_36    0
#define LV_FONT_MONTSERRAT_38    0
#define LV_FONT_MONTSERRAT_40    0
#define LV_FONT_MONTSERRAT_42    0
#define LV_FONT_MONTSERRAT_44    0
#define LV_FONT_MONTSERRAT_46    0
#define LV_FONT_MONTSERRAT_48    0
#define LV_FONT_MONTSERRAT_12_SUBPX 0
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0
#define LV_FONT_UNSCII_8     0
#define LV_FONT_UNSCII_16    0

#define LV_USE_FREETYPE         0

/*====================
   Text settings
 *====================*/

#define LV_TXT_ENC              LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS      " ,.;:-_"

#define LV_TXT_LINE_BREAK_LONG_LEN  20
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3

/*====================
   Themes
 *====================*/

#define LV_USE_THEME_DEFAULT    1
#define LV_THEME_DEFAULT_DARK   0
#define LV_THEME_DEFAULT_GROW   1
#define LV_THEME_DEFAULT_FONT   &lv_font_montserrat_14


/*====================
   文件系统
 *====================*/
#define LV_USE_FS_STDIO 0
#define LV_USE_FS_POSIX 0
#define LV_USE_FS_WIN32 0


/*====================
   UI components
 *====================*/
// #define LV_USE_ARC        0
// #define LV_USE_BAR        0
// #define LV_USE_BTN        0
// #define LV_USE_BTNMATRIX  0
// #define LV_USE_CANVAS     0
// #define LV_USE_CHECKBOX   0
// #define LV_USE_DROPDOWN   0
// #define LV_USE_IMG        1
// #define LV_USE_LABEL      1
// #define LV_USE_LINE       0
// #define LV_USE_ROLLER     0
// #define LV_USE_SLIDER     0
// #define LV_USE_SWITCH     0
// #define LV_USE_TEXTAREA   0
// #define LV_USE_TABLE      0



/*====================
   LVGL 的某些部件
 *====================*/

// #define LV_USE_CALENDAR      0
// #define LV_USE_CHART         0
// #define LV_USE_COLORWHEEL    0
// #define LV_USE_IMGBTN        0
// #define LV_USE_KEYBOARD      0
// #define LV_USE_LED           0
// #define LV_USE_LIST          0
// #define LV_USE_MENU          0
// #define LV_USE_METER         0
// #define LV_USE_MSGBOX        0
// #define LV_USE_SPINBOX       0
// #define LV_USE_SPINNER       0
// #define LV_USE_TABVIEW       0
// #define LV_USE_TILEVIEW      0
// #define LV_USE_WIN           0
 

#endif // LV_CONF_H
