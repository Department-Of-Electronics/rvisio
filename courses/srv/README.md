# Course: Real Time Operating Systems (SRV)

Examples of the course *Real Time Operating Systems* (*Sistemi u realnom vremenu*, School of Electrical
Engineering, University of Belgrade), ported from the ETF5529 board (MSP430F5529, Code Composer Studio) to
the RVisio system on chip. In the simulator every task, semaphore, queue and interrupt can be followed cycle
by cycle: the stack analyzer shows which task runs, the "Sensors and actuators" panel shows the LEDs, the
display, the buttons and the potentiometer of the board.

> **Run them in the browser:** [rvisio.com](https://rvisio.com) → Open simulator → **System on chip (SoC)** →
> **📦 FreeRTOS demos** → *🎓 Course: Real Time Operating Systems*. No toolchain needed.

| Example | Task | What it shows | FreeRTOS API |
|---|---|---|---|
| [`SRV_2_1`](SRV_2_1) | 3 | Two tasks, LEDs and a polled button | xTaskCreate, vTaskDelay |
| [`SRV_2_2`](SRV_2_2) | 6 | 7 segment display: multiplexing | vTaskDelay, pdMS_TO_TICKS |
| [`SRV_2_3`](SRV_2_3) | 7 | Binary semaphore: button → LED | xSemaphoreCreateBinary, xSemaphoreGive, xSemaphoreTake |
| [`SRV_2_4`](SRV_2_4) | 8 | Counting with two semaphores | xSemaphoreCreateBinary, xSemaphoreGive, xSemaphoreTake |
| [`SRV_2_5`](SRV_2_5) | 9 | Button interrupt and a semaphore | xSemaphoreGiveFromISR, vPortSetInterruptHandler |
| [`SRV_2_6`](SRV_2_6) | 11 | A mutex for a shared variable | xSemaphoreCreateMutex, xSemaphoreTake, xSemaphoreGive |
| [`SRV_2_7_v1`](SRV_2_7_v1) | 12 | UART without protection | xSemaphoreGiveFromISR, vTaskDelay |
| [`SRV_2_7_v2`](SRV_2_7_v2) | 12 | UART with a mutex | xSemaphoreCreateMutex |
| [`SRV_2_9`](SRV_2_9) | 16 | UART interrupt, queue and a command | xQueueCreate, xQueueSendToBackFromISR, xQueueReceive, mutex |
| [`SRV_2_10`](SRV_2_10) | 17 | A command queue from two sources | xQueueSendToBack, xQueueReceive |
| [`SRV_2_11`](SRV_2_11) | 18 | ADC, mailbox and display | xQueueCreate(1, ...), xQueueSendToBackFromISR, xQueueReceive with timeout 0 |
| [`SRV_2_12`](SRV_2_12) | 13 | Software timer | xTimerCreate, xTimerStart, xTimerStop |
| [`SRV_2_13`](SRV_2_13) | 14 | Changing the timer period | xTimerChangePeriod |
| [`SRV_2_14`](SRV_2_14) | 21 | Event group | xEventGroupCreate, xEventGroupSetBits, xEventGroupWaitBits |
| [`SRV_2_15`](SRV_2_15) | 22 | Two tasks wait for the same event | xEventGroupWaitBits (several tasks) |
| [`SRV_2_16`](SRV_2_16) | 23 | Task notifications | xTaskNotify, xTaskNotifyWait, ulTaskNotifyTake, vTaskNotifyGiveFromISR |

(There is no example 2.8 in the original set.)

## The board in the simulator

| ETF5529 board (MSP430F5529) | RVisio SoC | In the simulator |
|---|---|---|
| LEDs LD3, LD4 (P2.4, P2.5) | `gpio0` pins 3, 4 | **led3**, **led4** |
| Buttons S2, S3, S4 (P1.1, P1.4, P1.5, pull-up) | `gpio0` pins 2, 3, 4 | **sw2**, **sw3**, **sw4** (hold with the mouse) |
| 2 digit 7 segment display | `gpio1`: segments a..g on pins 0..6, digits on pins 8, 9 | **display0** |
| ADC12, inputs A0, A1 | `adc0`, inputs 0, 1 | **potentiometer**, **light sensor** (sliders) |
| UART USCI_A1, 9600 baud | `uart0` | **Console** panel |
| Port / ADC / UART interrupt vectors | INTC interrupts 3, 7, 0 | `vPortSetInterruptHandler()` |

The board HAL in `hal/` keeps the names of `ETF5529_HAL` (`LED3`, `halTOGGLE_LED`, `vHAL7SEGWriteDigit`,
`HAL_7SEG_DISPLAY_1_ON`, ...), so the tasks are the same as on the MSP430. What is different:

* **Buttons.** On the board the buttons have pull-up resistors (the pin reads 0 while pressed) and the port
  interrupt comes on the press edge. `halREAD_BUTTON()` and `halBUTTON_IFG()` give the same values on the RVisio
  SoC, so the debouncing code is unchanged.
* **Interrupts.** Instead of `__attribute__((interrupt(PORT1_VECTOR)))` an interrupt routine is a normal
  function `void vPORT1ISR( BaseType_t *pxHigherPriorityTaskWoken )`, registered in `prvSetupHardware()` with
  `vPortSetInterruptHandler( GPIO_IRQ, vPORT1ISR )`. It passes `pxHigherPriorityTaskWoken` to the `...FromISR`
  functions; the port switches to the woken task when the interrupt ends (this replaces `portYIELD_FROM_ISR`).
* **Clock, watchdog, low power.** The watchdog, `hal430SetSystemClock()` and the low power idle hook are MSP430
  specific and are not needed. The tick (1 ms = 10 000 cycles) comes from TIMER0 and is set up by the port.
* **Timing.** The busy wait loop used for debouncing (`for(i = 0; i < 1000; i++);`) runs a few ms on the
  simulated core; `i` is `volatile` so the compiler keeps the loop. A button held in the panel stays pressed at
  least 50 ms of simulated time.

## Build

Every example is a project of its own (`make` in its folder; it contains its own copy of the kernel, the port
and the HAL), with the same toolchain and VS Code setup as the other projects of this repository:

```bash
cd courses/srv/SRV_2_11
make
```
