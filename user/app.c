#include "app.h"
#include "vMsgExec.h"
#include <string.h>
#define LOG_TAG          "APP"
#include "elog.h"
#include "bsp.h"
#include "key.h"
#include "dht11.h"
#include "main.h"
#include "cmd_queue.h"
#include "uart_interface.h"
#include "cmd_process.h"
#include "radio.h"
#include "dyn_mem.h"

void app_uart_handle_effective_frame_callback(void* revobj, uint8_t *revbuff, uint16_t revlen);

void app_key_change_callback(key_action_t pressorrelase,uint8_t keyvalue);

void app_dlgcallback(VWM_MESSAGE * pMsg);

#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

typedef struct{
    int TX_Output_Power;
    int LORA_Preamble_Length;
    int CRCFlag;
    int LORA_BandWidth;
    int LORA_Spreading_Factor;
    int LORA_CodingRate;
    int FreqHopOn;
    int HopPeriod;
    int Mode;
    int RF_Frequency;
    int AutoRX;
}LoraParam_t;
static RadioEvents_t RadioEvents;

#pragma pack(1)
typedef struct lora_msg_frame_t{
    uint8_t network_id;
    uint8_t source_addr;
    uint8_t dest_addr;

    uint8_t temp;
    uint8_t humi;
}lora_msg_frame_t;
#pragma pack()

typedef struct lora_network_config_t{
    uint8_t network_id;
    uint8_t source_addr;
}lora_network_config_t;

#pragma pack(1)
typedef struct lora_rev_msg_t{
    uint16_t size;
    int16_t rssi;
    int8_t snr;
    uint8_t *payload[1];
}lora_rev_msg_t;
#pragma pack()

typedef struct  app_t
{
    VWM_HWIN main_hwnd;
    VWM_HTIMER blink_timer;
    VWM_HTIMER keyscan_timer;
    VWM_HTIMER sensor_timer;

    volatile uint8_t temp;
    volatile uint8_t humi;

    uart_interface_t lcd_uart;
    uint8_t lcd_cmd_buffer[CMD_MAX_SIZE];

    LoraParam_t WorkLoraParam;

    uint8_t device_role;
    uint8_t sensor_collect_count;

    lora_network_config_t lora_network_config;
}app_t;
app_t g_app;

#define VWM_LORA_REV_DATA			(VWM_USER + 0x1002)

void OnTxDone(void)
{
    Radio.Sleep();
    Radio.Rx(0);
    log_i("Radio Tx Done");
}

void OnTxTimeout(void)
{
    Radio.Sleep();
    log_i("Radio Tx Timeout");
    if(g_app.device_role==1){
        Radio.Rx(0);
    }
}

void OnRxTimeout(void)
{
    Radio.Sleep();
    log_i("Radio Rx Timeout");
}

void OnRxError(void)
{
    Radio.Sleep();
    log_i("Radio Rx Error");
    Radio.Rx(0);
}

void OnRxDone(uint8_t *payload,uint16_t size,int16_t rssi,int8_t snr)
{
    lora_rev_msg_t *msg;
    Radio.Sleep();

    msg=(lora_rev_msg_t*)dm_alloc(sizeof(lora_rev_msg_t)-1+size);
    msg->size=size;
    msg->rssi=rssi;
    msg->snr=snr;
    memcpy(msg->payload,payload,size);

    VWM_SendMessageP(VWM_LORA_REV_DATA,g_app.main_hwnd,g_app.main_hwnd,msg,1);
}

void app_update_channel(void)
{
    g_app.WorkLoraParam.FreqHopOn = 0;
    g_app.WorkLoraParam.AutoRX = 1;
    g_app.WorkLoraParam.CRCFlag = 1;
    g_app.WorkLoraParam.LORA_BandWidth = 1;//125Khz
    g_app.WorkLoraParam.LORA_CodingRate = 1;//4/5
    g_app.WorkLoraParam.LORA_Preamble_Length= 10;
    g_app.WorkLoraParam.LORA_Spreading_Factor = 12;
    g_app.WorkLoraParam.Mode = 1;//lora
    g_app.WorkLoraParam.RF_Frequency = 435000000;//424600000;//478000000;
    g_app.WorkLoraParam.TX_Output_Power = 10;
    g_app.WorkLoraParam.HopPeriod = 0;


    Radio.SetChannel(g_app.WorkLoraParam.RF_Frequency);
    Radio.SetMaxPayloadLength(MODEM_LORA, 255);


    Radio.SetTxConfig( MODEM_LORA, g_app.WorkLoraParam.TX_Output_Power, 0, g_app.WorkLoraParam.LORA_BandWidth,
                       g_app.WorkLoraParam.LORA_Spreading_Factor, g_app.WorkLoraParam.LORA_CodingRate,
                       g_app.WorkLoraParam.LORA_Preamble_Length, LORA_FIX_LENGTH_PAYLOAD_ON,
                       g_app.WorkLoraParam.CRCFlag, g_app.WorkLoraParam.FreqHopOn, g_app.WorkLoraParam.HopPeriod, LORA_IQ_INVERSION_ON, 6000 );
    Radio.SetRxConfig( MODEM_LORA, g_app.WorkLoraParam.LORA_BandWidth, g_app.WorkLoraParam.LORA_Spreading_Factor,
                       g_app.WorkLoraParam.LORA_CodingRate, 0, g_app.WorkLoraParam.LORA_Preamble_Length,
                       LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                       0, g_app.WorkLoraParam.CRCFlag, g_app.WorkLoraParam.FreqHopOn, g_app.WorkLoraParam.HopPeriod, LORA_IQ_INVERSION_ON, true );

}

void LoRa_Config(void)
{
    RadioEvents.TxDone=OnTxDone;
    RadioEvents.RxDone=OnRxDone;
    RadioEvents.TxTimeout=OnTxTimeout;
    RadioEvents.RxTimeout=OnRxTimeout;
    RadioEvents.RxError=OnRxError;
    RadioEvents.CadDone=NULL;
    Radio.Init(&RadioEvents);
    Radio.Sleep();

    app_update_channel();
}

void app_init()
{
    memset(&g_app,0,sizeof(app_t));
    g_app.main_hwnd=MsgExec_CreateModule(app_dlgcallback,0);
    g_app.device_role=1;
    if(g_app.main_hwnd==NULL){
        log_e("app_init MsgExec_CreateModule error!");
    }else{
        log_i("app_init MsgExec_CreateModule ok!");
    }
    key_init(app_key_change_callback);

    queue_reset();
    uart_interface__init(&g_app.lcd_uart,&huart2,g_app.main_hwnd,app_uart_handle_effective_frame_callback,0,NULL,0,NULL);

    uart_interface__start_receive(&g_app.lcd_uart);

    g_app.lora_network_config.network_id=0x10;
    if(g_app.device_role==1){
        g_app.lora_network_config.source_addr=0x00;
    }else{
        g_app.lora_network_config.source_addr=0x01;
    }

    LoRa_Config();

    if(g_app.device_role==1){
        Radio.Rx(0);
    }
    
    if(g_app.device_role==1){
        SetTextValue(0,6,"Íø¹Ø");
    }else{
        SetTextValue(0,6,"ÖÕ¶Ë");
    }
}

void app_rev_lora_msg(lora_rev_msg_t *msg)
{
    lora_msg_frame_t *revframe;
    lora_msg_frame_t sendframe;
    if(msg==NULL)
        return;
    log_i("Rev Lora Message,size:%d,rssi:%d,snr:%d",msg->size,msg->rssi,msg->snr);
    revframe=(lora_msg_frame_t *)msg->payload;
    log_i("package field=network_id:%d,source_addr:%d,dest_addr:%d,temp:%d,humi:%d",
    revframe->network_id,revframe->source_addr,revframe->dest_addr,revframe->temp,revframe->humi);
    if(revframe->dest_addr==g_app.lora_network_config.source_addr&&
       revframe->network_id==g_app.lora_network_config.network_id){
        SetTextInt32(0,4,revframe->temp,0,2);
        SetTextInt32(0,5,revframe->humi,0,2);

        if(g_app.device_role==1){
            sendframe.network_id=g_app.lora_network_config.network_id;
            sendframe.source_addr=g_app.lora_network_config.source_addr;
            sendframe.dest_addr=revframe->source_addr;
            sendframe.temp=g_app.temp;
            sendframe.humi=g_app.humi;

            Radio.Send((uint8_t *)&sendframe,sizeof(lora_msg_frame_t));
        }
    }else{
        log_e("dest_addr is not me,me addr:%d,me networkid:%d",g_app.lora_network_config.source_addr,g_app.lora_network_config.network_id);
    }
}

void app_dlgcallback(VWM_MESSAGE * pMsg)
{
    int16_t ledshowdelay;
    lora_msg_frame_t sendframe;
    switch (pMsg->MsgId)
    {
    case VWM_INIT_DIALOG:
        g_app.blink_timer=VWM_CreateTimer(pMsg->hWin,0,500,1);
        if(g_app.blink_timer==NULL){
            log_e("app_init create blink_timer error!");
        }else{
            log_i("app_init create blink_timer ok!");
            VWM_StartTimer(g_app.blink_timer);
        }

        g_app.keyscan_timer=VWM_CreateTimer(pMsg->hWin,0,10,1);
        if(g_app.keyscan_timer==NULL){
            log_e("app_init create keyscan_timer error!");
        }else{
            log_i("app_init create keyscan_timer ok!");
            VWM_StartTimer(g_app.keyscan_timer);
        }

        g_app.sensor_timer=VWM_CreateTimer(pMsg->hWin,0,1000,1);
        if(g_app.sensor_timer==NULL){
            log_e("app_init create sensor_timer error!");
        }else{
            log_i("app_init create sensor_timer ok!");
            VWM_StartTimer(g_app.sensor_timer);
        }
        break;
    case VWM_TIMER:
        if(pMsg->Data.v==g_app.blink_timer){
            if(HAL_GPIO_ReadPin(LED_GPIO_Port,LED_Pin)==GPIO_PIN_SET){
               if(g_app.humi>=90){
                  ledshowdelay=100;
               }else if(g_app.humi<=50){
                  ledshowdelay=500;
               }else{
                  ledshowdelay=(500-(g_app.humi-50)*(500-100)/(90-50));
                  if(ledshowdelay<0){
                     ledshowdelay=100;
                  }
               }
               VWM_RestartTimer(g_app.blink_timer,ledshowdelay);
            }
            led_toggole();
        }else if(pMsg->Data.v==g_app.keyscan_timer){
            key_scan();
        }else if(pMsg->Data.v==g_app.sensor_timer){
            if(DHT11_Read_Data((uint8_t *)&g_app.temp,(uint8_t *)&g_app.humi)!=0){
                log_e("DHT11_Read_Data error!");
            }else{
                log_i("Temp:%d;Humi:%d",g_app.temp,g_app.humi);
                SetTextInt32(0,2,g_app.temp,0,2);
                SetTextInt32(0,3,g_app.humi,0,2);

                g_app.sensor_collect_count++;

                if(g_app.device_role==0){
                    if(g_app.sensor_collect_count%2==0){
                        sendframe.network_id=g_app.lora_network_config.network_id;
                        sendframe.source_addr=g_app.lora_network_config.source_addr;
                        sendframe.dest_addr=0;
                        sendframe.temp=g_app.temp;
                        sendframe.humi=g_app.humi;

                        Radio.Send((uint8_t *)&sendframe,sizeof(lora_msg_frame_t));
                    }
                }
            }
        }
        break;
    case VWM_UART_REV_DATA:
		
        uart_interface__handle_data_from_uart((uart_revmsg_t*)pMsg->Data.p);
		break;
    case VWM_LORA_REV_DATA:	
        app_rev_lora_msg(pMsg->Data.p);
        break;
    default:
        break;
    }
}


void test_lcd()
{
  uint8_t sendbuff[]={0xEE,0xB1,0x10,0x00,0x00,0x00,0x02,0x32,0x38,0xFF,0xFC,0xFF,0xFF };
  HAL_UART_Transmit(&huart2,(uint8_t *)sendbuff,sizeof(sendbuff),sizeof(sendbuff)*10+100);
}

void app_key_change_callback(key_action_t pressorrelase,uint8_t keyvalue)
{
    if(pressorrelase==KEY_RELEASEED){
        log_i("app_key_release callback %d",keyvalue);
        if(keyvalue==KEY_1){
            //test_lcd();
            //log_i("Temp:%d;Humi:%d",g_app.temp,g_app.humi);
            //SetTextInt32(0,2,g_app.temp,0,2);
            //SetTextValue(0,2,"28");
        }
    }
}

void NotifyButton(uint16 screen_id, uint16 control_id, uint8  state)
{
    if (state == 1) {
        log_i("Button Pressed, ScreenID:%d; ControlID:%d", screen_id, control_id);
    } else {
        log_i("Button Released, ScreenID:%d; ControlID:%d", screen_id, control_id);
    }
}

void NOTIFYHandShake()
{
   SetButtonValue(3,2,1);
}

void ProcessMessage( PCTRL_MSG msg, uint16 size )
{
    uint8 cmd_type = msg->cmd_type;                                                  //
    uint8 ctrl_msg = msg->ctrl_msg;                                                  //
    uint8 control_type = msg->control_type;                                          //
    uint16 screen_id = PTR2U16(&msg->screen_id);                                     //ID
    uint16 control_id = PTR2U16(&msg->control_id);                                   //ID
    uint32 value = PTR2U32(msg->param);                                              //


    switch(cmd_type)
    {  
    case NOTIFY_TOUCH_PRESS:                                                        //
    case NOTIFY_TOUCH_RELEASE:                                                      //
        //NotifyTouchXY(cmd_buffer[1],PTR2U16(cmd_buffer+2),PTR2U16(cmd_buffer+4)); 
        break;                                                                    
    case NOTIFY_WRITE_FLASH_OK:                                                     //§ÕFLASH
        //NotifyWriteFlash(1);                                                      
        break;                                                                    
    case NOTIFY_WRITE_FLASH_FAILD:                                                  //§ÕFLASH
        //NotifyWriteFlash(0);                                                      
        break;                                                                    
    case NOTIFY_READ_FLASH_OK:                                                      //FLASH
        //NotifyReadFlash(1,cmd_buffer+2,size-6);                                     //¦Â
        break;                                                                    
    case NOTIFY_READ_FLASH_FAILD:                                                   //FLASH
        //NotifyReadFlash(0,0,0);                                                   
        break;                                                                    
    case NOTIFY_READ_RTC:                                                           //RTC
        //NotifyReadRTC(cmd_buffer[2],cmd_buffer[3],cmd_buffer[4],cmd_buffer[5],cmd_buffer[6],cmd_buffer[7],cmd_buffer[8]);
        break;
    case NOTIFY_CONTROL:
        {
            if(ctrl_msg==MSG_GET_CURRENT_SCREEN)                                    //ID£
            {
                //NotifyScreen(screen_id);                                            //§Ý
            }
            else
            {
                switch(control_type)
                {
                case kCtrlButton:                                                   //
                    NotifyButton(screen_id,control_id,msg->param[1]);                  
                    break;                                                             
                case kCtrlText:                                                     //
                    //NotifyText(screen_id,control_id,msg->param);                       
                    break;                                                             
                case kCtrlProgress:                                                 //
                    //NotifyProgress(screen_id,control_id,value);                        
                    break;                                                             
                case kCtrlSlider:                                                   //
                    //NotifySlider(screen_id,control_id,value);                          
                    break;                                                             
                case kCtrlMeter:                                                    //
                    //NotifyMeter(screen_id,control_id,value);                           
                    break;                                                             
                case kCtrlMenu:                                                     //
                    //NotifyMenu(screen_id,control_id,msg->param[0],msg->param[1]);      
                    break;                                                              
                case kCtrlSelector:                                                 //
                    //NotifySelector(screen_id,control_id,msg->param[0]);                
                    break;                                                              
                case kCtrlRTC:                                                      //
                    //NotifyTimer(screen_id,control_id);
                    break;
                default:
                    break;
                }
            } 
            break;  
        } 
    case NOTIFY_HandShake:                                                          //                                                     
        NOTIFYHandShake();
        break;
    default:
        break;
    }
}

void app_uart_handle_effective_frame_callback(void* revobj, uint8_t *revbuff, uint16_t revlen)
{
    uint16_t i;
    qsize size=0;
    for(i=0;i<revlen;i++){
        queue_push(revbuff[i]);
        size=queue_find_cmd(g_app.lcd_cmd_buffer,CMD_MAX_SIZE);
        if(size>0&&g_app.lcd_cmd_buffer[1]!=0x07){
            ProcessMessage((PCTRL_MSG)g_app.lcd_cmd_buffer,size);
        }else if(size>0&&g_app.lcd_cmd_buffer[1]==0x07){
            __disable_fault_irq();
            NVIC_SystemReset();
        }
    }
}

void HAL_UART_IDLECallBack(UART_HandleTypeDef *huart)
{
    if(huart==&huart2){
        uart_interface__rev_data(&g_app.lcd_uart,false);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart==&huart2){
        uart_interface__rev_data(&g_app.lcd_uart,true);
    }
}
