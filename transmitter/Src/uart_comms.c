#include "uart_comms.h"

/* This is a single packet buffer for the HAL_UART_TRANSMIT_DMA process. 
  static ensures this is not overwritten until after the packet is transmitted */
  /* Volatile ensures this isn't cached and accessed directly each time it is used */
static volatile uint8_t uartTxBusy = 0;
static RingBufferEntry txScratch;

/* This function dequeues a packet from txBuffer ring buffer, store it in history
    and attempts to transmit using direct memory access    
    
    HAL_UART_Transmit_DMA continuously uses the data buffer to 
    copy data to / from the USART data register until the DMA counter reaches 0
      -Ideal in case of high baud rate to avoid stalling the CPU and in low power mode
      -An interrupt service routine (ISR) can be executed when a certain number of characters have been received/transmitted 
      -Memory to UART is done by the DMA without CPU interaction

    Also can use a ring buffer specifically for the transmit, therefore can consume packets from the RTOS queue
    into the txBuffer ring buffer while waiting to transmit
    http://www.simplyembedded.org/tutorials/interrupt-free-ring-buffer/
    http://www.simplyembedded.org/tutorials/msp430-interrupts/
    https://community.st.com/t5/stm32-mcus/faq-stm32-hal-uart-driver-api-and-callbacks/ta-p/49301
   */
void StartUartTx(void) 
{
  if(uartTxBusy) return;

  if (dequeue(&txBuffer, &txScratch) == 0) // copy into stable memory
  {
    uartTxBusy = 1;
    enqueueRaw(&historyBuffer, &txScratch); //save into history for retransmit
    HAL_UART_Transmit_DMA(&huart2, txScratch.data, txScratch.size);
  }
}

/* The end of the operation is indicated by a callback function: either transmit / receive complete or error.
The callback functions are declared as weak functions in the driver, with no code to execute. This means that 
the user can declare the Tx / Rx callback again in the application and customize it to be informed in real-time 
about the process completion and to execute some application code
https://community.st.com/t5/stm32-mcus/faq-stm32-hal-uart-driver-api-and-callbacks/ta-p/49301 */

/* This is called automatically once a packet has been fully sent out from HAL_UART_Transmit_DMA().
  I need to start the transfer of another packet if the buffer isn't empty
  or signal that it is no longer busy*/

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    if (!RingBuffer_Empty(&txBuffer))
    {
      StartUartTx();
    } 
    else 
    {
      uartTxBusy = 0;
    }
  }
}

uint8_t UartTxBusy(void)
{
    return uartTxBusy;
}

/* printf -> _write -> __io_putchar -> HAL_UART_Transmit*/
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}