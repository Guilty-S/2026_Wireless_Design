#include "uart_interface.h"
#include "dyn_mem.h"
#include <string.h>
#include "vMsgExec.h"

void uart_interface__init(uart_interface_t *uartif, 
	UART_HandleTypeDef* huart, 
	VWM_HWIN rev_hwin, 
	uart_handle_effective_frame_callback_t handle_effective_frame_callback, 
	bool is_485, 
	GPIO_TypeDef* REDE_GPIO_Port, 
	uint16_t REDE_Pin,
	void* rev_obj)
{
	if (uartif == NULL)	return;

	memset(uartif, 0, sizeof(uart_interface_t));
	uartif->huart = huart;
	uartif->rev_process = (uart_rev_process_t*)dm_alloc(sizeof(uart_rev_process_t));

	uartif->rev_hwin = rev_hwin;
	uartif->handle_effective_frame_callback = handle_effective_frame_callback;
	uartif->rev_obj = rev_obj;
	
	uartif->is_485 = is_485;
	uartif->REDE_GPIO_Port = REDE_GPIO_Port;
	uartif->REDE_Pin = REDE_Pin;	

	
}



/**
 * @brief 分配一个串口接收所使用的消息
 * 
 * @param payloadlength 要分配的负载的长度
 * @return 分配好的消息对象
 */
uart_revmsg_t * uart_interface__alloc_rev_uartmsg(uint16_t payloadlength)
{
	uart_revmsg_t *rs;

	rs = (uart_revmsg_t*)dm_alloc(sizeof(uart_revmsg_t) - 1 + payloadlength);

	memset(rs, 0, sizeof(uart_revmsg_t) - 1 + payloadlength);

	return rs;
}



/**
 * @brief 处理接口从串口发来的数据
 * 
 * @param iscomplete 是否已经完成接收
 * @param 
 * @retval void
 */
void uart_interface__rev_data(uart_interface_t *uartif, bool iscomplete)
{	
	uart_revmsg_t *tmp_uart_revmsg = NULL;
	uint16_t revsize;

	uartif->rev_all_count++;
	if (iscomplete == true) {
		revsize = 0;
	}
	else {
		revsize = __HAL_DMA_GET_COUNTER(uartif->huart->hdmarx);
		if (UART_RECEIVE_MAXSIZE == revsize) {
			//说明没有接收到任何数据，应该还在接收状态，直接返回，不在处理
			return;
		}
		//HAL_UART_AbortReceive_IT(uartif->huart);//调用这个函数终止接收F4的HAL库会有问题，会再次进入DMA停止中断，改为下面的语句
		HAL_UART_AbortReceive(uartif->huart);
	}
		

	if (uartif->rev_process->uart_revmsg != NULL) 
	{
		tmp_uart_revmsg = (uart_revmsg_t *)uartif->rev_process->uart_revmsg;
		tmp_uart_revmsg->payload_length = UART_RECEIVE_MAXSIZE - revsize;
		uartif->rev_last_length = tmp_uart_revmsg->payload_length;
		uartif->rev_process->uart_revmsg = NULL;
	}
	
	
	uartif->rev_process->uart_revmsg = uart_interface__alloc_rev_uartmsg(UART_RECEIVE_MAXSIZE);

	if (uartif->rev_process->uart_revmsg != NULL)
		HAL_UART_Receive_DMA(uartif->huart, (uint8_t*)uartif->rev_process->uart_revmsg->payload, UART_RECEIVE_MAXSIZE);
	else {
		//return;
	}
	if (tmp_uart_revmsg != NULL) 
	{
		tmp_uart_revmsg->uartif = uartif;
		if (uartif->rev_hwin != NULL) {
			
		// printf("receive data=");
		// for(int i=0;i<tmp_uart_revmsg->payload_length;i++)
		// {
		// 	printf("%02x ",(uint8_t)tmp_uart_revmsg->payload[i]);
		// }
		// printf("\r\n");
			
	
			VWM_SendMessageP(VWM_UART_REV_DATA, uartif->rev_hwin, uartif->rev_hwin, tmp_uart_revmsg, 1);				
		}
	}
}


void uart_interface__start_receive(uart_interface_t *uartif)
{
	
	if (uartif->rev_process->uart_revmsg != NULL) {
		dm_free((void*)uartif->rev_process->uart_revmsg);
		uartif->rev_process->uart_revmsg = NULL;
	}
	memset(uartif->rev_process, 0, sizeof(uart_rev_process_t));
	uartif->rev_process->uart_revmsg = uart_interface__alloc_rev_uartmsg(UART_RECEIVE_MAXSIZE);

	__HAL_UART_CLEAR_IDLEFLAG(uartif->huart);
	__HAL_UART_ENABLE_IT(uartif->huart, UART_IT_IDLE);

	if (uartif->rev_process->uart_revmsg != NULL) {
		if (uartif->is_485) {
			uartif->REDE_GPIO_Port->BSRR = ((uint32_t)uartif->REDE_Pin << 16);
		}
		
		HAL_UART_Receive_DMA(uartif->huart, 
			(uint8_t*)uartif->rev_process->uart_revmsg->payload, UART_RECEIVE_MAXSIZE);
	}
	else {
		//return;
	}	
}

void uart_interface__handle_one_byte(uart_interface_t *uartif, uint8_t key)
{
	
	if(uartif->rev_process->uart_revmsg_state == SEARCH_FRAMEHEAD)///< 正在查找帧头
	{
		///< 找到帧头
		if(key == RADIO_FRAME_HEAD)
			if(uartif->rev_process->uart_prekey == RADIO_FRAME_HEAD) {
				uartif->rev_process->uart_effective_write_index = 0;
				uartif->rev_process->uart_revmsg_state = SEARCH_FRAMETAIL;
			}
	}
	else if(uartif->rev_process->uart_revmsg_state == SEARCH_FRAMETAIL)
	{
		uartif->rev_process->uart_effective_data[uartif->rev_process->uart_effective_write_index] = key;
		uartif->rev_process->uart_effective_write_index++;

		if (uartif->rev_process->uart_effective_write_index >= UART_RECEIVE_MAXSIZE) {
			uartif->rev_process->uart_revmsg_state = SEARCH_FRAMEHEAD;			
		}
		else {
			///< 找到帧尾
			if(key == RADIO_FRAME_TAIL && uartif->rev_process->uart_prekey == RADIO_FRAME_TAIL)
			{				
				if (uartif->handle_effective_frame_callback != NULL) {
					uartif->handle_effective_frame_callback(uartif->rev_obj, (uint8_t*)uartif->rev_process->uart_effective_data, uartif->rev_process->uart_effective_write_index);
				}
				uartif->rev_process->uart_revmsg_state = SEARCH_FRAMEHEAD;
			}
		}
	}

	uartif->rev_process->uart_prekey = key;
}


void uart_interface__handle_data_from_uart(uart_revmsg_t *revmsg)
{
	uint16_t i;
	uart_interface_t *uartif;
	uartif = revmsg->uartif;

	#ifdef USING_CUSTOM_FRAME_FORMAT
	if (revmsg->payload_length > 1) {
		i = revmsg->payload_length;
	}
	for(i=0; i < revmsg->payload_length; i++)///< 不能从 0开始, 会造成越界
	{
		uart_interface__handle_one_byte(uartif, revmsg->payload[i]);
	}
	#else
	if (uartif->handle_effective_frame_callback != NULL) {
//		printf("receive data = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x \r\n",
//		 revmsg->payload[0],revmsg->payload[1],revmsg->payload[2],revmsg->payload[3],revmsg->payload[4],revmsg->payload[5],
//		 revmsg->payload[6],revmsg->payload[7],revmsg->payload[8],revmsg->payload[9],revmsg->payload[10],revmsg->payload[11],
//		 revmsg->payload[12],revmsg->payload[13]);
		uartif->handle_effective_frame_callback(uartif->rev_obj,revmsg->payload, revmsg->payload_length);
	}
	#endif
//	uint16_t i = 0;
//	uart_interface_t *uartif;
//	uartif = revmsg->uartif;
//	if (revmsg->payload_length > 1) {
//		i = revmsg->payload_length;
//	}
//	for(i = 0; i < revmsg->payload_length; i++)///< 不能从 0开始, 会造成越界
//	{
//		uartif->handle_rec_one_byte_t(uartif, revmsg->payload[i]);
//	}
}



/**
 * @brief  通过串口发送数据
 * @note   
 * @param  *uartif: 串口接口
 * @param  psend: 要发送的数据
 * @param  data_len: 要发送的数据的长度
 * @retval 
 */
uint8_t uart_interface__send_data(uart_interface_t *uartif,  uint8_t* psend, uint16_t data_len)
{
	HAL_StatusTypeDef result;
    //将要发送的数据放入队列
    //调用UART调度函数
	if (uartif->is_485) {		
		uartif->REDE_GPIO_Port->BSRR = (uint32_t)uartif->REDE_Pin;
		osDelay(1);
	}
	
	result = HAL_UART_Transmit(uartif->huart, psend, data_len, data_len*10);
	
	if (uartif->is_485) {
		uartif->REDE_GPIO_Port->BSRR = ((uint32_t)uartif->REDE_Pin << 16);
	}

	if (result == HAL_OK) {
		return 0;
	}
	else {
		return 1;
	}	
}


/**
 * @brief  发送一帧数据
 * @note   自动增加帧头和帧尾
 * @param  *uartif: 串口接口
 * @param  psend: 要发送的数据
 * @param  data_len: 要发送的数据的长度
 * @retval 
 */
uint8_t uart_interface__send_one_frame(uart_interface_t *uartif,  uint8_t* psend, uint16_t data_len)
{
	HAL_StatusTypeDef result;
	uint16_t frameheadtail;
    //将要发送的数据放入队列
    //调用UART调度函数
	if (uartif->is_485) {		
//		uartif->REDE_GPIO_Port->BSRR = (uint32_t)uartif->REDE_Pin;
		HAL_GPIO_WritePin(uartif->REDE_GPIO_Port, uartif->REDE_Pin, GPIO_PIN_SET);
		osDelay(1);
	}
	//发送帧头
	frameheadtail = (RADIO_FRAME_HEAD << 8) + RADIO_FRAME_HEAD;
	result = HAL_UART_Transmit(uartif->huart, (uint8_t*)&frameheadtail, 2, 2*10);
	if (result != HAL_OK) {
		if (uartif->is_485) {
			HAL_GPIO_WritePin(uartif->REDE_GPIO_Port, uartif->REDE_Pin, GPIO_PIN_RESET);
		}	
		return 1;
	}
	

	result = HAL_UART_Transmit(uartif->huart, psend, data_len, data_len*10);
	if (result != HAL_OK) {
		if (uartif->is_485) {
			HAL_GPIO_WritePin(uartif->REDE_GPIO_Port, uartif->REDE_Pin, GPIO_PIN_RESET);
		}	
		return 1;
	}


	//发送帧头
	frameheadtail = (RADIO_FRAME_TAIL << 8) + RADIO_FRAME_TAIL;
	result = HAL_UART_Transmit(uartif->huart, (uint8_t*)&frameheadtail, 2, 2*10);
	if (result != HAL_OK) {
		if (uartif->is_485) {
			uartif->REDE_GPIO_Port->BSRR = ((uint32_t)uartif->REDE_Pin << 16);
		}	
		return 1;
	}

	if (uartif->is_485) {
		uartif->REDE_GPIO_Port->BSRR = ((uint32_t)uartif->REDE_Pin << 16);
	}

	if (result == HAL_OK) {
		return 0;
	}
	else {
		return 1;
	}	
}


/**
 * @brief 串口停止接收，释放串口对象的资源,但不是放串口对象自身
 * 
 * @param uartif 
 */
void uart_interface__free(uart_interface_t *uartif)
{
	if (uartif == NULL)
		return;
	
	// 关闭串口接收
	if (uartif->huart != NULL)
	{
		HAL_UART_AbortReceive(uartif->huart);
	}
	if (uartif->rev_process->uart_revmsg != NULL) {
			dm_free((uint8_t*)uartif->rev_process->uart_revmsg);
			uartif->rev_process->uart_revmsg = NULL;
	}

	if (uartif->rev_process!= NULL) {
			dm_free((uint8_t*)uartif->rev_process);
			uartif->rev_process = NULL;
	}

}


