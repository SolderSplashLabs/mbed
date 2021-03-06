/* mbed Microcontroller Library
 *******************************************************************************
 * Copyright (c) 2014, STMicroelectronics
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */
#include <stddef.h>
#include "cmsis.h"

#include "gpio_irq_api.h"
#include "pinmap.h"
#include "error.h"

#define EDGE_NONE (0)
#define EDGE_RISE (1)
#define EDGE_FALL (2)
#define EDGE_BOTH (3)

#define CHANNEL_NUM (3)

static uint32_t channel_ids[CHANNEL_NUM]  = {0, 0, 0};
static uint32_t channel_gpio[CHANNEL_NUM] = {0, 0, 0};
static uint32_t channel_pin[CHANNEL_NUM]  = {0, 0, 0};

static gpio_irq_handler irq_handler;

static void handle_interrupt_in(uint32_t irq_index) {
  
    // Retrieve the gpio and pin that generate the irq
    GPIO_TypeDef *gpio = (GPIO_TypeDef *)(channel_gpio[irq_index]);
    uint32_t pin = (uint32_t)(1 << channel_pin[irq_index]);
     
    // Clear interrupt flag
    if (EXTI_GetITStatus(pin) != RESET)
    {
        EXTI_ClearITPendingBit(pin);
    }
    
    if (channel_ids[irq_index] == 0) return;
    
    // Check which edge has generated the irq
    if ((gpio->IDR & pin) == 0) {
        irq_handler(channel_ids[irq_index], IRQ_FALL);
    }
    else  {
        irq_handler(channel_ids[irq_index], IRQ_RISE);
    }
}

// The irq_index is passed to the function
static void gpio_irq0(void) {handle_interrupt_in(0);}
static void gpio_irq1(void) {handle_interrupt_in(1);}
static void gpio_irq2(void) {handle_interrupt_in(2);}

extern uint32_t Set_GPIO_Clock(uint32_t port_idx);

int gpio_irq_init(gpio_irq_t *obj, PinName pin, gpio_irq_handler handler, uint32_t id) {
    IRQn_Type irq_n = (IRQn_Type)0;
    uint32_t vector = 0;
    uint32_t irq_index;

    if (pin == NC) return -1;

    uint32_t port_index = STM_PORT(pin);
    uint32_t pin_index  = STM_PIN(pin);

    // Select irq number and interrupt routine
    switch (pin) {
        case PC_13: // User button
            irq_n = EXTI4_15_IRQn;
            vector = (uint32_t)&gpio_irq0;
            irq_index = 0;
            break;
        case PA_0:
            irq_n = EXTI0_1_IRQn;
            vector = (uint32_t)&gpio_irq1;
            irq_index = 1;
            break;
        case PB_3:
            irq_n = EXTI2_3_IRQn;
            vector = (uint32_t)&gpio_irq2;
            irq_index = 2;
            break;
        default:
            error("This pin is not supported\n");
            return -1;
    }

    // Enable GPIO clock
    uint32_t gpio_add = Set_GPIO_Clock(port_index);

    // Enable SYSCFG clock
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    
    // Connect EXTI line to pin
    SYSCFG_EXTILineConfig(port_index, pin_index);

    // Configure EXTI line
    EXTI_InitTypeDef EXTI_InitStructure;    
    EXTI_InitStructure.EXTI_Line = (uint32_t)(1 << pin_index);
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    
    // Enable and set EXTI interrupt to the lowest priority
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = irq_n;
    NVIC_InitStructure.NVIC_IRQChannelPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
  
    NVIC_SetVector(irq_n, vector);
    NVIC_EnableIRQ(irq_n);

    // Save informations for future use
    obj->irq_n = irq_n;
    obj->irq_index = irq_index;
    obj->event = EDGE_NONE;
    channel_ids[irq_index] = id;
    channel_gpio[irq_index] = gpio_add;
    channel_pin[irq_index] = pin_index;
    
    irq_handler = handler; 
  
    return 0;
}

void gpio_irq_free(gpio_irq_t *obj) {
    channel_ids[obj->irq_index] = 0;
    channel_gpio[obj->irq_index] = 0;
    channel_pin[obj->irq_index] = 0;
    // Disable EXTI line
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_StructInit(&EXTI_InitStructure);
    EXTI_Init(&EXTI_InitStructure);  
    obj->event = EDGE_NONE;
}

void gpio_irq_set(gpio_irq_t *obj, gpio_irq_event event, uint32_t enable) {
    EXTI_InitTypeDef EXTI_InitStructure;
    
    uint32_t pin_index = channel_pin[obj->irq_index];

    EXTI_InitStructure.EXTI_Line = (uint32_t)(1 << pin_index);
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    
    if (event == IRQ_RISE) {
        if ((obj->event == EDGE_FALL) || (obj->event == EDGE_BOTH)) {
            EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
            obj->event = EDGE_BOTH;
        }
        else { // NONE or RISE
            EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
            obj->event = EDGE_RISE;
        }
    }
    
    if (event == IRQ_FALL) {
        if ((obj->event == EDGE_RISE) || (obj->event == EDGE_BOTH)) {
            EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
            obj->event = EDGE_BOTH;
        }
        else { // NONE or FALL
            EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
            obj->event = EDGE_FALL;
        }
    }
    
    if (enable) {
        EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    }
    else {
        EXTI_InitStructure.EXTI_LineCmd = DISABLE;
    }
    
    EXTI_Init(&EXTI_InitStructure);
}

void gpio_irq_enable(gpio_irq_t *obj) {
    NVIC_EnableIRQ(obj->irq_n);
}

void gpio_irq_disable(gpio_irq_t *obj) {
    NVIC_DisableIRQ(obj->irq_n);
    obj->event = EDGE_NONE;
}
