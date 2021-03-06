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
#include "gpio_api.h"
#include "pinmap.h"
#include "error.h"

extern uint32_t Set_GPIO_Clock(uint32_t port_idx);

uint32_t gpio_set(PinName pin) {  
    if (pin == NC) return 0;

    pin_function(pin, STM_PIN_DATA(GPIO_Mode_IN_FLOATING, 0));

    return (uint32_t)(1 << ((uint32_t)pin & 0xF)); // Return the pin mask
}

void gpio_init(gpio_t *obj, PinName pin, PinDirection direction) {
    GPIO_TypeDef *gpio;

    if (pin == NC) return;

    uint32_t port_index = STM_PORT(pin);
  
    // Enable GPIO clock
    uint32_t gpio_add = Set_GPIO_Clock(port_index);
    gpio = (GPIO_TypeDef *)gpio_add;
    
    // Fill GPIO object structure for future use
    obj->pin     = pin;
    obj->mask    = gpio_set(pin);
    obj->reg_in  = &gpio->IDR;
    obj->reg_set = &gpio->BSRR;
    obj->reg_clr = &gpio->BRR;    
  
    // Configure GPIO
    if (direction == PIN_OUTPUT) {
        pin_function(pin, STM_PIN_DATA(GPIO_Mode_Out_PP, 0));
    }
    else { // PIN_INPUT
        pin_function(pin, STM_PIN_DATA(GPIO_Mode_IN_FLOATING, 0));
    }
}

void gpio_mode(gpio_t *obj, PinMode mode) {
    pin_mode(obj->pin, mode);
}

void gpio_dir(gpio_t *obj, PinDirection direction) {
    if (direction == PIN_OUTPUT) {
        pin_function(obj->pin, STM_PIN_DATA(GPIO_Mode_Out_PP, 0));
    }
    else { // PIN_INPUT
        pin_function(obj->pin, STM_PIN_DATA(GPIO_Mode_IN_FLOATING, 0));
    }
}
