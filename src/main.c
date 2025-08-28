#ifndef	MAIN_C
#define	MAIN_C

#include <iocslib.h>
#include <stdio.h>
#include <stdlib.h>
#include <doslib.h>
#include <io.h>
#include <math.h>
#include <time.h>
#include <interrupt.h>

#include <usr_macro.h>
#include <apicglib.h>

#include "main.h"

#include "BIOS_CRTC.h"
#include "BIOS_DMAC.h"
#include "BIOS_MFP.h"
#include "BIOS_PCG.h"
#include "BIOS_MPU.h"
#include "IF_Draw.h"
#include "IF_FileManager.h"
#include "IF_Graphic.h"
#include "IF_Input.h"
#include "IF_MACS.h"
#include "IF_Math.h"
#include "IF_Memory.h"
#include "IF_Mouse.h"
#include "IF_Music.h"
#include "IF_PCG.h"
#include "IF_Text.h"
#include "IF_Task.h"
#include "APL_Menu.h"
#include "APL_PCG.h"
#include "APL_Score.h"

//#define 	W_BUFFER	/* ダブルバッファリングモード */
//#define	FPS_MONI	/* FPS計測 */
//#define	CPU_MONI	/* CPU計測 */
#define	MEM_MONI	/* MEM計測 */

enum
{
	STG_SM_SP,
	STG_SM_PAL,
	STG_68_PTN,
	STG_68_COL,
	STG_68_PAL,
};

enum
{
	VIEW_SP,
	VIEW_STG68K,
};

enum
{
	GR_Screen0,
	GR_Screen1,
};


/* グローバル変数 */
uint32_t	g_unTime_cal = 0u;
uint32_t	g_unTime_cal_PH = 0u;
uint32_t	g_unTime_Pass = 0xFFFFFFFFul;
int32_t		g_nCrtmod = 0;
int32_t		g_nMaxUseMemSize;
int16_t		g_CpuTime = 0;
uint8_t		g_mode;
uint8_t		g_mode_rev;
uint8_t		g_Vwait = 1;
uint8_t		g_bFlip = FALSE;
uint8_t		g_bFlip_old = FALSE;
uint16_t	g_unFrameCount;
#ifdef FPS_MONI
uint8_t		g_bFPS_PH = 0u;
#endif
uint8_t		g_bExit = TRUE;
int16_t		g_GameSpeed;

uint8_t		g_ubDemoPlay = FALSE;
uint8_t		g_ubPhantomX = FALSE;

/* グローバル構造体 */
#ifdef DEBUG	/* デバッグコーナー */
uint16_t	g_uDebugNum = 0;
//uint16_t	g_uDebugNum = (Bit_7|Bit_4|Bit_0);
#endif
int8_t	g_sFileName[5][32] = {0};
int32_t	g_nFileSize[5] = {0};
int8_t	*g_pSP_Buf;
int8_t	*g_pPAL_Buf;
int8_t	*g_pSTG68_PTN_Buf;
int8_t	*g_pSTG68_COL_Buf;
int8_t	*g_pSTG68_PAL_Buf;

enum
{
	STG68K_SHIP,
	STG68K_ENEMTY,
	STG68K_BOSS,
	STG68K_BG,
	STG68K_SCORE,
	STG68K_MAX,
};

int32_t g_nSTG68_PTN_MAP[] = {
	0x14000,	/* 自機 */
	0x08000,	/* 敵 */
	0x0E000,	/* ボス */
	0x00000,	/* 背景 */
	0x14000,	/* スコア */
};

int32_t g_nSTG68_PAL_MAP[] = {
	0x280,	/* 自機 */
	0x100,	/* 敵 */
	0x1C0,	/* ボス */
	0x000,	/* 背景 */
	0x280,	/* スコア */
};

const int16_t nTboxTable_X[4] = { 32, 32, 16, 16 };
const int16_t nTboxTable_Y[4] = { 32, 16, 32, 16 };
int16_t nTboxTableCount = 0;
int16_t	g_nTboxCursorPosX[2] = {0};
int16_t	g_nTboxCursorPosY[2] = {0};
int16_t	g_nTboxCursorPosX_ofst[2] = {0, 256};
int16_t	g_nTboxCursorWidth = 32;
int16_t	g_nTboxCursorHeight = 32;
int16_t	g_nTboxCursorPosX_O[2] = {0};
int16_t	g_nTboxCursorPosY_O[2] = {0};
int16_t	g_nTboxCursorWidth_O = 32;
int16_t	g_nTboxCursorHeight_O = 32;

/* 関数のプロトタイプ宣言 */
int16_t main(int16_t, int8_t**);
static void App_Init(void);
static void App_exit(void);
int16_t	BG_main(uint32_t);
void App_TimerProc( void );
int16_t App_RasterProc( uint16_t * );
void App_VsyncProc( void );
void App_HsyncProc( void );
int16_t	App_FlipProc( void );
int16_t	SetFlip(uint8_t);
int16_t	GetGameMode(uint8_t *);
int16_t	SetGameMode(uint8_t);

int16_t STG68K_SPPAL_to_PTNCOLPAL_Cnv(int16_t);

void (*usr_abort)(void);	/* ユーザのアボート処理関数 */

/* 関数 */
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t main_Task(void)
{
	int16_t	ret = 0;
	uint16_t	uFreeRunCount=0u;
#ifdef DEBUG	/* デバッグコーナー */
	uint32_t	unTime_cal = 0u;
	uint32_t	unTime_cal_PH = 0u;
#endif

	int16_t	loop;
	int16_t	pushCount = 0;

	uint8_t	bMode;
	uint8_t	bViewMode = VIEW_SP;
	
	ST_TASK		stTask = {0}; 
	ST_CRT		stCRT;
	
	/* 変数の初期化 */
#ifdef W_BUFFER
	SetGameMode(1);
#else
	SetGameMode(0);
#endif	
	loop = 1;
	g_unFrameCount = 0;
	
	/* 乱数 */
	srandom(TIMEGET());	/* 乱数の初期化 */
	
	do	/* メインループ処理 */
	{
		uint32_t time_st;
#if 0
		ST_CRT	stCRT;
#endif
#ifdef DEBUG	/* デバッグコーナー */
		uint32_t time_now;
		GetNowTime(&time_now);
#endif

#ifdef DEBUG
//		Draw_Box(	stCRT.hide_offset_x + X_POS_MIN - 1,
//					stCRT.hide_offset_y + Y_POS_BD + 1,
//					stCRT.hide_offset_x + X_POS_MAX - 1,
//					stCRT.hide_offset_y + g_TB_GameLevel[g_nGameLevel] + 1,
//					G_PURPLE, 0xFFFF);
#endif
		PCG_START_SYNC(g_Vwait);	/* 同期開始 */

		/* 終了処理 */
		if(loop == 0)
		{
			break;
		}
		
		/* 時刻設定 */
		GetNowTime(&time_st);	/* メイン処理の開始時刻を取得 */
		SetStartTime(time_st);	/* メイン処理の開始時刻を記憶 */
		
		/* タスク処理 */
		TaskManage();				/* タスクを管理する */
		GetTaskInfo(&stTask);		/* タスクの情報を得る */

		/* モード引き渡し */
		GetGameMode(&bMode);
		GetCRT(&stCRT, bMode);	/* 画面座標取得 */

#if CNF_VDISP
		/* V-Syncで入力 */
#else
		if(Input_Main(g_ubDemoPlay) != 0u) 	/* 入力処理 */
		{
		}
#endif

#ifdef DEBUG	/* デバッグコーナー */
//		DirectInputKeyNum(&g_uDebugNum, 3);	/* キーボードから数字を入力 */
#endif

		if((GetInput() & KEY_b_ESC ) != 0u)	/* ＥＳＣキー */
		{
			/* 終了 */
			pushCount = Minc(pushCount, 1);
			if(pushCount >= 5)
			{
				SetTaskInfo(SCENE_EXIT);	/* 終了シーンへ設定 */
			}
		}
		else if((GetInput() & KEY_b_HELP ) != 0u)	/* HELPキー */
		{
			if( (stTask.bScene != SCENE_GAME_OVER_S) && (stTask.bScene != SCENE_GAME_OVER) )
			{
				/* リセット */
				pushCount = Minc(pushCount, 1);
				if(pushCount >= 6)
				{

				}
				else if(pushCount >= 5)
				{
					SetTaskInfo(SCENE_INIT);	/* 終了シーンへ設定 */
				}
			}
		}		
		else if((GetInput() & KEY_b_Q ) != 0u)	/* Qキー */
		{
			if( (stTask.bScene != SCENE_GAME_OVER_S) && (stTask.bScene != SCENE_GAME_OVER) )
			{
				/* リセット */
				pushCount = Minc(pushCount, 1);
				if(pushCount >= 6)
				{

				}
				else if(pushCount >= 5)
				{
					SetTaskInfo(SCENE_EXIT);	/* 終了シーンへ設定 */
				}
			}
		}		
		else
		{
			pushCount = 0;
		}

		switch(stTask.bScene)
		{
			case SCENE_INIT:	/* 初期化シーン */
			{
				T_Clear();		/* テキストクリア */

				g_STG68K_mode = STG68K_SHIP;
				g_STG68K_stage = 0;	
				g_STG68K_PAL_num = 0;
				g_STG68K_View = 0;
				g_STG68K_save = 0;

				G_SP_to_GR_Load( g_pSP_Buf, g_nFileSize[STG_SM_SP], (int16_t *)((uint32_t)g_pPAL_Buf + (g_STG68K_PAL_num * sizeof(int16_t) * 16)), g_nFileSize[STG_SM_PAL], 0, 0, GR_Screen0);

				G_STG68K_to_GR_Load((int8_t *)((int32_t)(g_pSTG68_PTN_Buf + g_nSTG68_PTN_MAP[g_STG68K_mode])), g_nFileSize[STG_68_PTN], 
									(int16_t *)((int32_t)(g_pSTG68_COL_Buf)), g_nFileSize[STG_68_COL], 
									(int8_t *)((int32_t)(g_pSTG68_PAL_Buf + g_nSTG68_PAL_MAP[g_STG68K_mode])), g_nFileSize[STG_68_PAL], 
									256, 0, GR_Screen0, g_STG68K_mode );

				SetTaskInfo(SCENE_TITLE_S);	/* シーン(開始処理)へ設定 */
				break;
			}
			/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
			case SCENE_TITLE_S:
			{
				G_PaletteSetZero();

				APL_Menu_Init();	/* メニュー初期化 */
			    APL_Menu_On();	/* メニュー表示 */

				SetTaskInfo(SCENE_TITLE);	/* シーン(処理)へ設定 */
				break;
			}
			case SCENE_TITLE:
			{
				int16_t sw = 0;

				sw = APL_Menu_Proc();	/* メニュー */
				switch(sw)
				{
					case 0:	/* 編集対象の変更 */
					{
						SetTaskInfo(SCENE_TITLE_E);	/* シーン設定 */
						break;
					}
					case 1:	/* パレット番号の変更 */
					{
						g_STG68K_View = 0;
						SetTaskInfo(SCENE_TITLE_E);	/* シーン設定 */
						break;
					}
					case 2:	/* 選択 */
					{
						g_STG68K_View = 0;
						SetTaskInfo(SCENE_GAME1_S);	/* シーン設定 */
						break;
					}
					case 3:	/* 終了(保存あり) */
					{
						g_STG68K_View = 1;
						SetTaskInfo(SCENE_GAME_OVER_S);	/* シーン設定 */
						break;
					}
					case 4:	/* 終了(保存なし) */
					{
						g_STG68K_View = 1;
						SetTaskInfo(SCENE_GAME_OVER_E);	/* シーン設定 */
						break;
					}
					default:
					{
						/* 何もしない */
						break;
					}
				}
				break;
			}
			case SCENE_TITLE_E:
			{
				SetTaskInfo(SCENE_START_S);	/* シーン(処理)へ設定 */
				break;
			}
			/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
			case SCENE_START_S:
			{
				APL_Menu_Off();	/* メニューを消す */
				G_PaletteSetZero();

				/* 入力表示 */
				{
//					puts("G_SP_to_GR_Load");
					G_SP_to_GR_Load( g_pSP_Buf, g_nFileSize[STG_SM_SP], (int16_t *)((uint32_t)g_pPAL_Buf + (g_STG68K_PAL_num * sizeof(int16_t) * 16)), g_nFileSize[STG_SM_PAL], 0, 0, GR_Screen0);
				}
				/* 出力表示 */
				{
					G_STG68K_to_GR_Load((int8_t *)((int32_t)(g_pSTG68_PTN_Buf + g_nSTG68_PTN_MAP[g_STG68K_mode])), g_nFileSize[STG_68_PTN], 
										(int16_t *)((int32_t)(g_pSTG68_COL_Buf)), g_nFileSize[STG_68_COL], 
										(int8_t *)((int32_t)(g_pSTG68_PAL_Buf + g_nSTG68_PAL_MAP[g_STG68K_mode])), g_nFileSize[STG_68_PAL], 
										256, 0, GR_Screen0, g_STG68K_mode );
				}

				SetTaskInfo(SCENE_START);	/* シーン設定 */
				break;
			}
			case SCENE_START:
			{
				SetTaskInfo(SCENE_START_E);	/* シーン設定 */
				break;
			}
			case SCENE_START_E:
			{
				SetTaskInfo(SCENE_TITLE_S);	/* シーン(処理)へ設定 */
				break;
			}
			/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
			case SCENE_GAME1_S:
			{
				APL_Menu_Off();	/* メニューを消す */
				bViewMode = VIEW_SP;

				G_SP_to_GR_Load( g_pSP_Buf, g_nFileSize[STG_SM_SP], (int16_t *)((uint32_t)g_pPAL_Buf + (g_STG68K_PAL_num * sizeof(int16_t) * 16)), g_nFileSize[STG_SM_PAL], 0, 0, GR_Screen0);
				
				T_Box(g_nTboxCursorPosX_O[VIEW_SP] + g_nTboxCursorPosX_ofst[VIEW_SP], g_nTboxCursorPosY_O[VIEW_SP], g_nTboxCursorWidth_O, g_nTboxCursorHeight_O, 0xFFFF, T_BLACK1);
				T_Box(g_nTboxCursorPosX[VIEW_SP] + g_nTboxCursorPosX_ofst[VIEW_SP], g_nTboxCursorPosY[VIEW_SP], g_nTboxCursorWidth, g_nTboxCursorHeight, 0xFFFF, T_YELLOW);
				
				SetTaskInfo(SCENE_GAME1);	/* シーン(処理)へ設定 */
				break;
			}
			case SCENE_GAME2_S:
			{
				APL_Menu_Off();	/* メニューを消す */
				bViewMode = VIEW_STG68K;

				G_STG68K_to_GR_Load((int8_t *)((int32_t)(g_pSTG68_PTN_Buf + g_nSTG68_PTN_MAP[g_STG68K_mode])), g_nFileSize[STG_68_PTN], 
									(int16_t *)((int32_t)(g_pSTG68_COL_Buf)), g_nFileSize[STG_68_COL], 
									(int8_t *)((int32_t)(g_pSTG68_PAL_Buf + g_nSTG68_PAL_MAP[g_STG68K_mode])), g_nFileSize[STG_68_PAL], 
									256, 0, GR_Screen0, g_STG68K_mode );
				
				T_Box(g_nTboxCursorPosX_O[VIEW_STG68K] + g_nTboxCursorPosX_ofst[VIEW_STG68K], g_nTboxCursorPosY_O[VIEW_STG68K], g_nTboxCursorWidth_O, g_nTboxCursorHeight_O, 0xFFFF, T_BLACK1);
				T_Box(g_nTboxCursorPosX[VIEW_STG68K] + g_nTboxCursorPosX_ofst[VIEW_STG68K], g_nTboxCursorPosY[VIEW_STG68K], g_nTboxCursorWidth, g_nTboxCursorHeight, 0xFFFF, T_YELLOW);
				
				SetTaskInfo(SCENE_GAME2);	/* シーン(処理)へ設定 */
				break;
			}
			case SCENE_GAME1:
			case SCENE_GAME2:
			{
				static int8_t s_b_UP;
				static int8_t s_b_DOWN;
				static int8_t s_b_RIGHT;
				static int8_t s_b_LEFT;
				static int8_t s_b_A;
				static int8_t s_b_B;
				static int8_t s_b_Updata;

				int8_t ms_x;
				int8_t ms_y;
				int8_t ms_left;
				int8_t ms_right;
				int32_t ms_pos_x;
				int32_t ms_pos_y;

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
						if(stTask.bScene == SCENE_GAME1)SetTaskInfo(SCENE_GAME1_E);	/* シーン(処理)へ設定 */
						if(stTask.bScene == SCENE_GAME2)SetTaskInfo(SCENE_GAME2_E);	/* シーン(処理)へ設定 */
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
						
						if(s_b_A == TRUE)
						{
							if(stTask.bScene == SCENE_GAME1)SetTaskInfo(SCENE_GAME1_E);	/* シーン(処理)へ設定 */
							if(stTask.bScene == SCENE_GAME2)SetTaskInfo(SCENE_GAME2_E);	/* シーン(処理)へ設定 */
						}
						else
						{
							if(g_STG68K_mode == STG68K_SHIP)
							{
								nTboxTableCount = 0;
							}
							else
							{
								nTboxTableCount++;
								if(nTboxTableCount >= 4)
								{
									nTboxTableCount = 0;
								}
							}
							g_nTboxCursorWidth  = nTboxTable_X[nTboxTableCount];
							g_nTboxCursorHeight = nTboxTable_Y[nTboxTableCount];
						}
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

						g_nTboxCursorPosX[bViewMode] -= 16;
						if(g_nTboxCursorPosX[bViewMode] <= 0)
						{
							g_nTboxCursorPosX[bViewMode] = 0;
						}
					}
				}
				else if( ((GetInput_P1() & JOY_RIGHT ) != 0u )	||	/* RIGHT */
						((GetInput() & KEY_RIGHT) != 0 )			)	/* 右 */
				{
					if(s_b_RIGHT == FALSE)
					{
						s_b_RIGHT = TRUE;

						g_nTboxCursorPosX[bViewMode] += 16;
						if(g_nTboxCursorPosX[bViewMode] >= g_nTboxCursorPosX[bViewMode] + g_nTboxCursorWidth)
						{
							g_nTboxCursorPosX[bViewMode] = g_nTboxCursorPosX[bViewMode] + g_nTboxCursorWidth;
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

						g_nTboxCursorPosY[bViewMode] -= 16;
						if(g_nTboxCursorPosY[bViewMode] <= 0)
						{
							g_nTboxCursorPosY[bViewMode] = 0;
						}
					}
				}
				else if(((GetInput_P1() & JOY_DOWN ) != 0u )	||	/* UP */
						((GetInput() & KEY_LOWER) != 0 )	)	    /* 下 */
				{
					if(s_b_DOWN == FALSE)
					{
						s_b_DOWN = TRUE;

						g_nTboxCursorPosY[bViewMode] += 16;
						if(g_nTboxCursorPosY[bViewMode] >= g_nTboxCursorPosY[bViewMode] + g_nTboxCursorHeight)
						{
							g_nTboxCursorPosY[bViewMode] = g_nTboxCursorPosY[bViewMode] + g_nTboxCursorHeight;
						}
					}
				}
				else
				{
					s_b_UP = FALSE;
					s_b_DOWN = FALSE;

					if((ms_x != 0) || (ms_y != 0))
					{
//						T_Box(g_nTboxCursorPosX_O[bViewMode], g_nTboxCursorPosY_O[bViewMode], g_nTboxCursorWidth_O, g_nTboxCursorHeight_O, 0x5A5A, T_BLACK1);
//						T_Box(g_nTboxCursorPosX[bViewMode], g_nTboxCursorPosY[bViewMode], g_nTboxCursorWidth, g_nTboxCursorHeight, 0x5A5A, T_YELLOW);
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
					T_Box(g_nTboxCursorPosX_O[bViewMode] + g_nTboxCursorPosX_ofst[bViewMode], g_nTboxCursorPosY_O[bViewMode], g_nTboxCursorWidth_O, g_nTboxCursorHeight_O, 0xFFFF, T_BLACK1);
					T_Box(g_nTboxCursorPosX[bViewMode] + g_nTboxCursorPosX_ofst[bViewMode], g_nTboxCursorPosY[bViewMode], g_nTboxCursorWidth, g_nTboxCursorHeight, 0xFFFF, T_YELLOW);
					g_nTboxCursorPosX_O[bViewMode] = g_nTboxCursorPosX[bViewMode];
					g_nTboxCursorPosY_O[bViewMode] = g_nTboxCursorPosY[bViewMode];
					g_nTboxCursorWidth_O = g_nTboxCursorWidth;
					g_nTboxCursorHeight_O = g_nTboxCursorHeight;
					s_b_Updata = FALSE;
				}
				break;
			}
			case SCENE_GAME1_E:
			{
				T_Box(g_nTboxCursorPosX_O[VIEW_SP] + g_nTboxCursorPosX_ofst[VIEW_SP], g_nTboxCursorPosY_O[VIEW_SP], g_nTboxCursorWidth_O, g_nTboxCursorHeight_O, 0xFFFF, T_BLACK1);

				g_STG68K_View = 1;

				SetTaskInfo(SCENE_GAME2_S);	/* シーン(処理)へ設定 */
				break;
			}
			case SCENE_GAME2_E:
			{
				T_Box(g_nTboxCursorPosX_O[VIEW_STG68K] + g_nTboxCursorPosX_ofst[VIEW_STG68K], g_nTboxCursorPosY_O[VIEW_STG68K], g_nTboxCursorWidth_O, g_nTboxCursorHeight_O, 0xFFFF, T_BLACK1);

				STG68K_SPPAL_to_PTNCOLPAL_Cnv(g_STG68K_mode);

				g_STG68K_View = 1;

				SetTaskInfo(SCENE_START_S);	/* シーン(処理)へ設定 */
				break;
			}
			/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
			case SCENE_GAME_OVER_S:	
			{
				APL_Menu_Exit();	/* メニューを消す */

				G_PaletteSetZero();

				G_STG68K_to_GR_Load((int8_t *)((int32_t)(g_pSTG68_PTN_Buf + g_nSTG68_PTN_MAP[g_STG68K_mode])), g_nFileSize[STG_68_PTN], 
									(int16_t *)((int32_t)(g_pSTG68_COL_Buf)), g_nFileSize[STG_68_COL], 
									(int8_t *)((int32_t)(g_pSTG68_PAL_Buf + g_nSTG68_PAL_MAP[g_STG68K_mode])), g_nFileSize[STG_68_PAL], 
									256, 0, GR_Screen0, g_STG68K_mode );

				SetTaskInfo(SCENE_GAME_OVER);	/* シーン設定 */
				break;
			}
			case SCENE_GAME_OVER:
			{
				File_Save_OverWrite(g_sFileName[STG_68_PTN], g_pSTG68_PTN_Buf, sizeof(int8_t), g_nFileSize[STG_68_PTN] );

				File_Save_OverWrite(g_sFileName[STG_68_PAL], g_pSTG68_PAL_Buf, sizeof(int8_t), g_nFileSize[STG_68_PAL] );

				File_Save_OverWrite(g_sFileName[STG_68_COL], g_pSTG68_COL_Buf, sizeof(int8_t), g_nFileSize[STG_68_COL] );

				SetTaskInfo(SCENE_GAME_OVER_E);	/* シーン設定 */
				break;
			}
			case SCENE_GAME_OVER_E:
			{
				SetTaskInfo(SCENE_EXIT);	/* 終了シーンへ設定 */
				break;
			}
			/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
			default:	/* 異常シーン */
			{
				loop = 0;	/* ループ終了 */
				break;
			}
		}
		SetFlip(TRUE);	/* フリップ許可 */

		PCG_END_SYNC(g_Vwait);	/* スプライトの出力 */

		uFreeRunCount++;	/* 16bit フリーランカウンタ更新 */
		g_unFrameCount++;	/* 16bit フリーランカウンタ更新 */

#ifdef DEBUG	/* デバッグコーナー */
		/* 処理時間計測 */
		GetNowTime(&time_now);
		unTime_cal = time_now - time_st;	/* LSB:1 UNIT:ms */
		g_unTime_cal = unTime_cal;
		if( stTask.bScene == SCENE_GAME1 )
		{
			unTime_cal_PH = Mmax(unTime_cal, unTime_cal_PH);
			g_unTime_cal_PH = unTime_cal_PH;
		}
#endif

#ifdef MEM_MONI	/* デバッグコーナー */
		GetFreeMem();	/* 空きメモリサイズ確認 */
#endif

	}
	while( loop );

	g_bExit = FALSE;

	return ret;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
static void App_Init(void)
{
	uint32_t nNum;
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_Init 開始");
#endif
#ifdef MEM_MONI	/* デバッグコーナー */
	g_nMaxUseMemSize = GetFreeMem();
	printf("FreeMem(%d[kb])\n", g_nMaxUseMemSize);	/* 空きメモリサイズ確認 */
	puts("App_Init メモリ");
#endif

	Set_SupervisorMode();	/* スーパーバイザーモード */
	/* MFP */
	MFP_INIT();	/* V-Sync割り込み等の初期化処理 */
	Set_UserMode();			/* ユーザーモード */
#ifdef DEBUG	/* デバッグコーナー */
	printf("App_Init MFP(%d)\n", Get_CPU_Time());
#endif
	if(SetNowTime(0) == FALSE)
	{
		/*  */
	}
	nNum = Get_CPU_Time();	/* 300 = 10MHz基準 */
	if(nNum  < 400)
	{
		g_GameSpeed = 0;
		printf("Normal Speed(%d)\n", g_GameSpeed);
	}
	else if(nNum < 640)
	{
		g_GameSpeed = 2;
		printf("XVI Speed(%d)\n", g_GameSpeed);
	}
	else if(nNum < 800)
	{
		g_GameSpeed = 4;
		printf("RedZone Speed(%d)\n", g_GameSpeed);
	}
	else if(nNum < 2000)
	{
		g_GameSpeed = 7;
		printf("030 Speed(%d)\n", g_GameSpeed);
	}
	else
	{
		g_GameSpeed = 10;
		printf("Another Speed(%d)\n", g_GameSpeed);
	}
	puts("App_Init Input");
	Input_Init();			/* 入力初期化 */
	/* スーパーバイザーモード開始 */
	Set_SupervisorMode();


	/* Task */
	TaskManage_Init();
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_Init Task");
#endif
	
	/* マウス初期化 */
	Mouse_Init();	/* マウス初期化 */
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_Init マウス");
#endif
	/* 画面 */
//	g_nCrtmod = CRTC_INIT(10);	/* mode=10 256x256 col:16/256 31kHz */
	g_nCrtmod = CRTC_INIT(12);	/* mode=14 512x512 col:65536 31kHz */
//	g_nCrtmod = CRTC_INIT(14);	/* mode=14 256x256 col:65536 31kHz */
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_Init CRTC(画面)");
//	KeyHitESC();	/* デバッグ用 */
#endif

	/* グラフィック */
	G_INIT();			/* 画面の初期設定 */
	G_CLR();			/* クリッピングエリア全開＆消す */
	G_HOME(0);			/* ホーム位置設定 */
	G_VIDEO_INIT();		/* ビデオコントローラーの初期化 */
	G_PaletteSetZero();
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_Init グラフィック");
#endif

	/* テキスト */
	T_INIT();	/* テキストＶＲＡＭ初期化 */
	T_PALET();	/* テキストパレット初期化 */
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_Init T_INIT");
#endif
	g_Vwait = 1;

#ifdef DEBUG	/* デバッグコーナー */
	puts("App_Init 終了");
#endif
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
static void App_exit(void)
{
	int16_t	ret = 0;

#ifdef DEBUG	/* デバッグコーナー */
	puts("App_exit 開始");
#endif
	
	if(g_bExit == TRUE)
	{
		puts("エラーをキャッチ！ ESC to skip");
		KeyHitESC();	/* デバッグ用 */
	}
	g_bExit = TRUE;

	/* グラフィック */
	G_CLR();			/* クリッピングエリア全開＆消す */


	/* MFP */
	ret = MFP_EXIT();				/* MFP関連の解除 */
#ifdef DEBUG	/* デバッグコーナー */
	printf("App_exit MFP(%d)\n", ret);
#endif

	/* 画面 */
	CRTC_EXIT(0x100 + g_nCrtmod);	/* モードをもとに戻す */
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_exit 画面");
#endif

	Mouse_Exit();	/* マウス非表示 */

	/* テキスト */
	T_EXIT();				/* テキスト終了処理 */
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_exit テキスト");
#endif

	MyMfree(0);				/* 全てのメモリを解放 */
#ifdef DEBUG	/* デバッグコーナー */
	printf("MaxUseMem(%d[kb])\n", g_nMaxUseMemSize - GetMaxFreeMem());
	puts("App_exit メモリ");
#endif

	_dos_kflushio(0xFF);	/* キーバッファをクリア */
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_exit キーバッファクリア");
#endif

	/*スーパーバイザーモード終了*/
	Set_UserMode();
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_exit スーパーバイザーモード終了");
#endif

#ifdef DEBUG	/* デバッグコーナー */
	puts("App_exit 終了");
#endif
}


/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t	App_FlipProc(void)
{
	int16_t	ret = 0;
	
#ifdef FPS_MONI	/* デバッグコーナー */
	uint32_t time_now;
	static uint8_t	bFPS = 0u;
	static uint32_t	unTime_FPS = 0u;
#endif

	if(g_bFlip == FALSE)	/* 描画中なのでフリップしない */
	{
		return ret;
	}
	else
	{
#ifdef W_BUFFER
		ST_CRT		stCRT;
		GetCRT(&stCRT, g_mode);	/* 画面座標取得 */
#endif
		SetFlip(FALSE);			/* フリップ禁止 */
					
#ifdef W_BUFFER
		/* モードチェンジ */
		if(g_mode == 1u)		/* 上側判定 */
		{
			SetGameMode(2);
		}
		else if(g_mode == 2u)	/* 下側判定 */
		{
			SetGameMode(1);
		}
		else					/* その他 */
		{
			SetGameMode(0);
		}

		/* 非表示画面を表示画面へ切り替え */
		G_HOME(g_mode);

		/* 部分クリア */
		G_CLR_AREA(	stCRT.hide_offset_x, WIDTH,
					stCRT.hide_offset_y, HEIGHT, 0);
#endif
		
#ifdef FPS_MONI	/* デバッグコーナー */
		bFPS++;
#endif
		ret = 1;
	}

#ifdef FPS_MONI	/* デバッグコーナー */
	GetNowTime(&time_now);
	if( (time_now - unTime_FPS) >= 1000ul )
	{
		g_bFPS_PH = bFPS;
		unTime_FPS = time_now;
		bFPS = 0;
	}
#endif

	return ret;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t	SetFlip(uint8_t bFlag)
{
	int16_t	ret = 0;

	Set_DI();	/* 割り込み禁止 */
	g_bFlip_old = g_bFlip;	/* フリップ前回値更新 */
	g_bFlip = bFlag;
	Set_EI();	/* 割り込み許可 */

	return ret;
}
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
void App_TimerProc( void )
{
	ST_TASK stTask;

	TaskManage();
	GetTaskInfo(&stTask);	/* タスク取得 */

	/* ↓↓↓ この間に処理を書く ↓↓↓ */
	if(stTask.b96ms == TRUE)	/* 96ms周期 */
	{
		if(GetJoyAnalogMode() == TRUE)	/* アナログ入力 */
		{
			if(Input_Main(g_ubDemoPlay) != 0u) 	/* 入力処理 */
			{
				g_ubDemoPlay = FALSE;	/* デモ解除 */

				SetTaskInfo(SCENE_INIT);	/* シーン(初期化処理)へ設定 */
			}
		}
		else	/* デジタル入力 */
		{
		}
#if 0
		switch(stTask.bScene)
		{
			case SCENE_GAME1:
			case SCENE_GAME2:
			{
#ifdef FPS_MONI	/* デバッグコーナー */
//				int8_t	sBuf[8];
#endif
#ifdef FPS_MONI	/* デバッグコーナー */
				memset(sBuf, 0, sizeof(sBuf));
				sprintf(sBuf, "%3d", g_bFPS_PH);
				Draw_Message_To_Graphic(sBuf, 24, 24, F_MOJI, F_MOJI_BAK);	/* デバッグ */
#endif
				break;
			}
			default:
				break;
		}
#endif
	}
	/* ↑↑↑ この間に処理を書く ↑↑↑ */

	/* タスク処理 */
	UpdateTaskInfo();		/* タスクの情報を更新 */

}
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t App_RasterProc( uint16_t *pRaster_cnt )
{
	int16_t	ret = 0;
#if CNF_RASTER
	RasterProc(pRaster_cnt);	/* ラスター割り込み処理 */
#endif /* CNF_RASTER */
	return ret;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
void App_VsyncProc( void )
{
#if CNF_VDISP
	ST_TASK stTask;
	int8_t x;
	int8_t y;
	int8_t left;
	int8_t right;

//	puts("App_VsyncProc");
	Timer_D_Less_NowTime();

	GetTaskInfo(&stTask);	/* タスク取得 */
	/* ↓↓↓ この間に処理を書く ↓↓↓ */
	if(GetJoyAnalogMode() == TRUE)	/* アナログ入力 */
	{
	}
	else	/* デジタル入力 */
	{
		if(Input_Main(g_ubDemoPlay) != 0u) 	/* 入力処理 */
		{
			g_ubDemoPlay = FALSE;	/* デモ解除 */

			SetTaskInfo(SCENE_INIT);	/* シーン(初期化処理)へ設定 */
		}
	}

	switch(stTask.bScene)
	{
		case SCENE_TITLE:
		case SCENE_GAME_OVER:
		{
			Mouse_GetDataPos(&x, &y, &left, &right);

			if(x == 0)
			{

			}
			else if(x > 0)
			{
				SetInput(KEY_RIGHT);
			}
			else{
				SetInput(KEY_LEFT);
			}

			if(y == 0)
			{

			}
			else if(y > 0)
			{
				SetInput(KEY_LOWER);
			}
			else{
				SetInput(KEY_UPPER);
			}

			if(left != 0)
			{
				SetInput(KEY_b_Z);
			}

			if(right != 0)
			{
				SetInput(KEY_b_X);
			}
			break;
		}
		default:	/* 異常シーン */
		{
			break;
		}
	}

	/* ↑↑↑ この間に処理を書く ↑↑↑ */
	
	App_FlipProc();	/* 画面切り替え */

#endif /* CNF_VDISP */
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t	GetGameMode(uint8_t *bMode)
{
	int16_t	ret = 0;
	
	*bMode = g_mode;
	
	return ret;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t	SetGameMode(uint8_t bMode)
{
	int16_t	ret = 0;
	
	g_mode = bMode;
	if(bMode == 1)
	{
		g_mode_rev = 2;
	}
	else
	{
		g_mode_rev = 1;
	}
	
	return ret;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t SerchNumber(int8_t *pData, int32_t data_size, int8_t min, int8_t max)
{
    // 範囲内の使用状況を記録するフラグ
    int8_t *pFlag;
	int32_t i;
	int16_t range = max - min + 1;

	pFlag = MyMalloc(range *  sizeof(int8_t));
    for ( i = 0; i < range; i++) {
        pFlag[i] = FALSE;
    }
    // 使用されている数値にフラグを立てる
    for ( i = 0; i < data_size; i++) {
        if ((pData[i] >= min) && (pData[i] <= max)) {
 			pFlag[pData[i] - min] = TRUE;  // インデックス補正
        }
    }

    // 使用されていない数値を出力
    for ( i = 0; i < range; i++) {
        if(pFlag[i] == 0){
			MyMfree(pFlag);
            return (i + min);
        }
    }
	MyMfree(pFlag);
    return -1;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t STG68K_SPPAL_to_PTNCOLPAL_Cnv(int16_t nMode)
{
	int16_t ret = 0;
	int16_t x, y, z, u;
	int16_t dx, dy;
	int16_t src_pal, dst_pal;
	int8_t *pSrc, *pDst, *pDst2;
	int16_t *pSrc16, *pDst16;
	int16_t count_x, count_y;
	int16_t pal_min, pal_max;
	int8_t bMuch = FALSE;

	if(nMode >= STG68K_MAX)
	{
		return ret -1;
	}

	switch(nMode)
	{
		case STG68K_SHIP:
		case STG68K_ENEMTY:
		case STG68K_BOSS:
		{
			pal_min = 17;
			pal_max = 31;
			break;
		}
		case STG68K_SCORE:
		{
			pal_min = 16;
			pal_max = 16;
			break;
		}
		case STG68K_BG:
		{
			pal_min = 0;
			pal_max = 15;
			break;
		}
		default:
		{
			pal_min = 0;
			pal_max = 31;
			break;
		}
	}

	count_x = g_nTboxCursorWidth / 16;
	count_y = g_nTboxCursorHeight / 16;

	for(dy = 0; dy < count_y; dy++)	/* 縦SP */
	{
		for(dx = 0; dx < count_x; dx++)	/* 横SP  */
		{
			/*===<PTN>===============================================================================================================*/

			pSrc = g_pSP_Buf + (dy * 0x800) + (dx * 0x80) + ((g_nTboxCursorPosY[VIEW_SP] / 16) * 0x800) + ((g_nTboxCursorPosX[VIEW_SP] / 16) * 0x80);

			for(z = 0; z < 4; z++)	/* 8x8を4セット */
			{
				for(y = 0; y < 4; y++)	/* 縦8ドット分 */
				{
					int32_t nOffsetXY[] = {0x00, 0x40, 0x04, 0x44};

					pDst = g_pSTG68_PTN_Buf + g_nSTG68_PTN_MAP[nMode] + ((g_nTboxCursorPosY[VIEW_STG68K] / 16) * 0x800) + ((g_nTboxCursorPosX[VIEW_STG68K] / 16) * 0x80) + (dy * 0x800) + (dx * 0x80) + nOffsetXY[z] + (y * 0x10);
					pDst2= g_pSTG68_PTN_Buf + g_nSTG68_PTN_MAP[nMode] + ((g_nTboxCursorPosY[VIEW_STG68K] / 16) * 0x800) + ((g_nTboxCursorPosX[VIEW_STG68K] / 16) * 0x80) + (dy * 0x800) + (dx * 0x80) + nOffsetXY[z] + (y * 0x10) + 8;

					for(x = 0; x < 4; x++)	/* 横4ドット分1 */
					{
						*pDst = swap_nibbles(*pSrc);
						pDst++;
						pSrc++;
					}
					for(x = 0; x < 4; x++)	/* 横4ドット分2 */
					{
						*pDst2 = swap_nibbles(*pSrc);
						pDst2++;
						pSrc++;
					}
				}
			}
		}
	}

	/*===<PAL>===============================================================================================================*/
	dst_pal = SerchNumber(g_pSTG68_PAL_Buf, g_nFileSize[STG_68_PAL], pal_min, pal_max);	/* 空きパレット探す */
	for(z = pal_min; z <= pal_max; z++)	/* 探す */
	{
		bMuch = TRUE;
		pSrc16 = (int16_t *)((uint32_t)g_pPAL_Buf + (g_STG68K_PAL_num * sizeof(int16_t) * 16));
		pDst16 = (int16_t *)((uint32_t)g_pSTG68_COL_Buf + (sizeof(int16_t) * 16 * z));
		for(u=0; u < 16; u++)
		{
			int16_t dst_col;
			// ソースの０bitから順番にどこのビットに移動したいか（0 → , 1 → , ..., 15 → ）
			uint8_t reverse_order[16] = {
					9, 10,				/* R */
					11, 12, 13, 14, 15,	/* B */
					0,					/* I */
					1,  2,  3,  4,  5,	/* G */
					6,  7,  8			/* R */
			};

			dst_col = reorder_bits(*pSrc16, reverse_order);
//			printf("(%d,%d)*pDst16 0x%X dst_col 0x%X, *pSrc16 0x%X\n", z, u, (int16_t)*pDst16, dst_col, (int16_t)*pSrc16);
			if((int16_t)*pDst16 != dst_col)
			{
				bMuch = FALSE;
				break;
			}
			pSrc16++;
			pDst16++;
		}
		if(bMuch == TRUE)
		{
//			printf("much %d\n", z);
			dst_pal = z;
			break;
		}
	}

	for(dy = 0; dy < count_y; dy++)	/* 縦SP */
	{
		for(dx = 0; dx < count_x; dx++)	/* 横SP  */
		{
			if(dst_pal < 0)
			{
				src_pal = *(g_pSTG68_PAL_Buf + g_nSTG68_PAL_MAP[nMode] + ((g_nTboxCursorPosY[VIEW_STG68K] / 16) * 0x10) + (g_nTboxCursorPosX[VIEW_STG68K] / 16) + (dy * 0x10) + count_x);
//				printf("src_pal %d dst_pal%d\n", src_pal, dst_pal);
				dst_pal = src_pal;
			}
			*(g_pSTG68_PAL_Buf + g_nSTG68_PAL_MAP[nMode] + ((g_nTboxCursorPosY[VIEW_STG68K] / 16) * 0x10) + (g_nTboxCursorPosX[VIEW_STG68K] / 16) + (dy * 0x10) + dx) = dst_pal;
//			printf("%d,", *(g_pSTG68_PAL_Buf + g_nSTG68_PAL_MAP[nMode] + (dy * 0x10) + dx));
		}
	}
	
	/*===<COL>===============================================================================================================*/
	pSrc16 = (int16_t *)((uint32_t)g_pPAL_Buf + (g_STG68K_PAL_num * sizeof(int16_t) * 16));
	pDst16 = (int16_t *)((uint32_t)g_pSTG68_COL_Buf + (sizeof(int16_t) * 16 * dst_pal));
	for(u=0; u < 16; u++)
	{
		// ソースの０bitから順番にどこのビットに移動したいか（0 → , 1 → , ..., 15 → ）
		uint8_t reverse_order[16] = {
				9, 10,				/* R */
				11, 12, 13, 14, 15,	/* B */
				0,					/* I */
				1,  2,  3,  4,  5,	/* G */
				6,  7,  8			/* R */
		};
//							*pDst16 = swap_nibbles8(*pSrc16);
		*pDst16 = reorder_bits(*pSrc16, reverse_order);
		pSrc16++;
		pDst16++;
	}
	return	ret;
}
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
void HelpMessage(void)
{
	printf("=<Help Message>=============================================================\n");
	printf("  STG68K_CNV.X -i <SP/PALファイル名> -o <出力STAGE番号> -m <差し換え情報>\n");
	printf("   ex. >STG68K_CNV.x -i hogehoge -o 2 -m 0\n");
	printf("        自機のみをデータコンバートします\n");
	printf("        hogehoge.sp hogehoge.pal =>  STAGE2.PTN STAGE2.COL STAGE2.PAL \n");
	printf("============================================================================\n");
	printf("<差し換え情報(bit)>\n");
	printf("  bit0:自機のみ\n");
	printf("============================================================================\n");
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t main(int16_t argc, int8_t** argv)
{
	int16_t	ret = 0;

	/* COPYキー無効化 */
	init_trap12();
	/* 例外ハンドリング処理 */
	usr_abort = App_exit;	/* 例外発生で終了処理を実施する */
	init_trap14();			/* デバッグ用致命的エラーハンドリング */
#if 0	/* アドレスエラー発生 */
	{
		char buf[10];
		int *A = (int *)&buf[1];
		int B = *A;
		return B;
	}
#endif

	printf("SHOOTING 68Kデータ作成補助ツール「STG68K_CNV.X」Ver%d.%d.%d (c)2025 カタ.\n", MAJOR_VER, MINOR_VER, PATCH_VER);

	App_Init();		/* 初期化 */

	if(argc > 1)	/* オプションチェック */
	{
		int16_t i, j;
		
		for(i = 1; i < argc; i++)
		{
			int8_t	bOption = FALSE;
			int8_t	bFlag;
			
			bOption	= ((argv[i][0] == '-') || (argv[i][0] == '/')) ? TRUE : FALSE;

			if(bOption == TRUE)
			{
				/* ヘルプ */
				bFlag	= ((argv[i][1] == '?') || (argv[i][1] == 'h') || (argv[i][1] == 'H')) ? TRUE : FALSE;
				if(bFlag == TRUE)
				{
					HelpMessage();	/* ヘルプ */
					ret = -1;
					break;
				}

				/* 入力ファイル */
				bFlag	= ((argv[i][1] == 'i') || (argv[i][1] == 'I')) ? TRUE : FALSE;
				if(bFlag == TRUE)
				{
					for(j = 0; j < 2; j++)
					{
						int8_t sStr[128] = {0};
						int8_t sExt[2][8] = {".SP",".PAL"};

						strcpy(sStr, argv[i+1]);
						strcat(sStr, sExt[j]);
						ret = Get_FileAlive(sStr);
						if(ret < 0)
						{
							HelpMessage();	/* ヘルプ */
							ret = -1;
							break;
						}
						else
						{
							int32_t nLength = 0;
							GetFileLength(sStr, &nLength);

							switch(j)
							{
								case STG_SM_SP:
								{
									strcpy(g_sFileName[STG_SM_SP], sStr);
									g_nFileSize[STG_SM_SP] = nLength;
									g_pSP_Buf = MyMalloc(nLength);
									File_Load(sStr, g_pSP_Buf, sizeof(int8_t), nLength);
									break;
								}
								case STG_SM_PAL:
								{
									strcpy(g_sFileName[STG_SM_PAL], sStr);
									g_nFileSize[STG_SM_PAL] = nLength;
									g_pPAL_Buf = MyMalloc(nLength);
									File_Load(sStr, g_pPAL_Buf, sizeof(int8_t), nLength);
									break;
								}
							}
							printf("%s(%d)\n",g_sFileName[j], g_nFileSize[j]);
						}
					}

					continue;
				}
				
				/* 出力ファイル */
				bFlag	= ((argv[i][1] == 'o') || (argv[i][1] == 'O')) ? TRUE : FALSE;
				if(bFlag == TRUE)
				{
					int16_t nStageNum;
					nStageNum = atoi(argv[i+1]);
					g_STG68K_stage = (uint8_t)nStageNum;

					for(j = 0; j < 3; j++)
					{
						int8_t sStr[128] = {0};
						int8_t sExt[3][8] = {".PTN",".COL",".PAL"};

						sprintf(sStr, "STAGE%d%s", nStageNum, sExt[j]);
						ret = Get_FileAlive(sStr);
						if(ret < 0)
						{
							HelpMessage();	/* ヘルプ */
							ret = -1;
							break;
						}
						else
						{
							int32_t nLength = 0;
							GetFileLength(sStr, &nLength);
							//printf("%s\n",sStr);

							switch(j)
							{
								case 0:
								{
									strcpy(g_sFileName[STG_68_PTN], sStr);
									g_nFileSize[STG_68_PTN] = nLength;
									g_pSTG68_PTN_Buf = MyMalloc(nLength);
									File_Load(sStr, g_pSTG68_PTN_Buf, sizeof(int8_t), nLength);
									break;
								}
								case 1:
								{
									strcpy(g_sFileName[STG_68_COL], sStr);
									g_nFileSize[STG_68_COL] = nLength;
									g_pSTG68_COL_Buf = MyMalloc(nLength);
									File_Load(sStr, g_pSTG68_COL_Buf, sizeof(int8_t), nLength);
									break;
								}
								case 2:
								{
									strcpy(g_sFileName[STG_68_PAL], sStr);
									g_nFileSize[STG_68_PAL] = nLength;
									g_pSTG68_PAL_Buf = MyMalloc(nLength);
									File_Load(sStr, g_pSTG68_PAL_Buf, sizeof(int8_t), nLength);
									break;
								}
							}
						}
						printf("%s(%d)\n",g_sFileName[STG_68_PTN+j], g_nFileSize[STG_68_PTN+j]);
					}
					continue;
				}

				/* 差し換え情報 */
				bFlag	= ((argv[i][1] == 'm') || (argv[i][1] == 'M')) ? TRUE : FALSE;
				if(bFlag == TRUE)
				{
					g_STG68K_mode = atoi(argv[i+1]);
					continue;
				}

				/* ヘルプ */
				if(bFlag == FALSE)
				{
					HelpMessage();	/* ヘルプ */
					ret = -1;
					break;
				}
			}
		}
	}
	else
	{
		HelpMessage();	/* ヘルプ */
		ret = -1;
	}

	if(ret == 0)
	{
		main_Task();	/* メイン処理 */

		App_exit();		/* 終了処理 */
	}
	
	return ret;
}

#endif	/* MAIN_C */
