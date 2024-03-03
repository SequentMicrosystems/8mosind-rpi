#ifndef MOSFET8_H_
#define MOSFET8_H_

#include <stdint.h>

#define RETRY_TIMES	10
#define MOSFET8_INPORT_REG_ADD	0x00
#define MOSFET8_OUTPORT_REG_ADD	0x01
#define MOSFET8_POLINV_REG_ADD	0x02
#define MOSFET8_CFG_REG_ADD		0x03
#define PWM_SIZE_B 2
#define MOSFET_NO 8



enum
{
	I2C_INPORT_REG_ADD,
	I2C_OUTPORT_REG_ADD,
	I2C_POLINV_REG_ADD,
	I2C_CFG_REG_ADD,

	I2C_MEM_DIAG_3V3_MV_ADD,
	I2C_MEM_DIAG_TEMPERATURE_ADD = I2C_MEM_DIAG_3V3_MV_ADD +2,
	I2C_MEM_PWM1,
	I2C_MODBUS_SETINGS_ADD  = I2C_MEM_PWM1 + MOSFET_NO * PWM_SIZE_B,

	I2C_MEM_CPU_RESET = 0xaa,
	I2C_MEM_REVISION_HW_MAJOR_ADD ,
	I2C_MEM_REVISION_HW_MINOR_ADD,
	I2C_MEM_REVISION_MAJOR_ADD,
	I2C_MEM_REVISION_MINOR_ADD,
	SLAVE_BUFF_SIZE = 255
};


#define CHANNEL_NR_MIN		1
#define MOSFET_CH_NR_MAX		8

#define ERROR	-1
#define OK		0
#define FAIL	-1
#define ARG_CNT_ERR -2;

#define MOSFET8_HW_I2C_BASE_ADD	0x38
#define MOSFET8_HW_I2C_ALTERNATE_BASE_ADD 0x20
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef enum
{
	OFF = 0,
	ON,
	STATE_COUNT
} OutStateEnumType;

typedef struct
{
 const char* name;
 const int namePos;
 int(*pFunc)(int, char**);
 const char* help;
 const char* usage1;
 const char* usage2;
 const char* example;
}CliCmdType;


typedef struct
	__attribute__((packed))
	{
		unsigned int mbBaud :24;
		unsigned int mbType :4;
		unsigned int mbParity :2;
		unsigned int mbStopB :2;
		unsigned int add:8;
	} ModbusSetingsType;

#endif //MOSFET8_H_
