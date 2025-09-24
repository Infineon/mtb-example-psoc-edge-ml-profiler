/******************************************************************************
* File Name:   elapsed_timer.c
*
* Description: This file contains the implementation of a elapsed timer.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2023-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/
#include <stdio.h>

#include "cybsp.h"
#include "cy_pdl.h"

#include "elapsed_timer.h"

/*******************************************************************************
* Constants
*******************************************************************************/
#define SYSTICK_MAX_CNT (0xFFFFFF)
#define RESET_VAL       (0u)

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* System Tick overflow counter */
static volatile uint64_t elapsed_timer_ov = RESET_VAL;

/*******************************************************************************
* Function Name: elapsed_timer_callback
********************************************************************************
* Summary:
* This is the callback implementation for the elapsed timer. It increments an
* internal counter;
*
* Paramters:
*   void
*
* Return:
*   void
*
*******************************************************************************/
static void elapsed_timer_callback(void)
{
    elapsed_timer_ov++;  
}

/*******************************************************************************
* Function Name: elapsed_timer_init
********************************************************************************
* Summary:
*   Initialize the elapsed system tick timer.
*
* Parameters:
*   void
*
* Return:
*   cy_rslt_t: the status of the initialization.
*
*******************************************************************************/
cy_rslt_t elapsed_timer_init(void)
{
    /* Initialize the System Tick */
    Cy_SysTick_Init(CY_SYSTICK_CLOCK_SOURCE_CLK_CPU, SYSTICK_MAX_CNT);
    Cy_SysTick_SetCallback(0, elapsed_timer_callback);

    elapsed_timer_ov = RESET_VAL;

    return CY_RSLT_SUCCESS;
}

/*******************************************************************************
* Function Name: elapsed_timer_get_tick
********************************************************************************
* Summary:
*   Return the current tick (number of CPU cycles) since the timer was started.
*
* Parameters:
*   tick: current number of ticks.
*
* Return:
*   int: the status of the operation.
*
*******************************************************************************/
int elapsed_timer_get_tick(uint64_t *tick)
{
    *tick = (SYSTICK_MAX_CNT - (uint64_t) Cy_SysTick_GetValue()) + (elapsed_timer_ov * (SYSTICK_MAX_CNT+1));
    
    return CY_RSLT_SUCCESS;
}

