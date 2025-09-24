/******************************************************************************
* File Name:   app_common.c
*
* Description: This file contains the implementation of common functions used
*              by the application.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2025, Cypress Semiconductor Corporation (an Infineon company) or
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
#include "app_common.h"

#include "cybsp.h"
#include "cy_pdl.h"

#include "cy_retarget_io.h"

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* For the RetargetIO (Debug UART) usage */
cy_stc_scb_uart_context_t  CYBSP_DEBUG_UART_context;  /** UART context */
mtb_hal_uart_t mtb_ml_retarget_io_uart_obj;  /** UART HAL object */

/*******************************************************************************
* Function Name: app_retarget_io_init
********************************************************************************
* Summary:
* User defined function to initialize the debug UART. 
*
* Parameters:
*  baudrate: baudrate to be applied to the UART
*
* Return:
*  void
*
*******************************************************************************/
void app_retarget_io_init(uint32_t baudrate)
{
    cy_rslt_t result;

    /* Initialize the SCB UART */
    result = (cy_rslt_t)Cy_SCB_UART_Init(CYBSP_DEBUG_UART_HW, 
                                        &CYBSP_DEBUG_UART_config, 
                                        &CYBSP_DEBUG_UART_context);
    
    /* UART init failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_error();
    }

    /* Enable the SCB UART */
    Cy_SCB_UART_Enable(CYBSP_DEBUG_UART_HW);

    result = mtb_hal_uart_setup(&mtb_ml_retarget_io_uart_obj, 
                                &CYBSP_DEBUG_UART_hal_config, 
                                &CYBSP_DEBUG_UART_context, NULL);
    
    /* UART setup failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_error();
    }

    /* Set the UART baudrate */
    result = mtb_hal_uart_set_baud(&mtb_ml_retarget_io_uart_obj, baudrate, NULL);
    
    /* UART setup failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_error();
    }

    /* Initialize retarget-io to use the debug UART port */
    result = cy_retarget_io_init(&mtb_ml_retarget_io_uart_obj);

    /* retarget-io init failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_error();
    }
}


/*******************************************************************************
* Function Name: handle_error
********************************************************************************
* Summary:
* User defined error handling function
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void handle_error(void)
{
    /* Disable all interrupts. */
    __disable_irq();

    CY_ASSERT(0);
}

/* [] END OF FILE */