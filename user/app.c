//
// Created by Administrator on 2026/6/29.
//

#include "app.h"
#include "vMsgExec.h"
#include "string.h"
#include "elog.h"
#include "bsp.h"
#include "key.h"
#include "dht11.h"
#include "main.h"

void app_dlgcallback(VWM_MESSAGE *pMsg);

void key_change_callback(key_action_t pressorrelase, uint8_t keyvalue);

typedef struct app_t {
    VWM_HWIN main_hwnd;
    VWM_HTIMER blink_timer;
    VWM_HTIMER keyscan_timer;
    VWM_HTIMER sensor_timer;
    volatile uint8_t temp;
    volatile uint8_t humi;
} app_t;

app_t g_app;

void app_init() {
    memset(&g_app, 0, sizeof(app_t));
    g_app.main_hwnd = MsgExec_CreateModule(app_dlgcallback, 0);
    if (g_app.main_hwnd == 0) {
        log_e("app_init MsgExec_CreateMoudle error!");
    } else {
        log_i("app_init MsgExec_CreateMoudle ok!");
    }
    key_init(key_change_callback);
}

void app_dlgcallback(VWM_MESSAGE *pMsg) {
    uint16_t ledshowdelay;
    switch (pMsg->MsgId) {
        case VWM_INIT_DIALOG:
            g_app.blink_timer = VWM_CreateTimer(pMsg->hWin, 0, 500, 1);
            if (g_app.blink_timer == 0) {
                log_e("app_init blink_timer error!");
            } else {
                log_i("app_init blink_timer ok!");
                VWM_StartTimer(g_app.blink_timer);
            }
            g_app.keyscan_timer = VWM_CreateTimer(pMsg->hWin, 0, 10, 1);
            if (g_app.keyscan_timer == 0) {
                log_e("app_init keyscan_timer error!");
            } else {
                log_i("app_init keyscan_timer ok!");
                VWM_StartTimer(g_app.keyscan_timer);
            }
            g_app.sensor_timer = VWM_CreateTimer(pMsg->hWin, 0, 1000, 1);
            if (g_app.sensor_timer == 0) {
                log_e("app_init sensor_timer error!");
            } else {
                log_i("app_init sensor_timer ok!");
                VWM_StartTimer(g_app.sensor_timer);
            }
            break;

        case VWM_TIMER:
            if (pMsg->Data.v == g_app.blink_timer) {
                if(HAL_GPIO_ReadPin(LED_GPIO_Port,LED_Pin)==GPIO_PIN_SET){
                    if (g_app.humi >= 90) {
                        ledshowdelay = 100;
                    } else if (g_app.humi <= 50) {
                        ledshowdelay = 500;
                    } else {
                        ledshowdelay = (500 - (g_app.humi - 50) * (500 - 100) / (90 - 50));
                    }
                    if (ledshowdelay < 0) {
                        ledshowdelay = 100;
                    }
                    VWM_RestartTimer(g_app.blink_timer,ledshowdelay);
                    log_i("LED_Timer:%d",ledshowdelay);
                }
                LED_Toggle();
            } else if (pMsg->Data.v == g_app.keyscan_timer) {
                key_scan();
            } else if (pMsg->Data.v == g_app.sensor_timer) {
                if (DHT11_Read_Data(&g_app.temp, &g_app.humi) != 0) {
                    log_e("DHT11_Read Error!");
                } else {
                    log_i("Temp:%d,Humi:%d", g_app.temp, g_app.humi);
                }
            }
            break;
        default:
            break;
    }
}

void key_change_callback(key_action_t pressorrelase, uint8_t keyvalue) {
    if (pressorrelase == KEY_RELEASEED) {
        log_i("app_key_release_callback %d", keyvalue);
        if (keyvalue == KEY_1) {
            log_i("Temp:%d,Humi:%d", g_app.temp, g_app.humi);
        }
    } else if (pressorrelase == KEY_PRESSED) {

    }
}

