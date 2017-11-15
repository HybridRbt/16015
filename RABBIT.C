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
int g_bGotData;  // indicate if initial data has been received

long g_flatUpStep_57;
long g_flatUpStep_47;

long g_flatDownStep_57;
long g_flatDownStep_47;

long g_mappingStep;
long g_checkFlatTypeStep;

/*==============================================================================
 *                  main
 *============================================================================*/
void main(void)
{
	int re, iMode;
	int fullBoat;
   int flatCount;
	unsigned long end;

	/**
	 * Get command from keypad, process the message,
	 * and display on LCD
	 */
	init(); 	//initialize I/O and globle parameters

	g_bConnected = FALSE;
	g_stage = NOT_READY;
   g_bGotData = FALSE;

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

		end = MS_TIMER + TIMEOUT_PC_CONNECT;
		while (MS_TIMER < end)
		{
         if (!g_bConnected)
			{
         	if ((re = SendPcReady()) == NO_ERROR)  // connected with PC
         		g_bConnected = TRUE;
         }
         else
         {
            printf("entering handleMsg()...\n");
            handleMsg();

            if (g_bGotData)
         		break;
         }
		}

      //if (!g_bGotData) // no data, use default
      //	LoadDefaultValueForSteps();

      setIdleLight();
	}

	//main loop
	while (TRUE)
	{
		if (g_stage == READY)
		{
			handleMsg();
			DelayMilliseconds(2);

         if (!IsBoatPresent())
			{
				g_bBoatRemoved = TRUE;
				AlarmOff();
			}

			if (g_bBoatRemoved && IsBoatPresent())
			{
            setOperationLight();
            g_stage = IN_OP;
         }
		}
		else if (g_stage == IN_IO)
		{
			//setOperationLight();
			handleMsg();
			DelayMilliseconds(2);
		}
		else if (g_stage == IN_ERROR)
		{
			handleMsg();
			DelayMilliseconds(2);

         if (!IsBoatPresent())
			{
				g_bBoatRemoved = TRUE;
				AlarmOff();
            setIdleLight();
            g_stage = READY;
			}
		}
		else     // in op
		{
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
               setErrorLight();
					sendErrMsg(re);
				}
				else
				{
					PrepareForGetFlatType();
					flatCount = GetFlatCount();

					PrepareForGetWaferMap();
					// get wafer map
					fullBoat = GetWaferMap(flatCount);

					if (IsFlatToUp())
						g_flatOrientation = UP;
					else
						g_flatOrientation = DOWN;

					TurnFlatToTargetPos();

					if (!fullBoat || g_flatType == MIXED)
						// need to turn on red light
               {
                  setErrorLight();
                  g_stage = IN_ERROR;
               }
               else
               {
                  setIdleLight();
                  g_stage = READY;
               }

               SendPcOpDone();
				}
			}
		}
	}
}

/*------------------------------ end of rabbit.c -----------------------------*/