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
///< don't retry if error happened
char g_bNeedForceSoftReset;

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
unsigned char g_msgbuf[PCPORTOUTBUFSIZE];   /* buffer for RS232 message */

int g_stage;  // indicate current machine stage
int g_flatType;  // indicate the flat type (47 or 57)
int g_bConnected;  // indicate if pc is connected, if not, no msg will be sent
/*==============================================================================
 *                  main
 *============================================================================*/
void main(void)
{
	int	re, iMode;
	int fullBoat;

	/**
	 * Get command from keypad, process the message,
	 * and display on LCD
	 */
	init(); 	//initialize I/O and globle parameters

	g_stage = NOT_READY;

	if (IsBoatPresent())
		g_bBoatRemoved = FALSE;
	else
		g_bBoatRemoved = TRUE;

	if ((re = InitWhole()) != NO_ERROR)
	{
		sendErrMsg(re);
		setErrorLight();
		g_stage = IN_ERROR;
	}
	else
	{
		g_stage = READY;

		do
		{
			re = SendPcReady();
		}
		while (re != NO_ERROR);
	}

	//main loop
	while (TRUE)
	{
		if (g_stage == READY)
		{
			setIdleLight();
			handleMsg();
			DelayMilliseconds(2);

			if (!IsBoatPresent())
			{
				g_bBoatRemoved = TRUE;
				AlarmOff();
			}

			if (g_bBoatRemoved && IsBoatPresent())
				g_stage = IN_OP;
		}
		else if (g_stage == IN_IO)
		{
			setOperationLight();
			handleMsg();
			DelayMilliseconds(2);
		}
		else if (g_stage == IN_ERROR)
		{
			setErrorLight();
			handleMsg();
			DelayMilliseconds(2);
		}
		else     // in op
		{
			setOperationLight();

			if (!IsBoatPresent())
			{
				g_bBoatRemoved = TRUE;
				g_stage = READY;
				setIdleLight();
			}

			if (g_bBoatRemoved && IsBoatPresent())
			{
				g_bBoatRemoved = FALSE;
				SendPcOpStarted();

				if ((re = FindFlat()) != NO_ERROR)
				{
					g_stage = IN_ERROR;
					sendErrMsg(re);
				}
				else
				{
					PrepareForGetFlatType();
					g_flatType = GetFlatType();

					PrepareForGetWaferMap();
					// get wafer map
					fullBoat = GetWaferMap();

					if (IsFlatToUp())
						g_flatOrientation = UP;
					else
						g_flatOrientation = DOWN;

					TurnFlatToTargetPos();

					if (!fullBoat)
					{
						// need to turn on red light
						setErrorLight();
					}

					SendPcOpDone();
					g_stage = READY;
				}
			}
		}
	}
}

/*------------------------------ end of rabbit.c -----------------------------*/