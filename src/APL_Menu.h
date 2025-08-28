#ifndef	APL_MENU_H
#define	APL_MENU_H

#include <usr_define.h>

typedef struct
{
    int16_t     (*MenuFunc)(int16_t, int16_t);    /* 実行関数 */
    int16_t     nType;                            /* メニュー種類 */
    int8_t*     pbSelect;                         /* メニュー選択内容 */
    int8_t      bSelectMin;                       /* メニュー選択内容(最小) */
    int8_t      bSelectMax;                       /* メニュー選択内容(最大) */
    int8_t*     sMenuName;                        /* メニュー名 */
    int8_t*     sHelpMess;                        /* メニュー選択時のヘルプ */
}   ST_MENU_DATA;

typedef struct
{
    ST_PCG      stPCG;                  /* カーソル */
	int16_t	    nSelectMenu;		    /* メニュー番号 */
	int16_t	    nSelectMenu_Old;		/* メニュー番号(前回値) */
	int8_t		sDispMess[64];	        /* ステータス */
	int16_t		nNum;		        	/* ナンバー */
	int16_t		nMenuX;		        	/* x座標 */
	int16_t		nMenuY;	        		/* y座標 */
}	ST_MENU;

extern uint8_t	g_STG68K_mode;
extern uint8_t	g_STG68K_stage;
extern uint8_t	g_STG68K_PAL_num;
extern uint8_t	g_STG68K_View;
extern uint8_t	g_STG68K_save;

extern void APL_Menu_Init(void);
extern void APL_Menu_Exit(void);
extern void APL_Menu_On(void);
extern void APL_Menu_Off(void);
extern int16_t APL_Menu_Mess(int16_t);
extern int16_t APL_Menu_Proc(void);

#endif	/* APL_MENU_H */
