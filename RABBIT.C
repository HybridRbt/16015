/*******************************************************************************
 *              Copyright(c) 2012, Mactronix Inc.
 *              All rights reserved
 *
 *              File:    rabbit.c
 *              Descrioption: main program for AFWC-600
 *
 *					Author:		 Marvin Zhang
 *					Date:        01/23/2012
 ******************************************************************************/

/**\file
 *\brief Main Program
 */
/*  Set a default of declaring all local variables "auto" (on stack) */
#class auto

/*==============================================================================
 *             Libraries
 *============================================================================*/
#use "header.lib"
#use "motion.lib"
#use "func.lib"
#use "nmc.lib"
#use "sensors.lib"
#use "com.lib"

unsigned int g_save0;
unsigned int g_save1;
unsigned int g_save2;
unsigned int g_save3;
unsigned int g_save4;
char g_bSave;                /**< Is sending message saved?      */
char g_bRetry;               ///< if operation is in init and sensor adjust

typedef struct
{
	int bMoving;
	int bMoveMode;        //0 speed mode 1 pos mode
	long TargetPos;
} Move_data;

Move_data       myRoller;
/*==============================================================================
 *                        global variables
 *============================================================================*/
int  g_currentRotationDegree;
int  g_flatOrientation;
char g_bBoatRemoved;

int g_waferMap[25]; //[i]==0: no wafer in i-th slot; 1: wafer exists in this slot
/*==============================================================================
 *                  main
 *============================================================================*/
void main(void)
{
	int	re, iMode;
	/**
	 * Get command from keypad, process the message,
	 * and display on LCD
	 */
	init(); 	//initialize I/O and globle parameters
	InitWhole();

	if (IsBoatPresent())
		g_bBoatRemoved = FALSE;
	else
		g_bBoatRemoved = TRUE;

	//main loop
	while (TRUE)
	{
		if (!IsBoatPresent())
			g_bBoatRemoved = TRUE;

		if (g_bBoatRemoved && IsBoatPresent())
		{
			re = NO_ERROR;
			re = FindFlat();

			if (re == NO_ERROR)
			{
				if (IsFlatToUp())
					g_flatOrientation = UP;
				else
					g_flatOrientation = DOWN;

				if (g_flatOrientation == UP)
					TurnFlatToTargetPos();
			}

			g_bBoatRemoved = FALSE;
		}
	}
}

/*------------------------------ end of rabbit.c -----------------------------*/