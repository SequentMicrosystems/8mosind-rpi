/*
 * mosfet8.c:
 *	Command-line interface to the Raspberry
 *	Pi's 8-Mosfet board.
 *	Copyright (c) 2016-2023 Sequent Microsystem
 *	<http://www.sequentmicrosystem.com>
 ***********************************************************************
 *	Author: Alexandru Burcea
 ***********************************************************************
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "mosfet.h"
#include "comm.h"
#include "thread.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

#define VERSION_BASE	(int)1
#define VERSION_MAJOR	(int)0
#define VERSION_MINOR	(int)7

#define UNUSED(X) (void)X      /* To avoid gcc/g++ warnings */
#define CMD_ARRAY_SIZE	18

#define THREAD_SAFE
//#define DEBUG_SEM
#define TIMEOUT_S 3

#define MOS_MIN_FREQ 16
#define MOS_MAX_FREQ 1000

const u8 mosfetMaskRemap[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
const int mosfetChRemap[8] = {0, 1, 2, 3, 4, 5, 6, 7};

int mosfetChSet(int dev, u8 channel, OutStateEnumType state);
int mosfetChGet(int dev, u8 channel, OutStateEnumType *state);
u8 mosfetToIO(u8 mosfet);
u8 IOToMosfet(u8 io);
int cfg485Set(int dev, u8 mode, u32 baud, u8 stopB, u8 parity, u8 add);
int cfg485Get(int dev);

static int doHelp(int argc, char *argv[]);
const CliCmdType CMD_HELP =
	{"-h", 1, &doHelp,
		"\t-h          Display the list of command options or one command option details\n",
		"\tUsage:      8mosind -h    Display command options list\n",
		"\tUsage:      8mosind -h <param>   Display help for <param> command option\n",
		"\tExample:    8mosind -h write    Display help for \"write\" command option\n"};

static int doVersion(int argc, char *argv[]);
const CliCmdType CMD_VERSION = {"-v", 1, &doVersion,
	"\t-v              Display the version number\n",
	"\tUsage:          8mosind -v\n", "",
	"\tExample:        8mosind -v  Display the version number\n"};

static int doWarranty(int argc, char *argv[]);
const CliCmdType CMD_WAR = {"-warranty", 1, &doWarranty,
	"\t-warranty       Display the warranty\n",
	"\tUsage:          8mosind -warranty\n", "",
	"\tExample:        8mosind -warranty  Display the warranty text\n"};

static int doList(int argc, char *argv[]);
const CliCmdType CMD_LIST =
	{"-list", 1, &doList,
		"\t-list:       List all 8mosind boards connected,\n\treturn       nr of boards and stack level for every board\n",
		"\tUsage:       8mosind -list\n", "",
		"\tExample:     8mosind -list display: 1,0 \n"};

static int doMosfetWrite(int argc, char *argv[]);
const CliCmdType CMD_WRITE = {"write", 2, &doMosfetWrite,
	"\twrite:       Set mosfets On/Off\n",
	"\tUsage:       8mosind <id> write <channel> <on/off>\n",
	"\tUsage:       8mosind <id> write <value>\n",
	"\tExample:     8mosind 0 write 2 On; Set Mosfet #2 on Board #0 On\n"};

static int doMosfetRead(int argc, char *argv[]);
const CliCmdType CMD_READ = {"read", 2, &doMosfetRead,
	"\tread:        Read mosfets status\n",
	"\tUsage:       8mosind <id> read <channel>\n",
	"\tUsage:       8mosind <id> read\n",
	"\tExample:     8mosind 0 read 2; Read Status of Mosfet #2 on Board #0\n"};

static int doMosfetPWMWrite(int argc, char *argv[]);
const CliCmdType CMD_PWM_WRITE = {"pwmwr", 2, &doMosfetPWMWrite,
	"\tpwmwr:       Set one mosfet pwm fill facor\n",
	"\tUsage:       8mosind <id> pwmwr <channel> <0..100>\n",
	"",
	"\tExample:     8mosind 0 pwmwr 2 45; Set Mosfet #2 on Board #0 pwm fill factor to 45%\n"};

static int doMosfetPWMRead(int argc, char *argv[]);
const CliCmdType CMD_PWM_READ = {"pwmrd", 2, &doMosfetPWMRead,
	"\tpwmrd:       Read one channel pwm fill factor\n",
	"\tUsage:       8mosind <id> pwmrd <channel>\n",
	"",
	"\tExample:     8mosind 0 pwmrd 2; Read pwm fill factor of Mosfet #2 on Board #0\n"};

static int doMosfetFreqWr(int argc, char *argv[]);
const CliCmdType CMD_F_WRITE =
	{"fwr", 2, &doMosfetFreqWr, "\tfwr:         Write pwm frequency in Hz\n",
		"\tUsage:       8mosind <id> fwr <frequency [16..1000]>\n", "",
		"\tExample:     8mosind 0 fwr 200; Set pwm frequency at 200Hz for all mosfets on Board #0\n"};

static int doMosfetFreqRd(int argc, char *argv[]);
const CliCmdType CMD_F_READ =
	{"frd", 2, &doMosfetFreqRd,
		"\tfrd:         Read pwm frequency in Hz\n",
		"\tUsage:       8mosind <id> frd\n", "",
		"\tExample:     8mosind 0 frd; Read pwm frequency for all mosfets on Board #0\n"};



static int doTest(int argc, char *argv[]);
const CliCmdType CMD_TEST = {"test", 2, &doTest,
	"\ttest:        Turn ON and OFF the mosfets until press a key\n", "",
	"\tUsage:       8mosind <id> test\n", "\tExample:     8mosind 0 test\n"};

int doRs485Write(int argc, char *argv[]);
const CliCmdType CMD_RS485_WRITE =
	{
		"cfg485wr",
		2,
		&doRs485Write,
		"\tcfg485wr:    Write the RS485 communication settings\n",
		"\tUsage:      8mosind <id> cfg485wr <mode> <baudrate> <stopBits> <parity> <slaveAddr>\n",
		"",
		"\tExample:		 8mosind 0 cfg485wr 1 9600 1 0 1; Write the RS485 settings on Board #0 \n\t\t\t(mode = Modbus RTU; baudrate = 9600 bps; stop bits one; parity none; modbus slave address = 1)\n"};

int doRs485Read(int argc, char *argv[]);
const CliCmdType CMD_RS485_READ =
{
	"cfg485rd",
	2,
	&doRs485Read,
	"\tcfg485rd:    Read the RS485 communication settings\n",
	"\tUsage:      8mosind <id> cfg485rd\n",
	"",
	"\tExample:		8mosind 0 cfg485rd; Read the RS485 settings on Board #0\n"};

CliCmdType gCmdArray[CMD_ARRAY_SIZE];

char *usage = "Usage:	 8mosind -h <command>\n"
	"         8mosind -v\n"
	"         8mosind -warranty\n"
	"         8mosind -list\n"
	"         8mosind <id> write <channel> <on/off>\n"
	"         8mosind <id> write <value>\n"
	"         8mosind <id> read <channel>\n"
	"         8mosind <id> read\n"
	"         8mosind <id> pwmwr <channel> <0..100>\n"
	"         8mosind <id> pwmrd <channel>\n"
	"         8mosind <id> fwr <[16..1000]>\n"
	"         8mosind <id> frd\n"
	"         8mosind <id> test\n"
	"         8mosind <id> cfg485wr <mode> <baudrate> <stopBits> <parity> <slaveAddr>\n"
	"         8mosind <id> cfg485rd\n"
	"Where: <id> = Board level id = 0..7\n"
	"Type 8mosind -h <command> for more help"; // No trailing newline needed here.

char *warranty =
	"	       Copyright (c) 2016-2023 Sequent Microsystems\n"
		"                                                             \n"
		"		This program is free software; you can redistribute it and/or modify\n"
		"		it under the terms of the GNU Leser General Public License as published\n"
		"		by the Free Software Foundation, either version 3 of the License, or\n"
		"		(at your option) any later version.\n"
		"                                    \n"
		"		This program is distributed in the hope that it will be useful,\n"
		"		but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		"		GNU Lesser General Public License for more details.\n"
		"			\n"
		"		You should have received a copy of the GNU Lesser General Public License\n"
		"		along with this program. If not, see <http://www.gnu.org/licenses/>.";
u8 mosfetToIO(u8 mosfet)
{
	u8 i;
	u8 val = 0;
	for (i = 0; i < 8; i++)
	{
		if ( (mosfet & (1 << i)) != 0)
			val += mosfetMaskRemap[i];
	}
	return 0xff ^ val;
}

u8 IOToMosfet(u8 io)
{
	u8 i;
	u8 val = 0;

	io ^= 0xff;
	for (i = 0; i < 8; i++)
	{
		if ( (io & mosfetMaskRemap[i]) != 0)
		{
			val += 1 << i;
		}
	}
	return val;
}

int mosfetChSet(int dev, u8 channel, OutStateEnumType state)
{
	int resp;
	u8 buff[2];

	if ( (channel < CHANNEL_NR_MIN) || (channel > MOSFET_CH_NR_MAX))
	{
		printf("Invalid mosfet nr!\n");
		return ERROR;
	}
	if (FAIL == i2cMem8Read(dev, MOSFET8_OUTPORT_REG_ADD, buff, 1))
	{
		return FAIL;
	}

	switch (state)
	{
	case ON:
		buff[0] &= ~ (1 << mosfetChRemap[channel - 1]);
		resp = i2cMem8Write(dev, MOSFET8_OUTPORT_REG_ADD, buff, 1);
		break;
	case OFF:
		buff[0] |= 1 << mosfetChRemap[channel - 1];
		resp = i2cMem8Write(dev, MOSFET8_OUTPORT_REG_ADD, buff, 1);
		break;
	default:
		printf("Invalid mosfet state!\n");
		return ERROR;
		break;
	}
	return resp;
}

int mosfetChSetPwm(int dev, u8 channel, float value)
{
	u8 buff[2];
	uint16_t raw = 0;

	if ( (channel < CHANNEL_NR_MIN) || (channel > MOSFET_CH_NR_MAX))
	{
		printf("Invalid mosfet nr!\n");
		return ERROR;
	}
	if(value > 100)
	{
		value = 100;
	}
	if(value < 0)
	{
		value = 0;
	}
	raw = (uint16_t)(value * 10);
	memcpy(buff, &raw, 2);
	return i2cMem8Write(dev, I2C_MEM_PWM1 + PWM_SIZE_B * (channel -1), buff, 2);

}

int mosfetChGet(int dev, u8 channel, OutStateEnumType *state)
{
	u8 buff[2];

	if (NULL == state)
	{
		return ERROR;
	}

	if ( (channel < CHANNEL_NR_MIN) || (channel > MOSFET_CH_NR_MAX))
	{
		printf("Invalid mosfet nr!\n");
		return ERROR;
	}

	if (FAIL == i2cMem8Read(dev, MOSFET8_OUTPORT_REG_ADD, buff, 1))
	{
		return ERROR;
	}

	if (buff[0] & (1 << mosfetChRemap[channel - 1]))
	{
		*state = OFF;
	}
	else
	{
		*state = ON;
	}
	return OK;
}


int mosfetChGetPwm(int dev, u8 channel, float *value)
{
	u8 buff[2];
	uint16_t raw = 0;

	if (NULL == value)
	{
		return ERROR;
	}

	if ( (channel < CHANNEL_NR_MIN) || (channel > MOSFET_CH_NR_MAX))
	{
		printf("Invalid mosfet nr!\n");
		return ERROR;
	}

	if (FAIL == i2cMem8Read(dev, I2C_MEM_PWM1 + PWM_SIZE_B * (channel -1), buff, 2))
	{
		return ERROR;
	}
	memcpy(&raw, buff, 2);

	*value = (float)raw / 10;

	return OK;
}

int mosfetSet(int dev, int val)
{
	u8 buff[2];

	buff[0] = mosfetToIO(0xff & val);

	return i2cMem8Write(dev, MOSFET8_OUTPORT_REG_ADD, buff, 1);
}

int mosfetGet(int dev, int *val)
{
	u8 buff[2];

	if (NULL == val)
	{
		return ERROR;
	}
	if (FAIL == i2cMem8Read(dev, MOSFET8_OUTPORT_REG_ADD, buff, 1))
	{
		return ERROR;
	}
	*val = IOToMosfet(buff[0]);
	return OK;
}


int mosfetSetFrequency(int dev, int val)
{
	u8 buff[2];
	uint16_t raw = 0;

	if (val < MOS_MIN_FREQ || val > MOS_MAX_FREQ)
	{
		printf("Frequency out of range [%d..%d]\n", MOS_MIN_FREQ, MOS_MAX_FREQ);
		return ERROR;
	}
	raw = (uint16_t)val;
	memcpy(buff, &raw, 2);
	return i2cMem8Write(dev, I2C_PWM_FREQ, buff, 2);
}

int mosfetGetFrequency(int dev, int *val)
{
	u8 buff[2];
	uint16_t raw = 0;

	if (NULL == val)
	{
		return ERROR;
	}
	if (FAIL == i2cMem8Read(dev, I2C_PWM_FREQ, buff, 2))
	{
		return ERROR;
	}
	memcpy(&raw, buff, 2);
	*val = raw;

	return OK;
}

int cfg485Set(int dev, u8 mode, u32 baud, u8 stopB, u8 parity, u8 add)
{
	ModbusSetingsType settings;
	u8 buff[5];

	if (baud > 921600 || baud < 1200)
	{
		printf("Invalid RS485 Baudrate [1200, 921600]!\n");
		return ERROR;
	}
	if (mode > 1)
	{
		printf("Invalid RS485 mode : 0 = disable, 1= Modbus RTU (Slave)!\n");
		return ERROR;
	}
	if (stopB < 1 || stopB > 2)
	{
		printf("Invalid RS485 stop bits [1, 2]!\n");
		return ERROR;
	}
	if (parity > 2)
	{
		printf("Invalid RS485 parity 0 = none; 1 = even; 2 = odd! \n");
		return ERROR;
	}
	if (add < 1)
	{
		printf("Invalid MODBUS device address: [1, 255]!\n");
	}
	settings.mbBaud = baud;
	settings.mbType = mode;
	settings.mbParity = parity;
	settings.mbStopB = stopB;
	settings.add = add;

	memcpy(buff, &settings, sizeof(ModbusSetingsType));
	if (OK != i2cMem8Write(dev, I2C_MODBUS_SETINGS_ADD, buff, 5))
	{
		printf("Fail to write RS485 settings!\n");
		return ERROR;
	}
	return OK;
}

int cfg485Get(int dev)
{
	ModbusSetingsType settings;
	u8 buff[5];

	if (OK != i2cMem8Read(dev, I2C_MODBUS_SETINGS_ADD, buff, 5))
	{
		printf("Fail to read RS485 settings!\n");
		return ERROR;
	}
	memcpy(&settings, buff, sizeof(ModbusSetingsType));
	printf("<mode> <baudrate> <stopbits> <parity> <add> %d %d %d %d %d\n",
		(int)settings.mbType, (int)settings.mbBaud, (int)settings.mbStopB,
		(int)settings.mbParity, (int)settings.add);
	return OK;
}



int doBoardInit(int stack)
{
	int dev = 0;
	int add = 0;
	uint8_t buff[8];

	if ( (stack < 0) || (stack > 7))
	{
		printf("Invalid stack level [0..7]!");
		return ERROR;
	}
	add = (stack + MOSFET8_HW_I2C_BASE_ADD) ^ 0x07;
	dev = i2cSetup(add);
	if (dev == -1)
	{
		return ERROR;
	}
	if (ERROR == i2cMem8Read(dev, MOSFET8_CFG_REG_ADD, buff, 1))
	{
		add = (stack + MOSFET8_HW_I2C_ALTERNATE_BASE_ADD) ^ 0x07;
		dev = i2cSetup(add);
		if (dev == -1)
		{
			return ERROR;
		}
		if (ERROR == i2cMem8Read(dev, MOSFET8_CFG_REG_ADD, buff, 1))
		{
			printf("8-MOSFETS card id %d not detected\n", stack);
			return ERROR;
		}
	}
	if (buff[0] != 0) //non initialized I/O Expander
	{
		// make all I/O pins output
		buff[0] = 0;
		if (0 > i2cMem8Write(dev, MOSFET8_CFG_REG_ADD, buff, 1))
		{
			return ERROR;
		}
		// put all pins in 0-logic state
		buff[0] = 0xff;
		if (0 > i2cMem8Write(dev, MOSFET8_OUTPORT_REG_ADD, buff, 1))
		{
			return ERROR;
		}
	}

	return dev;
}

int boardCheck(int hwAdd)
{
	int dev = 0;
	uint8_t buff[8];

	hwAdd ^= 0x07;
	dev = i2cSetup(hwAdd);
	if (dev == -1)
	{
		return FAIL;
	}
	if (ERROR == i2cMem8Read(dev, MOSFET8_CFG_REG_ADD, buff, 1))
	{
		return ERROR;
	}
	return OK;
}

/*
 * doMosfetWrite:
 *	Write coresponding mosfet channel
 **************************************************************************************
 */
static int doMosfetWrite(int argc, char *argv[])
{
	int pin = 0;
	OutStateEnumType state = STATE_COUNT;
	int val = 0;
	int dev = 0;
	OutStateEnumType stateR = STATE_COUNT;
	int valR = 0;
	int retry = 0;

	if ( (argc != 5) && (argc != 4))
	{
		printf("Usage: 8mosind <id> write <mosfet number> <on/off> \n");
		printf("Usage: 8mosind <id> write <mosfet reg value> \n");
		return (FAIL);
	}

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}
	if (argc == 5)
	{
		pin = atoi(argv[3]);
		if ( (pin < CHANNEL_NR_MIN) || (pin > MOSFET_CH_NR_MAX))
		{
			printf("Mosfet number value out of range\n");
			return (FAIL);
		}

		/**/if ( (strcasecmp(argv[4], "up") == 0)
			|| (strcasecmp(argv[4], "on") == 0))
			state = ON;
		else if ( (strcasecmp(argv[4], "down") == 0)
			|| (strcasecmp(argv[4], "off") == 0))
			state = OFF;
		else
		{
			if ( (atoi(argv[4]) >= STATE_COUNT) || (atoi(argv[4]) < 0))
			{
				printf("Invalid mosfet state!\n");
				return (FAIL);
			}
			state = (OutStateEnumType)atoi(argv[4]);
		}

		retry = RETRY_TIMES;

		while ( (retry > 0) && (stateR != state))
		{
			if (OK != mosfetChSet(dev, pin, state))
			{
				printf("Fail to write mosfet\n");
				return (FAIL);
			}
			if (OK != mosfetChGet(dev, pin, &stateR))
			{
				printf("Fail to read mosfet\n");
				return (FAIL);
			}
			retry--;
		}
#ifdef DEBUG_I
		if(retry < RETRY_TIMES)
		{
			printf("retry %d times\n", 3-retry);
		}
#endif
		if (retry == 0)
		{
			printf("Fail to write mosfet\n");
			return (FAIL);
		}
	}
	else
	{
		val = atoi(argv[3]);
		if (val < 0 || val > 255)
		{
			printf("Invalid mosfet value\n");
			return (FAIL);
		}

		retry = RETRY_TIMES;
		valR = -1;
		while ( (retry > 0) && (valR != val))
		{

			if (OK != mosfetSet(dev, val))
			{
				printf("Fail to write mosfet!\n");
				return (FAIL);
			}
			if (OK != mosfetGet(dev, &valR))
			{
				printf("Fail to read mosfet!\n");
				return (FAIL);
			}
		}
		if (retry == 0)
		{
			printf("Fail to write mosfet!\n");
			return (FAIL);
		}
	}
	return OK;
}


/*
 * doMosfetPWMWrite:
 *	Write coresponding mosfet channel
 **************************************************************************************
 */
static int doMosfetPWMWrite(int argc, char *argv[])
{
	int pin = 0;
	int dev = 0;
	int retry = 0;

	float pwm = 0;
	float pwmR = 101;

	if (argc != 5)
	{
		printf("Usage: 8mosind <id> pwmwr <mosfet number> <0..100> \n");
		return (FAIL);
	}

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}
	if (argc == 5)
	{
		pin = atoi(argv[3]);
		if ( (pin < CHANNEL_NR_MIN) || (pin > MOSFET_CH_NR_MAX))
		{
			printf("Mosfet number value out of range\n");
			return (FAIL);
		}

		pwm = atof(argv[4]);

		retry = RETRY_TIMES;

		while ( (retry > 0) && (pwmR != pwm))
		{
			if (OK != mosfetChSetPwm(dev, pin, pwm))
			{
				printf("Fail to write mosfet or not PWM capable board\n");
				return (FAIL);
			}
			if (OK != mosfetChGetPwm(dev, pin, &pwmR))
			{
				printf("Fail to read mosfet\n");
				return (FAIL);
			}
			retry--;
		}
#ifdef DEBUG_I
		if(retry < RETRY_TIMES)
		{
			printf("retry %d times\n", 3-retry);
		}
#endif
		if (retry == 0)
		{
			printf("Fail to write mosfet\n");
			return (FAIL);
		}
	}
	return OK;
}

/*
 * doMosfetRead:
 *	Read mosfet state
 ******************************************************************************************
 */
static int doMosfetRead(int argc, char *argv[])
{
	int pin = 0;
	int val = 0;
	int dev = 0;
	OutStateEnumType state = STATE_COUNT;

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}

	if (argc == 4)
	{
		pin = atoi(argv[3]);
		if ( (pin < CHANNEL_NR_MIN) || (pin > MOSFET_CH_NR_MAX))
		{
			printf("Mosfet number value out of range!\n");
			return (FAIL);
		}

		if (OK != mosfetChGet(dev, pin, &state))
		{
			printf("Fail to read!\n");
			return (FAIL);
		}
		if (state != 0)
		{
			printf("1\n");
		}
		else
		{
			printf("0\n");
		}
	}
	else if (argc == 3)
	{
		if (OK != mosfetGet(dev, &val))
		{
			printf("Fail to read!\n");
			return (FAIL);
		}
		printf("%d\n", val);
	}
	else
	{
		printf("Usage: %s read mosfet value\n", argv[0]);
		return (FAIL);
	}
	return OK;
}



/*
 * doMosfetPWMRead:
 *	Read mosfet state
 ******************************************************************************************
 */
static int doMosfetPWMRead(int argc, char *argv[])
{
	int pin = 0;
	float val = 0;
	int dev = 0;


	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}

	if (argc == 4)
	{
		pin = atoi(argv[3]);
		if ( (pin < CHANNEL_NR_MIN) || (pin > MOSFET_CH_NR_MAX))
		{
			printf("Mosfet number value out of range!\n");
			return (FAIL);
		}

		if (OK != mosfetChGetPwm(dev, pin, &val))
		{
			printf("Fail to read!\n");
			return (FAIL);
		}

			printf("%.01f\n", val);
	}
	else
	{
		printf("Usage: %s read mosfet value\n", argv[0]);
		return (FAIL);
	}
	return OK;
}

static int doMosfetFreqWr(int argc, char *argv[])
{
	int freq = 0;
	int dev = 0;

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}

	if (argc == 4)
	{
		freq = atoi(argv[3]);

		if (OK != mosfetSetFrequency(dev, freq))
		{
			printf("Fail to set the frequency!");
			return FAIL;
		}
	}
	else
	{
		printf("Usage: %s set pwm frequency\n", argv[0]);
		return (FAIL);
	}
	return OK;
}


static int doMosfetFreqRd(int argc, char *argv[])
{
	int freq = 0;
	int dev = 0;

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}

	if (argc == 3)
	{
		if (OK != mosfetGetFrequency(dev, &freq))
		{
			printf("Fail to read the frequency!");
			return FAIL;
		}
		printf("%d\n", freq);
	}
	else
	{
		printf("Usage: %s get pwm frequency\n", argv[0]);
		return (FAIL);
	}
	return OK;
}

static int doHelp(int argc, char *argv[])
{
	int i = 0;
	if (argc == 3)
	{
		for (i = 0; i < CMD_ARRAY_SIZE; i++)
		{
			if ( (gCmdArray[i].name != NULL))
			{
				if (strcasecmp(argv[2], gCmdArray[i].name) == 0)
				{
					printf("%s%s%s%s", gCmdArray[i].help, gCmdArray[i].usage1,
						gCmdArray[i].usage2, gCmdArray[i].example);
					break;
				}
			}
		}
		if (CMD_ARRAY_SIZE == i)
		{
			printf("Option \"%s\" not found\n", argv[2]);
			printf("%s: %s\n", argv[0], usage);
		}
	}
	else
	{
		printf("%s: %s\n", argv[0], usage);
	}
	return OK;
}

static int doVersion(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);
	printf("8mosind v%d.%d.%d Copyright (c) 2016 - 2023 Sequent Microsystems\n",
	VERSION_BASE, VERSION_MAJOR, VERSION_MINOR);
	printf("\nThis is free software with ABSOLUTELY NO WARRANTY.\n");
	printf("For details type: 8mosind -warranty\n");
	return OK;
}

static int doList(int argc, char *argv[])
{
	int ids[8];
	int i;
	int cnt = 0;

	UNUSED(argc);
	UNUSED(argv);

	for (i = 0; i < 8; i++)
	{
		if (boardCheck(MOSFET8_HW_I2C_BASE_ADD + i) == OK)
		{
			ids[cnt] = i;
			cnt++;
		}
		else
		{
			if (boardCheck(MOSFET8_HW_I2C_ALTERNATE_BASE_ADD + i) == OK)
			{
				ids[cnt] = i;
				cnt++;
			}
		}
	}
	printf("%d board(s) detected\n", cnt);
	if (cnt > 0)
	{
		printf("Id:");
	}
	while (cnt > 0)
	{
		cnt--;
		printf(" %d", ids[cnt]);
	}
	printf("\n");
	return OK;
}

/* 
 * Self test for production
 */
static int doTest(int argc, char *argv[])
{
	int dev = 0;
	int i = 0;
	int retry = 0;
	int relVal;
	int valR;
	int mosfetResult = 0;
	FILE *file = NULL;
	const u8 mosfetOrder[8] = {1, 2, 3, 4, 5, 6, 7, 8};

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}
	if (argc == 4)
	{
		file = fopen(argv[3], "w");
		if (!file)
		{
			printf("Fail to open result file\n");
			//return -1;
		}
	}
//mosfet test****************************
	if (strcasecmp(argv[2], "test") == 0)
	{
		relVal = 0;
		printf(
			"Are all mosfets and LEDs turning on and off in sequence?\nPress y for Yes or any key for No....");
		startThread();
		while (mosfetResult == 0)
		{
			for (i = 0; i < 8; i++)
			{
				mosfetResult = checkThreadResult();
				if (mosfetResult != 0)
				{
					break;
				}
				valR = 0;
				relVal = (u8)1 << (mosfetOrder[i] - 1);

				retry = RETRY_TIMES;
				while ( (retry > 0) && ( (valR & relVal) == 0))
				{
					if (OK != mosfetChSet(dev, mosfetOrder[i], ON))
					{
						retry = 0;
						break;
					}

					if (OK != mosfetGet(dev, &valR))
					{
						retry = 0;
					}
				}
				if (retry == 0)
				{
					printf("Fail to write mosfet\n");
					if (file)
						fclose(file);
					return (FAIL);
				}
				busyWait(150);
			}

			for (i = 0; i < 8; i++)
			{
				mosfetResult = checkThreadResult();
				if (mosfetResult != 0)
				{
					break;
				}
				valR = 0xff;
				relVal = (u8)1 << (mosfetOrder[i] - 1);
				retry = RETRY_TIMES;
				while ( (retry > 0) && ( (valR & relVal) != 0))
				{
					if (OK != mosfetChSet(dev, mosfetOrder[i], OFF))
					{
						retry = 0;
					}
					if (OK != mosfetGet(dev, &valR))
					{
						retry = 0;
					}
				}
				if (retry == 0)
				{
					printf("Fail to write mosfet!\n");
					if (file)
						fclose(file);
					return (FAIL);
				}
				busyWait(150);
			}
		}
	}
	if (mosfetResult == YES)
	{
		if (file)
		{
			fprintf(file, "Mosfet Test ............................ PASS\n");
		}
		else
		{
			printf("Mosfet Test ............................ PASS\n");
		}
	}
	else
	{
		if (file)
		{
			fprintf(file, "Mosfet Test ............................ FAIL!\n");
		}
		else
		{
			printf("Mosfet Test ............................ FAIL!\n");
		}
	}
	if (file)
	{
		fclose(file);
	}
	mosfetSet(dev, 0);
	return OK;
}

static int doWarranty(int argc UNU, char* argv[] UNU)
{
	printf("%s\n", warranty);
	return OK;
}

int doRs485Read(int argc, char *argv[])
{
	int dev = 0;

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return ERROR;
	}

	if (argc == 3)
	{
		if (OK != cfg485Get(dev))
		{
			return ERROR;
		}
	}
	else
	{
		return ARG_CNT_ERR;
	}
	return OK;
}

int doRs485Write(int argc, char *argv[])
{
	int dev = 0;
	u8 mode = 0;
	u32 baud = 1200;
	u8 stopB = 1;
	u8 parity = 0;
	u8 add = 0;

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return ERROR;
	}
	if (argc == 8)
	{
		mode = 0xff & atoi(argv[3]);
		baud = atoi(argv[4]);
		stopB = 0xff & atoi(argv[5]);
		parity = 0xff & atoi(argv[6]);
		add = 0xff & atoi(argv[7]);
		if (OK != cfg485Set(dev, mode, baud, stopB, parity, add))
		{
			return ERROR;
		}
		printf("done\n");
	}
	else
	{
		return ARG_CNT_ERR;
	}
	return OK;
}


static void cliInit(void)
{
	int i = 0;

	memset(gCmdArray, 0, sizeof(CliCmdType) * CMD_ARRAY_SIZE);

	memcpy(&gCmdArray[i], &CMD_HELP, sizeof(CliCmdType));
	i++;
	memcpy(&gCmdArray[i], &CMD_WAR, sizeof(CliCmdType));
	i++;
	memcpy(&gCmdArray[i], &CMD_LIST, sizeof(CliCmdType));
	i++;
	memcpy(&gCmdArray[i], &CMD_WRITE, sizeof(CliCmdType));
	i++;
	memcpy(&gCmdArray[i], &CMD_READ, sizeof(CliCmdType));
	i++;
	memcpy(&gCmdArray[i], &CMD_PWM_WRITE, sizeof(CliCmdType));
	i++;
	memcpy(&gCmdArray[i], &CMD_PWM_READ, sizeof(CliCmdType));
	i++;
	memcpy(&gCmdArray[i], &CMD_F_WRITE, sizeof(CliCmdType));
	i++;
	memcpy(&gCmdArray[i], &CMD_F_READ, sizeof(CliCmdType));
	i++;
	memcpy(&gCmdArray[i], &CMD_TEST, sizeof(CliCmdType));
	i++;
	memcpy(&gCmdArray[i], &CMD_VERSION, sizeof(CliCmdType));
	i++;
	memcpy(&gCmdArray[i], &CMD_RS485_WRITE, sizeof(CliCmdType));
	i++;
	memcpy(&gCmdArray[i], &CMD_RS485_READ, sizeof(CliCmdType));

}

int waitForI2C(sem_t *sem)
{
  int semVal = 2;
  struct timespec ts;
  int s = 0;
  
#ifdef DEBUG_SEM
	sem_getvalue(sem, &semVal);
	printf("Semaphore initial value %d\n", semVal);
	semVal = 2;
#endif
	while (semVal > 0)
	{
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        /* handle error */
        printf("Fail to read time \n");
        return -1;
    }
    ts.tv_sec += TIMEOUT_S;
    while ((s = sem_timedwait(sem, &ts)) == -1 && errno == EINTR)
               continue;       /* Restart if interrupted by handler */
		sem_getvalue(sem, &semVal);
	}
#ifdef DEBUG_SEM
	sem_getvalue(sem, &semVal);
	printf("Semaphore after wait %d\n", semVal);
#endif
  return 0;
}

int releaseI2C(sem_t *sem)
{
  int semVal = 2;
  sem_getvalue(sem, &semVal);
	if (semVal < 1)
	{
		 if (sem_post(sem) == -1)
		 {
			 printf("Fail to post SMI2C_SEM \n");
       return -1;
		 }
	}
#ifdef DEBUG_SEM
	sem_getvalue(sem, &semVal);
	printf("Semaphore after post %d\n", semVal);
#endif
return 0;
}

int main(int argc, char *argv[])
{
	int i = 0;
	int ret = 0;

	cliInit();
	if (argc == 1)
	{
		printf("%s\n", usage);
		return 1;
	}
#ifdef THREAD_SAFE
	sem_t *semaphore = sem_open("/SMI2C_SEM", O_CREAT, 0000666, 3);
	waitForI2C(semaphore);
#endif
	for (i = 0; i < CMD_ARRAY_SIZE; i++)
	{
		if ( (gCmdArray[i].name != NULL) && (gCmdArray[i].namePos < argc))
		{
			if (strcasecmp(argv[gCmdArray[i].namePos], gCmdArray[i].name) == 0)
			{
				ret = gCmdArray[i].pFunc(argc, argv);
#ifdef THREAD_SAFE
			 releaseI2C(semaphore);
#endif
				return ret;
			}
		}
	}
	printf("Invalid command option\n");
	printf("%s\n", usage);
#ifdef THREAD_SAFE
  releaseI2C(semaphore);
#endif
	return -1;
}
