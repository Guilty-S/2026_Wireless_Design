#ifndef KEY_HHHHH
#define KEY_HHHHH
#include <stdint.h>

#define KEY_1 1
#define KEY_2 2
#define KEY_3 3

typedef enum {
    KEY_PRESSED,
    KEY_RELEASEED
} key_action_t;

//pressorrelase 1 按下 0 松开
typedef void (*key_change_callback_function_t)(key_action_t pressorrelase, uint8_t keyvalue);

void key_init(key_change_callback_function_t kayscanfun);

// 需要在10ms扫描函数调用
void key_scan(void);


#endif
