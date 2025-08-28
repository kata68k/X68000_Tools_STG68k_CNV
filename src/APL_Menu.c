#ifndef	APL_MENU_C
#define	APL_MENU_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iocslib.h>
#include <doslib.h>

#include <usr_macro.h>

#include "IF_Draw.h"
#include "IF_Input.h"
#include "IF_Graphic.h"
#include "IF_Math.h"
#include "IF_Mouse.h"
#include "IF_PCG.h"
#include "APL_Menu.h"
#include "APL_PCG.h"
#include "APL_Score.h"

/* define定義 */
#define Menu_INDEX  (5)
#define Menu_POS_X  (14)
#define Menu_POS_Y  (256 + 64)

/* 関数のプロトタイプ宣言 */
void APL_Menu_Init(void);
void APL_Menu_Exit(void);
void APL_Menu_On(void);
void APL_Menu_Off(void);
static int16_t Menu_Set_Dummy(int16_t, int16_t);
int16_t APL_Menu_Mess(int16_t);
int16_t APL_Menu_HELP(void);
int16_t APL_Menu_CURSOR(void);
int16_t APL_Menu_Proc(void);

/* グローバル変数 */
static int8_t s_b_Updata = TRUE;

uint8_t	g_STG68K_mode;
uint8_t	g_STG68K_stage;
uint8_t	g_STG68K_PAL_num;
uint8_t	g_STG68K_View;
uint8_t	g_STG68K_save;

int16_t     g_WindowMMax1 = 0;
int16_t     g_WindowMMax2 = 0;


ST_MENU  stMenu;

ST_MENU_DATA    stMenu_Data[] = {
    /* Menu1 */
    {
        Menu_Set_Dummy,
        4,  /* 文字列 */
        &g_STG68K_mode,
        0,  /* 最小 */
        4,  /* 最大 */
        "パターン変更",
        "差し換え対象のパターンを選ぶ"
    },
    /* Menu2 */
    {
        Menu_Set_Dummy,
        1,  /* No.付き番号 */
        &g_STG68K_PAL_num,
        0,  /* 最小 */
        15, /* 最大 */
        "パレット番号",
        "差し換え対象のパレット番号を選ぶ"
    },
    /* Menu3 */
    {
        Menu_Set_Dummy,
        0,  /* プロセスの実行 */
        NULL,
        0,  /* 最小 */
        0,  /* 最大 */
        "画像を選ぶ",
        "差し換え対象の画像を選択"
    },
    /* Menu4 */
    {
        NULL,
        0,  /* プロセスの実行 */
        NULL,
        0,  /* 最小 */
        0,  /* 最大 */
        "保存して終了する",
        "保存して終了。"
    },
    /* Menu5 */
    {
        NULL,
        0,  /* プロセスの実行 */
        NULL,
        0,  /* 最小 */
        0,  /* 最大 */
        "終了する",
        "保存しないで終了。ESCキーで強制終了。"
    }
};


/* 関数 */
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
void APL_Menu_Init(void)
{
    int16_t loop;
    int16_t len, len_max, len_max2;
    ST_MENU  *p_stMenuWork = (ST_MENU *)&stMenu;
    ST_MENU_DATA  *p_stMenuDataWork = (ST_MENU_DATA *)&stMenu_Data;

    static int8_t bInit = TRUE;

    if(bInit == TRUE)
    {
        bInit = FALSE;
        p_stMenuWork->nSelectMenu = Menu_INDEX-1;
        p_stMenuWork->nNum = 0;
    }
    p_stMenuWork->nMenuX = Menu_POS_X;
    p_stMenuWork->nMenuY = Menu_POS_Y;
    s_b_Updata = TRUE;

    G_PAGE_SET(0b0001);	/* GR0 */

    /* Windowの幅を決めるために文字の最大の長さを測る */
    len_max = 0;
    len = 0;
    for(loop=0; loop < Menu_INDEX; loop++)
    {
        len = APL_Menu_Mess(loop);
        if(len_max < len)len_max = len;
    }
    g_WindowMMax1 = len_max;
    Draw_Fill(Menu_POS_X - 12, Menu_POS_Y - 12, Menu_POS_X + (g_WindowMMax1 * 6) + 8 - 1, Menu_POS_Y + (loop * 12) + 12, G16b_BLACK);
    Draw_Box(Menu_POS_X - 12 - 1, Menu_POS_Y - 12, Menu_POS_X + (g_WindowMMax1 * 6) + 8, Menu_POS_Y + (loop * 12) + 12, G16b_WHITE, 0xFFFF);
    for(loop=0; loop < Menu_INDEX; loop++)
    {
        APL_Menu_Mess(loop);
    }
    APL_Menu_CURSOR();

    len_max2 = 0;
    len = 0;
    for(loop=0; loop < Menu_INDEX; loop++)
    {
        len = strlen(p_stMenuDataWork[loop].sHelpMess);
        if(len_max2 < len)len_max2 = len;
    }
    g_WindowMMax2 = len_max2;
    Draw_Fill(Menu_POS_X - 12, Menu_POS_Y - 12 + 128, Menu_POS_X + (g_WindowMMax2 * 6) + 8, Menu_POS_Y + 12 + 12 + 128, G16b_BLACK);
    Draw_Box(Menu_POS_X - 12, Menu_POS_Y - 12 + 128, Menu_POS_X + (g_WindowMMax2 * 6) + 8, Menu_POS_Y + 12 + 12 + 128, G16b_WHITE, 0xFFFF);
    APL_Menu_HELP();
}
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
void APL_Menu_Exit(void)
{
	G_CLR_AREA(0, 256, 0, 256, 0);	/* ページ 0 */
}
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
void APL_Menu_On(void)
{
	int32_t nCrtMod;
	nCrtMod = _iocs_crtmod(-1);
	if( (nCrtMod >= 12) && (nCrtMod <= 15) )return;	/* 65536色はパレット変更不可にする */

	G_Scroll(0, 0, 0);
}
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
void APL_Menu_Off(void)
{
	int32_t nCrtMod;
	nCrtMod = _iocs_crtmod(-1);
	if( (nCrtMod >= 12) && (nCrtMod <= 15) )return;	/* 65536色はパレット変更不可にする */
    
	G_Scroll(256, 0, 0);
}
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
static int16_t Menu_Set_Dummy(int16_t nData, int16_t nPos)
{
    ST_MENU_DATA  *p_stMenuDataWork = (ST_MENU_DATA *)&stMenu_Data;

    switch(nData)
    {
        case 0:/* 実行 */
        {
            break;
        }
        case 1:/* 左の実行 */
        {
            if(*p_stMenuDataWork[nPos].pbSelect == p_stMenuDataWork[nPos].bSelectMin)
            {
                *p_stMenuDataWork[nPos].pbSelect = p_stMenuDataWork[nPos].bSelectMax;
            }
            else
            {
                *p_stMenuDataWork[nPos].pbSelect -= 1;
            }
            break;
        }
        case 2:/* 右の実行 */
        {
            if(*p_stMenuDataWork[nPos].pbSelect == p_stMenuDataWork[nPos].bSelectMax)
            {
                *p_stMenuDataWork[nPos].pbSelect = p_stMenuDataWork[nPos].bSelectMin;
            }
            else
            {
                *p_stMenuDataWork[nPos].pbSelect += 1;
            }
            break;
        }
        default:
        {
            break;
        }
    }

    return 0;
}
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t APL_Menu_Mess(int16_t nPos)
{
    int16_t ret = 0;

    int8_t str[128] = {0};

    ST_MENU_DATA  *p_stMenuDataWork = (ST_MENU_DATA *)&stMenu_Data;

    if(nPos < 0)return ret;

    G_PAGE_SET(0b0001);	/* GR0 */

    /* 設定 */
    if(p_stMenuDataWork[nPos].pbSelect != NULL)
    {
        switch(p_stMenuDataWork[nPos].nType)
        {
            case 0: /* プロセスの実行 */
            {
                break;
            }
            case 1: /* No.付き番号 */
            {
                sprintf( str, "%s = No.%d", p_stMenuDataWork[nPos].sMenuName, *p_stMenuDataWork[nPos].pbSelect );
                break;
            }
            case 2: /* No.なし番号 */
            {
                sprintf( str, "%s = %d", p_stMenuDataWork[nPos].sMenuName, *p_stMenuDataWork[nPos].pbSelect );
                break;
            }
            case 3: /* On/Off */
            {
                int8_t sOnOff[4] = {0};
                if(*p_stMenuDataWork[nPos].pbSelect == TRUE)
                {
                    strcpy( sOnOff, "ON");
                }
                else
                {
                    strcpy( sOnOff, "OFF");
                }
                sprintf( str, "%s = %s", p_stMenuDataWork[nPos].sMenuName, sOnOff );
                break;
            }
            case 4: /* 文字列1 */
            {
                int8_t sMes[6][64] = {"自機","敵","ボス","背景","スコアロゴ、スコア、残機",""};
                sprintf( str, "%s = %s", p_stMenuDataWork[nPos].sMenuName, sMes[*p_stMenuDataWork[nPos].pbSelect] );
                break;
            }
            case 5: /* 文字列2 */
            {
                int8_t sMes[2][64] = {"入力画像","出力画像"};
                sprintf( str, "%s = %s", p_stMenuDataWork[nPos].sMenuName, sMes[*p_stMenuDataWork[nPos].pbSelect] );
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else
    {
        sprintf( str, "%s", p_stMenuDataWork[nPos].sMenuName );
    }
    ret = strlen(str);

    Draw_Fill(Menu_POS_X, Menu_POS_Y + (nPos * 12), Menu_POS_X + (g_WindowMMax1 * 6) + 8 - 1, Menu_POS_Y + (nPos * 12) + 12, G16b_BLACK);
    PutGraphic_To_Symbol12(str, Menu_POS_X, Menu_POS_Y + (nPos * 12), G16b_WHITE);

    return ret;
}
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t APL_Menu_HELP(void)
{
    int16_t ret = 0; 
    ST_MENU  *p_stMenuWork = (ST_MENU *)&stMenu;
    ST_MENU_DATA  *p_stMenuDataWork = (ST_MENU_DATA *)&stMenu_Data;

    G_PAGE_SET(0b0001);	/* GR0 */

    /* HELP */
    PutGraphic_To_Symbol12(p_stMenuDataWork[p_stMenuWork->nSelectMenu_Old].sHelpMess, Menu_POS_X, Menu_POS_Y + 128, G16b_BLACK);
    PutGraphic_To_Symbol12(p_stMenuDataWork[p_stMenuWork->nSelectMenu].sHelpMess, Menu_POS_X, Menu_POS_Y + 128, G16b_WHITE);

    return ret;
}
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t APL_Menu_CURSOR(void)
{
    int16_t ret = 0; 
    ST_MENU  *p_stMenuWork = (ST_MENU *)&stMenu;

    G_PAGE_SET(0b0001);	/* GR0 */

    /* カーソル */
    PutGraphic_To_Symbol12("→" , Menu_POS_X - 12, Menu_POS_Y + (p_stMenuWork->nSelectMenu_Old * 12) , G16b_BLACK);
    PutGraphic_To_Symbol12("→" , Menu_POS_X - 12, Menu_POS_Y + (p_stMenuWork->nSelectMenu * 12) , G16b_WHITE);

    return ret;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t APL_Menu_Proc(void)
{
    int16_t ret = -1; 
    int16_t mess = -1; 
    static int8_t s_b_UP;
    static int8_t s_b_DOWN;
    static int8_t s_b_RIGHT;
    static int8_t s_b_LEFT;
    static int8_t s_b_A;
    static int8_t s_b_B;
    
    int8_t ms_x;
    int8_t ms_y;
    int8_t ms_left;
    int8_t ms_right;
    int32_t ms_pos_x;
    int32_t ms_pos_y;

    ST_MENU  *p_stMenuWork = (ST_MENU *)&stMenu;
    ST_MENU_DATA  *p_stMenuDataWork = (ST_MENU_DATA *)&stMenu_Data;

    /* マウス操作 */
    Mouse_GetDataPos(&ms_x, &ms_y, &ms_left, &ms_right);

    Mouse_GetPos(&ms_pos_x, &ms_pos_y);

    if(ms_left != 0)
    {
        SetInput(KEY_b_Z);
    }
    if(ms_right != 0)
    {
        SetInput(KEY_b_X);
    }

    /* ジョイスティック操作 */

    /* メニュー番号の実行 */
    if(	((GetInput_P1() & JOY_A ) != 0u)	||		/* A */
        ((GetInput() & KEY_b_Z) != 0u)		||		/* A(z) */
        ((GetInput() & KEY_b_SP ) != 0u)		)	/* スペースキー */
    {
        if(s_b_A == FALSE)
        {
            s_b_A = TRUE;
        }
        else
        {
        }
    }
    else
    {
        if(s_b_A == TRUE)
        {
            if(p_stMenuDataWork[p_stMenuWork->nSelectMenu].MenuFunc == NULL )
            {

            }
            else{
                p_stMenuWork->nNum = 0; /* 実行 */
                p_stMenuDataWork[p_stMenuWork->nSelectMenu].MenuFunc( p_stMenuWork->nNum, p_stMenuWork->nSelectMenu );
            }
            mess = p_stMenuWork->nSelectMenu;    /* どのメニューの実行結果なのか返す */
        }
        s_b_A = FALSE;
    }

    /* キャンセル */
    if(	((GetInput_P1() & JOY_B ) != 0u)	||		/* B */
        ((GetInput() & KEY_b_X) != 0u)	)			/* B(z) */
    {
        if(s_b_B == FALSE)
        {
            s_b_B = TRUE;
        }
    }
    else
    {
        s_b_B = FALSE;
    }

    /* メニュー番号の実行（各メニューの番号変更＆実行） */
    if(	((GetInput_P1() & JOY_LEFT ) != 0u )	||	/* LEFT */
        ((GetInput() & KEY_LEFT) != 0 )		)	/* 左 */
    {
        if(s_b_LEFT == FALSE)
        {
            s_b_LEFT = TRUE;

            if(p_stMenuDataWork[p_stMenuWork->nSelectMenu].MenuFunc == NULL )
            {

            }
            else{
                p_stMenuWork->nNum = 1; /* 左押下の実行 */
                p_stMenuDataWork[p_stMenuWork->nSelectMenu].MenuFunc( p_stMenuWork->nNum, p_stMenuWork->nSelectMenu );
                mess = p_stMenuWork->nSelectMenu;    /* どのメニューの実行結果なのか返す */
            }
        }
    }
    else if( ((GetInput_P1() & JOY_RIGHT ) != 0u )	||	/* RIGHT */
            ((GetInput() & KEY_RIGHT) != 0 )			)	/* 右 */
    {
        if(s_b_RIGHT == FALSE)
        {
            s_b_RIGHT = TRUE;

            if(p_stMenuDataWork[p_stMenuWork->nSelectMenu].MenuFunc == NULL )
            {

            }
            else{
                p_stMenuWork->nNum = 2; /* 右押下の実行 */
                p_stMenuDataWork[p_stMenuWork->nSelectMenu].MenuFunc( p_stMenuWork->nNum, p_stMenuWork->nSelectMenu );
                mess = p_stMenuWork->nSelectMenu;    /* どのメニューの実行結果なのか返す */
            }
        }
    }
    else
    {
        s_b_LEFT = FALSE;
        s_b_RIGHT = FALSE;
    }

    /* メニュー番号変更 */
    if(	((GetInput_P1() & JOY_UP ) != 0u )	||	/* UP */
        ((GetInput() & KEY_UPPER) != 0 )	)	/* 上 */
    {
        if(s_b_UP == FALSE)
        {
            s_b_UP = TRUE;

            p_stMenuWork->nSelectMenu_Old = p_stMenuWork->nSelectMenu;
            p_stMenuWork->nSelectMenu-=1;
            if(p_stMenuWork->nSelectMenu < 0)p_stMenuWork->nSelectMenu = Menu_INDEX - 1;

            APL_Menu_HELP();    /* ヘルプメッセージ更新 */
            APL_Menu_CURSOR();  /* カーソル更新 */
        }
    }
	else if(((GetInput_P1() & JOY_DOWN ) != 0u )	||	/* UP */
       		((GetInput() & KEY_LOWER) != 0 )	)	    /* 下 */
    {
        if(s_b_DOWN == FALSE)
        {
            s_b_DOWN = TRUE;

            p_stMenuWork->nSelectMenu_Old = p_stMenuWork->nSelectMenu;
            p_stMenuWork->nSelectMenu+=1;
            if(p_stMenuWork->nSelectMenu >= Menu_INDEX)p_stMenuWork->nSelectMenu = 0;
            
            APL_Menu_HELP();    /* ヘルプメッセージ更新 */
            APL_Menu_CURSOR();  /* カーソル更新 */
        }
    }
    else
    {
        s_b_UP = FALSE;
        s_b_DOWN = FALSE;

        if((ms_x != 0) || (ms_y != 0))
        {
            p_stMenuWork->nSelectMenu_Old = p_stMenuWork->nSelectMenu;
            p_stMenuWork->nSelectMenu = Mmax(0,Mmin(Menu_INDEX-1, ((ms_pos_y - Menu_POS_Y) / 12)));
            
            APL_Menu_HELP();    /* ヘルプメッセージ更新 */
            APL_Menu_CURSOR();  /* カーソル更新 */
        }
    }

    if( (s_b_UP == TRUE)    ||
        (s_b_DOWN == TRUE)  ||
        (s_b_RIGHT == TRUE) ||
        (s_b_LEFT == TRUE)  ||
        (s_b_A == TRUE)     ||
        (s_b_B == TRUE)     )
    {
        s_b_Updata = TRUE;
    }

    if( s_b_Updata == TRUE )
    {
        s_b_Updata = FALSE;
    }

    APL_Menu_Mess(mess);

    if(p_stMenuWork->nNum == 0)
    {
        ret = mess;
    }

    return ret;
}

#endif	/* APL_MENU_C */
