#ifndef __TIMER_H__
#define __TIMER_H__
#include <stdint.h>
#include "cmsis_os.h"

/*!
 * \brief Timer object description
 */
typedef struct TimerEvent_s
{
    //uint32_t Timestamp;         //! Current timer value
   // uint32_t ReloadValue;       //! Timer delay value
    uint32_t period;
    //bool IsRunning;             //! Is the timer currently running
    void ( *Callback )( void ); //! Timer IRQ callback function
	osTimerId osTimerID; ///<在操作系统中对应的定时器ID
	uint32_t os_timer_cb_internel[6];///<定时器所需要内存
	osTimerDef_t osTimerDef;///<在操作系统中对应的定义数据结构
	uint32_t os_timer_cb_internel_rev[6];///<定时器所需要内存
}TimerEvent_t;

/*!
 * \brief Timer time variable definition
 */
#ifndef TimerTime_t
typedef uint32_t TimerTime_t;
#endif

/*!
 * \brief Initializes the timer object
 *
 * \remark TimerSetValue function must be called before starting the timer.
 *         this function initializes timestamp and reload value at 0.
 *
 * \param [IN] obj          Structure containing the timer object parameters
 * \param [IN] callback     Function callback called at the end of the timeout
 */
void TimerInit( TimerEvent_t *obj, void ( *callback )( void ) );


/*!
 * \brief Starts and adds the timer object to the list of timer events
 *
 * \param [IN] obj Structure containing the timer object parameters
 */
void TimerStart( TimerEvent_t *obj );

/*!
 * \brief Stops and removes the timer object from the list of timer events
 *
 * \param [IN] obj Structure containing the timer object parameters
 */
void TimerStop( TimerEvent_t *obj );

/*!
 * \brief Resets the timer object
 *
 * \param [IN] obj Structure containing the timer object parameters
 */
void TimerReset( TimerEvent_t *obj );

/*!
 * \brief Set timer new timeout value
 *
 * \param [IN] obj   Structure containing the timer object parameters
 * \param [IN] value New timer timeout value
 */
void TimerSetValue( TimerEvent_t *obj, uint32_t value );

void Timer_SystemInit(void);

#endif  // __TIMER_H__
