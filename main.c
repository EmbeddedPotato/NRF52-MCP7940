#include <stdio.h>
#include "boards.h"
#include "app_util_platform.h"
#include "app_error.h"
#include "nrf_drv_twi.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_delay.h"
#include "defines.h"

/* TWI instance ID. */
#if TWI0_ENABLED
#define TWI_INSTANCE_ID     0
#elif TWI1_ENABLED
#define TWI_INSTANCE_ID     1
#endif


// Set the initial Time Parameters

#define SECONDS 	((uint8_t)(00)) // 00 - 59 
#define MINUTES 	((uint8_t)(00)) // 00 - 59
#define HOURS  		((uint8_t)(00)) // 00 - 12 (for 12 Hour Mode) OR 00 - 23 (for 24 Hour Mode) 
#define DAY 			((uint8_t)(01)) // 1 - 7 
#define MONTHDATE	((uint8_t)(15)) // 1 - 29/30/31 
#define MONTH 		((uint8_t)(06))	// 0 - 12	
#define YEAR    	((uint8_t)(19))	// Year upto 2399


char* DaysOfWeek[7] = {"Monday","Tuesday","Wednesday","Thursday", "Friday", "Saturday","Sunday"};
char* Months[12] = {"January","February","March","April", "May", "June","July","August","September","October","November", "December"};


uint8_t DecToBcd(uint8_t Decimal);
uint8_t BcdToDec(uint8_t BCD);

// TWI functions 
void twi_init (void);
uint32_t read_register(uint8_t , uint8_t * , uint32_t );
uint32_t write_register(uint8_t , uint8_t );
//----------------------------------------------------//

uint32_t OscillatorRun();
void SetTime();


/* TWI instance. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);


uint8_t timeStamp[7];
uint8_t check_status;

int main(void)
{
    ret_code_t err_code;
    uint8_t address;
    uint8_t sample_data;
    bool detected_device = false;

    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    NRF_LOG_FLUSH();
    twi_init();

	SetTime();
	OscillatorRun();

	
	while(1)
	{
		err_code = read_register(MCP7940_RTCSEC,timeStamp,7);
		NRF_LOG_RAW_INFO    ("Current Time: ");
		NRF_LOG_RAW_INFO    ("%d",BcdToDec(timeStamp[2] & 0x3F));
		NRF_LOG_RAW_INFO    (":");
		if              (BcdToDec(timeStamp[1] & 0x7F) / 10 == 0) NRF_LOG_RAW_INFO ("0");
		NRF_LOG_RAW_INFO    ("%d",BcdToDec(timeStamp[1] & 0x7F));
		NRF_LOG_RAW_INFO    (":");
		if              (BcdToDec(timeStamp[0] & 0x7F) / 10 == 0) NRF_LOG_RAW_INFO ("0");  
		NRF_LOG_RAW_INFO  ("%d | ",BcdToDec(timeStamp[0] & 0x7F));
  
		NRF_LOG_RAW_INFO  ("%s, ",DaysOfWeek[BcdToDec(timeStamp[3] & 0x07)-1]);
		NRF_LOG_RAW_INFO  (" %d, ",BcdToDec(timeStamp[4] ));
		NRF_LOG_RAW_INFO  (" %s, ",Months[BcdToDec(timeStamp[5] )-1]);
		NRF_LOG_RAW_INFO  ("20%d\r\n",BcdToDec(timeStamp[6] ));
  
		nrf_delay_ms(1000);
		NRF_LOG_FLUSH();
	}
}

uint8_t DecToBcd(uint8_t Decimal) {
  return(Decimal / 10) << 4 | (Decimal % 10);
}

uint8_t BcdToDec(uint8_t BCD) {
  uint8_t HighByte = 0;
  uint8_t LowByte = 0;
  HighByte = (BCD & 0xF0) >> 4;
  LowByte = (BCD & 0x0F);
  return((HighByte*10) + LowByte);
}


/**
 * @brief TWI initialization.
 */

void twi_init (void)
{
    ret_code_t err_code;

    const nrf_drv_twi_config_t twi_config = {
       .scl                = 3,
       .sda                = 4,
       .frequency          = NRF_DRV_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
}

/**
 * @brief Read defined number of bytes from I2C register.
 */

uint32_t read_register(uint8_t reg, uint8_t * p_data, uint32_t length)
{
    uint32_t err_code;
    err_code = nrf_drv_twi_tx(&m_twi, MCP7940_ADDRESS, &reg, 1, false);
    if(err_code != NRF_SUCCESS) return err_code;

    
    err_code = nrf_drv_twi_rx(&m_twi, MCP7940_ADDRESS, p_data, length);
    if(err_code != NRF_SUCCESS) return err_code;
    return err_code;
}

/**
 * @brief Write single byte to I2C register.
 */

uint32_t write_register(uint8_t reg_address, uint8_t data)
{
    ret_code_t err_code; 
    uint8_t buffer_send[2] = {reg_address, data};
		
    err_code = nrf_drv_twi_tx(&m_twi, MCP7940_ADDRESS, buffer_send, sizeof(buffer_send), true);
    APP_ERROR_CHECK(err_code);
	
}

void SetTime()
{
	uint32_t err_code;
	
	timeStamp[0]  = DecToBcd(SECONDS);    // SECONDS
  timeStamp[1]  = DecToBcd (MINUTES);    // MINUTES
  timeStamp[2]  = DecToBcd(HOURS);    // HOURS
  timeStamp[3]  = DecToBcd(DAY);    // DAY OF WEEK (arbitrary value 1 - 7)
  timeStamp[4]  = DecToBcd( MONTHDATE);    // DAY
  timeStamp[5]  = DecToBcd( MONTH);    // MONTH
  timeStamp[6]  = DecToBcd(YEAR);    // YEAR

	err_code = write_register(MCP7940_RTCSEC,timeStamp[0]);
	APP_ERROR_CHECK(err_code);
	err_code = write_register(MCP7940_RTCMIN,timeStamp[1]);
	APP_ERROR_CHECK(err_code);
	err_code = write_register(MCP7940_RTCHOUR,timeStamp[2]);
	APP_ERROR_CHECK(err_code);
	err_code = write_register(MCP7940_RTCWKDAY,timeStamp[3]);
	APP_ERROR_CHECK(err_code);
	err_code = write_register(MCP7940_RTCDATE,timeStamp[4]);
	APP_ERROR_CHECK(err_code);
	err_code = write_register(MCP7940_RTCMTH,timeStamp[5]);
	APP_ERROR_CHECK(err_code);
	err_code = write_register(MCP7940_RTCYEAR,timeStamp[6]);
	APP_ERROR_CHECK(err_code);
	
}

uint32_t OscillatorRun ()
{
	ret_code_t err_code;
	err_code = write_register(MCP7940_RTCSEC,1<<7 );
	APP_ERROR_CHECK(err_code);
	return err_code;
}

