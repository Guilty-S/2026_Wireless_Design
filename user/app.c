//
// Created by Administrator on 2026/6/29.
//

#include "app.h"
#include "vMsgExec.h"
#include "string.h"
#include "elog.h"
#include "bsp.h"

void app_dlgcallback(VWM_MESSAGE* pMsg);
typedef struct app_t{
    VWM_HWIN main_hwnd;
    VWM_HTIMER blink_timer;
}app_t;

app_t g_app;

void app_init(){
    memset(&g_app,0,sizeof(app_t));
    g_app.main_hwnd=MsgExec_CreateModule(app_dlgcallback, 0);
    if(g_app.main_hwnd==0){
        log_e("app_init MsgExec_CreateMoudle error!");
    }else{
        log_i("app_init MsgExec_CreateMoudle ok!");
    }
}
void app_dlgcallback(VWM_MESSAGE* pMsg){
    switch (pMsg->MsgId) {
        case VWM_INIT_DIALOG:
            g_app.blink_timer= VWM_CreateTimer(pMsg->hWin,0,500,1);
            if(g_app.blink_timer==0){
                log_e("app_init blink_timer error!");
            }else{
                log_i("app_init blink_timer ok!");
                VWM_StartTimer(g_app.blink_timer);
            }
            break;
        case VWM_TIMER:
            LED_Toggle();
            break;
        default:
            break;
    }
}
