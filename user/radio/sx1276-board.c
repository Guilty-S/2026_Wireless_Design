#include "radio.h"
#include "sx1276/sx1276.h"
#include "sx1276-board.h"
#include "bsp.h"
#include "main.h"

//extern volatile uint32_t  SendBeginTick;
//extern volatile uint16  InterrCount;
extern volatile uint8_t  InterrFlag;
/*!
 * Flag used to set the RF switch control pins in low power mode when the radio is not active.
 */
//static bool RadioIsActive = false;



void SX1276BoardInit( void )
{
	SX1276.Spi = &hspi1;
}

uint8_t SX1276GetPaSelect( uint32_t channel )
{
    if( channel < RF_MID_BAND_THRESH )
    {
        return RF_PACONFIG_PASELECT_PABOOST;
    }
    else
    {
        return RF_PACONFIG_PASELECT_RFO;
    }
}

void SX1276SetAntSwLowPower( bool status )
{
//    if( RadioIsActive != status )
//    {
//        RadioIsActive = status;
//    
//        if( status == false )
//        {
//            SX1276AntSwInit( );
//        }
//        else
//        {
//            SX1276AntSwDeInit( );
//        }
//    }
}


void SX1276SetAntSw( uint8_t rxTx )
{
//    if( rxTx != 0 ) // 1: TX, 0: RX
//    {
//        GpioWrite( &AntSwitchLf, 0 );
//        GpioWrite( &AntSwitchHf, 1 );
//    }
//    else
//    {
//        GpioWrite( &AntSwitchLf, 1 );
//        GpioWrite( &AntSwitchHf, 0 );
//    }
}

bool SX1276CheckRfFrequency( uint32_t frequency )
{
    // Implement check. Currently all frequencies are supported
    return true;
}

//中断函数
//调用对应的回调函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	switch (GPIO_Pin){
		case VRADIO_DIO_0_Pin:
			SX1276OnDio0Irq();
			break;
		case VRADIO_DIO_1_Pin:
			SX1276OnDio1Irq();
			break;
		// case VRADIO_DIO_2_Pin:
		// 	SX1276OnDio2Irq();
		// 	break;
		case VRADIO_DIO_3_Pin:
			SX1276OnDio3Irq();
			break;
        
//         case GPIO_PIN_2:
// //            devicemac_auto_updata();       
//             break;
//		case VRADIO_DIO_4_Pin:	
//SX1276OnDio4Irq();
//			break;
		
//		case VRADIO_DIO_4_Pin:
			//SX1276OnDio4Irq();
//			break;
		// case VRADIO_DIO_5_Pin:
		// 	SX1276OnDio5Irq();
		// 	break;
			
	}
}

