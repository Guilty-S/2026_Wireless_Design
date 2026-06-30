#include "key.h"
#include <stdint.h>
#include "stm32f1xx_hal.h"
#include <string.h>
#include "main.h"

#define KEY_COUNT 2
#define KEY_QUDOU_MAXCOUNT 3

typedef struct key_t
{
    GPIO_PinState key_state;
    uint8_t key_press_count;
    GPIO_TypeDef *gpio_port;
    uint16_t gpio_pin;
    uint8_t key_value;
}key_t;

typedef struct  key_scan_t
{
    key_t key[KEY_COUNT];
    key_change_callback_function_t key_scan_callback;
}key_scan_t;

key_scan_t g_keyscan;

void key_init(key_change_callback_function_t keyscanfun)
{
    memset(&g_keyscan,0,sizeof(key_scan_t));

    g_keyscan.key[0].gpio_port=KEY1_GPIO_Port;
    g_keyscan.key[0].gpio_pin=KEY1_Pin;
    g_keyscan.key[0].key_value=KEY_1;
    g_keyscan.key[0].key_state=HAL_GPIO_ReadPin(g_keyscan.key[0].gpio_port,g_keyscan.key[0].gpio_pin);

    g_keyscan.key[1].gpio_port=KEY2_GPIO_Port;
    g_keyscan.key[1].gpio_pin=KEY2_Pin;
    g_keyscan.key[1].key_value=KEY_2;
    g_keyscan.key[1].key_state=HAL_GPIO_ReadPin(g_keyscan.key[1].gpio_port,g_keyscan.key[1].gpio_pin);

    g_keyscan.key_scan_callback=keyscanfun;
}

void key_scan(void)
{
    uint8_t i;
    GPIO_PinState nowstate;
    for(i=0;i<KEY_COUNT;i++){
        nowstate=HAL_GPIO_ReadPin(g_keyscan.key[i].gpio_port,g_keyscan.key[i].gpio_pin);
        if(g_keyscan.key[i].key_state==nowstate){
            g_keyscan.key[i].key_press_count=0;
        }else{
            g_keyscan.key[i].key_press_count++;
            if(g_keyscan.key[i].key_press_count>=KEY_QUDOU_MAXCOUNT){
                g_keyscan.key[i].key_state=nowstate;
                if(nowstate==GPIO_PIN_RESET){
                    if(g_keyscan.key_scan_callback!=NULL){
                        g_keyscan.key_scan_callback(KEY_PRESSED,g_keyscan.key[i].key_value);
                    }
                }else{
                    if(g_keyscan.key_scan_callback!=NULL){
                        g_keyscan.key_scan_callback(KEY_RELEASEED,g_keyscan.key[i].key_value);
                    }
                }
            }
        }
    }
}