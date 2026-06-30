#ifndef UART_INTERFACE_H
#define UART_INTERFACE_H
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "vMsgExec.h"

#define xUSING_CUSTOM_FRAME_FORMAT //是否使用自定义的帧定界符

#define UART_RECEIVE_MAXSIZE (255+16)



//------------------------------------------------------------------------
//UART串口对象实现开始
//------------------------------------------------------------------------

//消息引擎消息定义
#define VWM_UART_REV_DATA			(VWM_USER + 0x1001)

//串口通信的帧头
#define RADIO_FRAME_HEAD 0x55
#define RADIO_FRAME_TAIL 0xAA

typedef enum revmsg_process_state_t 
{
	SEARCH_FRAMEHEAD,///< 正在寻找帧头
	SEARCH_FRAMETAIL,///< 已寻找到帧头, 正在寻找帧尾
	SEARCH_FRAMETAIL_MODBUS,///< 已寻找到帧头, 正在寻找帧尾,Modbus模式
}revmsg_process_state_t;

#pragma pack(1)
typedef struct
{
	struct uart_interface_t *uartif;
	uint16_t payload_length;
	uint8_t  payload[1];
}uart_revmsg_t;
#pragma pack()


typedef struct uart_rev_process_t
{
    volatile revmsg_process_state_t     uart_revmsg_state;///< 串口接收数据状态
    volatile uart_revmsg_t*			    uart_revmsg;///<正在接收的串口数据
    volatile uint8_t				    uart_effective_data[UART_RECEIVE_MAXSIZE];	
	volatile uint16_t					uart_effective_write_index;///< 数据接收长度
    volatile uint8_t					uart_prekey;
}uart_rev_process_t;

typedef void (*uart_handle_effective_frame_callback_t)(void* revobj, uint8_t *revbuff, uint16_t revlen);

typedef void (*uart_handle_rec_one_byte_t)(struct uart_interface_t *uartif, uint8_t key);

/**
 * LoRa模块1~10，用L1~L10表示
 * L1~L3 挂载到UART3
 * L4~L6 挂载到UART6
 * L7、L8挂载到UART1
 * L9 挂载到UART4
 * L10 挂载到UART5
 * 
 * UART2接上位机
 * */
typedef struct uart_interface_t {
    UART_HandleTypeDef *				huart;///<使用的单片机UART接口对象
    uart_rev_process_t *				rev_process;//接收状态对象
	VWM_HWIN 							rev_hwin;//接收串口消息的窗口句柄
	uart_handle_effective_frame_callback_t 		handle_effective_frame_callback;//接收完整一帧数据后的回调函数
	uint32_t							rev_all_count;
	uint16_t							rev_last_length;
	bool								is_485;//是否是485
	GPIO_TypeDef* 						REDE_GPIO_Port;//是485的话，这里设置控制读写的端口
	uint16_t							REDE_Pin;//是485的话，这里设置控制读写的管脚
	void*								rev_obj;//回调函数的接收对象指针
}uart_interface_t;


uart_revmsg_t * uart_interface__alloc_rev_uartmsg(uint16_t payloadlength);

void uart_interface__init(uart_interface_t *uartif, 
	UART_HandleTypeDef* huart, 
	VWM_HWIN rev_hwin, 
	uart_handle_effective_frame_callback_t handle_effective_frame_callback, 
	bool is_485, 
	GPIO_TypeDef* REDE_GPIO_Port, 
	uint16_t REDE_Pin,
	void* rev_obj);

void uart_interface__start_receive(uart_interface_t *uartif);


/**
 * @brief 处理接口从串口发来的数据，在串口中断中被调用
 * 
 * @param iscomplete 是否已经完成接收
 * @param 
 * @retval void
 */
void uart_interface__rev_data(uart_interface_t *uartif, bool iscomplete);

/**
 * @brief UART接口从串口接收到数据的处理，由接收串口消息的模块调用，在串口接收消息处理函数中调用
 * 
 * @param revframe 从无线模块接收到的数据
 */
void uart_interface__handle_data_from_uart(uart_revmsg_t *revframe);

/**
 * @brief  通过串口发送数据
 * @note   
 * @param  *uartif: 串口接口
 * @param  psend: 要发送的数据
 * @param  data_len: 要发送的数据的长度
 * @retval 
 */
uint8_t uart_interface__send_data(uart_interface_t *uartif, uint8_t* psend, uint16_t data_len);

/**
 * @brief  发送一帧数据
 * @note   自动增加帧头和帧尾
 * @param  *uartif: 串口接口
 * @param  psend: 要发送的数据
 * @param  data_len: 要发送的数据的长度
 * @retval 
 */
uint8_t uart_interface__send_one_frame(uart_interface_t *uartif,  uint8_t* psend, uint16_t data_len);

void uart_interface__free(uart_interface_t *uartif);

#endif
