#ifndef KEY_HHHHH
#define KEY_HHHHH
#include <stdint.h>

#define KEY_1 1
#define KEY_2 2

typedef enum{
    KEY_PRESSED,KEY_RELEASEED
}key_action_t;

typedef void (*key_change_callback_function_t)(key_action_t pressorrelase,uint8_t keyvalue);
void key_init(key_change_callback_function_t keyscanfun);
void key_scan(void);


#endif