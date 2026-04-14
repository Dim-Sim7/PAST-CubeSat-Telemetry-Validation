#include "uart_comms.h"
#include "tmr.h"

/* This is a single packet buffer for the HAL_UART_TRANSMIT_DMA process. 
  static ensures this is not overwritten until after the packet is transmitted */
static RingBufferEntry txScratch;

/* Volatile ensures this isn't cached and accessed directly each time it is used */
TMR_SECTION_A static volatile BaseType_t uartTxBusyA = pdFALSE;
TMR_SECTION_B static volatile BaseType_t uartTxBusyB = pdFALSE;
TMR_SECTION_C static volatile BaseType_t uartTxBusyC = pdFALSE;

static RingBufferEntry txEntries[TX_BUFFER_SIZE];
TMR_SECTION_A static RingBuffer txBuffer = {
    .entries = txEntries,
    .head_a  = 0, .head_b = 0, .head_c = 0,
    .tail_a  = 0, .tail_b = 0, .tail_c = 0,
    .size    = TX_BUFFER_SIZE,
    .mask    = TX_BUFFER_SIZE - 1
};

static inline BaseType_t UartTxBusy(void)
{
    return TMR_Vote_BaseType(uartTxBusyA, uartTxBusyB, uartTxBusyC);
}

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
/* Returns pdTRUE if DMA was successfully started */
static BaseType_t StartUartTx(void)
{
    dequeue(&txBuffer, &txScratch);
    /* Transmit via direct memory access method - non-blocking. Completion signalled via HAL_UART_TxCpltCallback() */
    if (HAL_UART_Transmit_DMA(&huart2, txScratch.data, txScratch.size) == HAL_OK)
    {
      /* save sent packet into history for retransmit. Locally lost packets are discarded at the moment */
      addHistoryRaw(&txScratch); 
      return pdTRUE;
    }
    else
    {
      return pdFALSE;
    }
}

/* Called by Transmit_Packets_Consumer() task. enqueues a packet to the txBuffer
  and calls StartUartTx to transmit the packet. Includes critical sections to prevent
  race conditions*/
BaseType_t UartTx_Enqueue(TelemetryPacket_t *packet)
{
  /* This Critical section masks interrupts to prevent TxCpltCallback() from firing
      between the enqueue and the UartTxBusy() check. Without this, the ISR could dequeue
      the entry we just enqueued and set uartTxBusy = pdFALSE before we read it, causing us
      to call StartUartTx() concurrently with the ISR*/
  taskENTER_CRITICAL();

  enqueue(&txBuffer, packet);

  if (!UartTxBusy())
  {
    /* Mark busy before exiting the critical section so that if a context switch
   occurs before StartUartTx() is called, any other task calling UartTx_Enqueue()
   will see uartTxBusy = pdTRUE and not attempt to also call StartUartTx() */
    SET_BUSY_TRUE(uartTxBusyA, uartTxBusyA, uartTxBusyA);

    /* Exit critical section before calling StartUartTx() as the transmit can be slow
    uarTxBusy = pdTRUE ensure the ISR will not interfere if it fires during the DMA setup*/
    taskEXIT_CRITICAL();
    if (StartUartTx() != pdTRUE) /* Attempt to send packet */
    {
      /* DMA failed - reset busy flag so the pipeline is not pemanently stalled.
      Re enter critical section to write uartTxBusym as the ISR could fire at any point*/
      taskENTER_CRITICAL();
      SET_BUSY_FALSE(uartTxBusyA, uartTxBusyA, uartTxBusyA);
      taskEXIT_CRITICAL();
    }
  }
  else
  {
    /* UART is already busy transmitting. The packet has been enqueued into txBuffer
    and will be picked up automatically by HAL_UART_TxCplyCallback() when the current transfer ends*/
    taskEXIT_CRITICAL();
  }
  return pdTRUE;
}

/* The end of the operation is indicated by a callback function: either transmit / receive complete or error.
The callback functions are declared as weak functions in the driver, with no code to execute. This means that 
the user can declare the Tx / Rx callback again in the application and customize it to be informed in real-time 
about the process completion and to execute some application code
https://community.st.com/t5/stm32-mcus/faq-stm32-hal-uart-driver-api-and-callbacks/ta-p/49301 */

/* Called automatically by the HAL once HAL_UART_Transmit_DMA() completes.
   Runs in ISR context and interrupts at this priority are already masked,
   so no additional critical section or FreeRTOS API calls are needed.
   Responsible for keeping the TX pipeline running until txBuffer is drained. */

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    if (!RingBuffer_Empty(&txBuffer))
    {
      /* More packets waiting -- immediately kick off next DMA transfer.
        txScratch is safe to overwrite here because the previous DMA transfer
        has fully completed before this callback fires*/
      if (StartUartTx() != pdTRUE)
      {
        /* DMA failed -- release busy flagh so the consumer task can attempt
        to restart the pipeline on the next packet arrival*/
        SET_BUSY_FALSE(uartTxBusyA, uartTxBusyA, uartTxBusyA);
      }
    } 
    else 
    {
      /* Buffer is empty / has been drained. Mark UART as idle. The consumer
        task will set uartTxBusy = pdTRUE and call StartUartTx() when the next packet arrives. */
      SET_BUSY_FALSE(uartTxBusyA, uartTxBusyA, uartTxBusyA);
    }
  }
}

/* Holds the received nack packet */
static NACKPacket_t rxNack;

/* Receiver */
void StartUartRx(void)
{
  /* Start receiving exactly sizeof(NACKPacket_t) into rxNack*/
  HAL_UART_Receive_DMA(&huart2, (uint8_t*)&rxNack, sizeof(NACKPacket_t));
}

/* https://forums.freertos.org/t/portyield-from-isr-question/7162 resource on portYIELD_FROM_ISR*/
/* Executes when a nack packet is received fully from StartUartRx */
/* rxNack -> validate -> find entry in history -> store in queue -> retransmit task consumes */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef * huart)
{
  if (huart->Instance == USART2)
  {
    /* Copy data from rxNack */
    uint32_t seq = rxNack.seq;
    uint16_t nackCrc = rxNack.crc;
    uint8_t valid = validateCrc(&nackCrc, (uint8_t*)&rxNack, offsetof(NACKPacket_t, crc));
    /* Restart Rx */
    StartUartRx();

    if (valid)
    {
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      xQueueSendFromISR(Retransmit_Queue, &seq, &xHigherPriorityTaskWoken);
      /* Preps the scheduler to resume running the high priority task once the ISR finishes*/
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
  }
}

/* https://www.embedded-communication.com/en/misc/printf-with-st-link/ */
#ifdef DEBUG
int __io_putchar(int ch)
{
    ITM_SendChar(ch);
    return ch;
}
#endif