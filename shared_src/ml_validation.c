/******************************************************************************
* File Name:   ml_validation.c
*
* Description: This file contains the implementation of the validation of the
*              machine learning model.
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
#include "ml_validation.h"

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#ifndef USE_STREAM_DATA
/* Include regression files */
#include MTB_ML_INCLUDE_MODEL_X_DATA_FILE(MODEL_NAME)
#include MTB_ML_INCLUDE_MODEL_Y_DATA_FILE(MODEL_NAME)
#endif

/*******************************************************************************
* Constants
*******************************************************************************/
#define SUCCESS_RATE       (98.0f)

/* Timeout value for streaming */
#define DEFAULT_TIMEOUT_MS (5000u)

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* NN Model Object */
static mtb_ml_model_t *model_obj;

/* Output/result buffers for the inference engine */
static MTB_ML_DATA_T *result_buffer;

/* Model Output Size */
static int model_output_size;

/*******************************************************************************
* Function Name: ml_validation_init
********************************************************************************
* Summary:
*   Initialize the Neural Network based on the given model and setup to start
*   regression of the model and profiling configuration.
*
* Parameters:
*   profile_cfg: profiling configuration
*   model_bin: pointer to the model data
*
* Return:
*   cy_rslt_t: the status of the initialization.
*******************************************************************************/
cy_rslt_t ml_validation_init(mtb_ml_profile_config_t profile_cfg,
                             mtb_ml_model_bin_t *model_bin)
{
    cy_rslt_t result;

    /* Initialize the neural network */
    result = mtb_ml_model_init(model_bin,
                               NULL,
                               &model_obj);
    if (CY_RSLT_SUCCESS != result)
    {
        printf("MTB ML initialization failure: %lu\r\n", (unsigned long) result);
        return result;
    }

    mtb_ml_model_profile_config(model_obj, profile_cfg);

    mtb_ml_model_get_output(model_obj, &result_buffer, &model_output_size);

    /* Print information about the model */
    mtb_ml_utils_print_model_info(model_obj);

    return CY_RSLT_SUCCESS;
}

#ifndef USE_STREAM_DATA
/*******************************************************************************
* Function Name: ml_validation_local_task
********************************************************************************
* Summary:
*   Run the Neural Network Inference Engine based on the local data.
*
* Parameters:
*   void
*
* Return:
*   cy_rslt_t: the status of the task execution.
*******************************************************************************/
cy_rslt_t ml_validation_local_task(void)
{
    /* Regression pointers */
    MTB_ML_DATA_T  *input_reference;
    MTB_ML_DATA_T  *output_reference;
       
    uint32_t     num_loop;
    uint32_t     correct_result = 0;
    bool         test_result;
    uint32_t     total_count = 0;
    cy_rslt_t    result;
    int          file_input_size;
    int          model_input_size = mtb_ml_model_get_input_size(model_obj);

    /* Parse input data information: 
     * - Data type (TFLM only)
     * - Number of samples
     * - Frame size
     */
    mtb_ml_x_file_header_t *x_file_header = (mtb_ml_x_file_header_t *) MTB_ML_MODEL_X_DATA_BIN(MODEL_NAME);

    /* Point to regression data */
    input_reference  = (MTB_ML_DATA_T *) (((uint32_t) x_file_header) + sizeof(*x_file_header));
    output_reference = (MTB_ML_DATA_T *) MTB_ML_MODEL_Y_DATA_BIN(MODEL_NAME);

    /* Get the number of loops for this regression */
    num_loop = x_file_header->num_of_samples;

    /* Get the number of inputs of the NN */
    file_input_size = x_file_header->input_size;

#if defined(RNN_STREAMING)

    model_obj->recurrent_ts_size = x_file_header->recurrent_ts_size;

    /* Check if it is a non-RNN model */
    if (model_obj->recurrent_ts_size <= 0)
    {
        printf("This is not a RNN model (%d). Set the NN_RNN_MODEL variable to NO in the Makefile, aborting...\r\n", 
            model_obj->recurrent_ts_size);
        return MTB_ML_RESULT_MISMATCH_DATA_TYPE;
    }

    /* If using RRN Model, check if the data time steps matches */
    if ((file_input_size/model_obj->recurrent_ts_size) != model_input_size)
    {
        printf("Data size error, file input size=%d, model input size=%d recurrent time series size=%d, aborting...\r\n", 
            file_input_size, model_input_size, model_obj->recurrent_ts_size);
        return MTB_ML_RESULT_MISMATCH_DATA_TYPE;
    }

    /* Allocate memory for the RNN input slice */
    MTB_ML_DATA_T * input_slice = (MTB_ML_DATA_T *) malloc (model_input_size * sizeof(MTB_ML_DATA_T));

    if (input_slice == NULL)
    {
        printf("ERROR: Allocating memory for input slice\r\n");
        return MTB_ML_RESULT_ALLOC_ERR;
    }

#else
    /* Check if the file input size matches the model input size */
    if (file_input_size != model_input_size)
    {
        printf("Input buffer size error, file input size=%d, model input size=%d, aborting...\r\n", 
                file_input_size, model_input_size);
        return MTB_ML_RESULT_MISMATCH_DATA_TYPE;
    }
#endif /* RNN_STREAMING */

    /* The following loop runs for number of examples used in regression */
    for (int j = 0; j < num_loop; j++)
    {

#if defined(RNN_STREAMING)
        result = mtb_ml_model_rnn_reset_all_parameters(model_obj);
        if (MTB_ML_RESULT_SUCCESS != result)
        {
            printf("ERROR: failed to reset model parameters\r\n");
            free(input_slice);
            return MTB_ML_RESULT_INFERENCE_ERROR;
        }

        for (int i = 0; i < model_obj->recurrent_ts_size; i++)
        {
            /* Input data is 2D array squashed to 1D array by Coretools */
            for (int z = 0; z < model_input_size; z++)
            {
                input_slice[z] = input_reference[i*model_input_size+z];
            }

            result = mtb_ml_model_run(model_obj, input_slice);

            /* Check if the inferencing return any error */
            if (MTB_ML_RESULT_SUCCESS != result)
            {
                free(input_slice);
                return result;
            }
        }

#else
        result = mtb_ml_model_run(model_obj, input_reference);

        /* Check if the inferencing return any error */
        if (MTB_ML_RESULT_SUCCESS != result)
        {
            return result;
        }
#endif /* RNN_STREAMING */

        /* Check if the results are accurate enough */
        if (mtb_ml_utils_find_max(result_buffer, model_output_size) ==
            mtb_ml_utils_find_max(output_reference, model_output_size))
        {
            correct_result++;
        }

        /* Increment buffers */
        input_reference  += file_input_size;
        output_reference += model_output_size;

        total_count++;
    }

#if defined(RNN_STREAMING)
    free(input_slice);
#endif /* RNN_STREAMING */

    /* Print PASS or FAIL with Accuracy percentage 
     * Only for regression ... 
     */
    {
        float success_rate;

        /* Check if total count is equal to ZERO */
        if (total_count == 0)
        {
            success_rate = 0;
        }
        else
        {
            success_rate = ((float) correct_result) * 100.0f / ((float) total_count);
        }
        
        test_result = (success_rate >= SUCCESS_RATE);

        mtb_ml_model_profile_log(model_obj);
        
        printf("\r\n***************************************************\r\n");
        if (test_result == true)
        {
            printf("PASS with accuracy percentage =%3.2f, total_cnt=%d", success_rate, (int) total_count);
        }
        else
        {
            printf("FAIL with accuracy percentage =%3.2f, total_cnt=%d", success_rate, (int) total_count);
        }
        printf("\r\n***************************************************\r\n");
    }

    return CY_RSLT_SUCCESS;
}
#endif /* USE_STREAM_DATA */

/*******************************************************************************
* Function Name: ml_validation_stream_task
********************************************************************************
* Summary:
*   Run the Neural Network Inference Engine based on the stream data.
*
* Parameters:
*   iface: pointer to the streaming interface
*
* Return:
*   cy_rslt_t: the status of the task execution.
*******************************************************************************/
cy_rslt_t ml_validation_stream_task(mtb_ml_stream_interface_t *iface)
{
    cy_rslt_t result = MTB_ML_RESULT_SUCCESS;

    /* Initialize the streaming interface */
    result = mtb_ml_stream_init(iface, model_obj);
    if (CY_RSLT_SUCCESS != result)
    {
        printf("MTB ML streaming init failure: %lu\r\n", (unsigned long) result);
        return result;
    }

    /* Alloc RX buf */
    MTB_ML_DATA_T *rx_buf = (MTB_ML_DATA_T *) malloc(iface->input_size * sizeof(MTB_ML_DATA_T));
    if(!rx_buf)
    {
        printf("ERROR: Allocating memory for rx_buf\r\n");
        return MTB_ML_RESULT_ALLOC_ERR;
    }

    /* Set slice length and initialize buffer */
#if defined(RNN_STREAMING)
    model_obj->recurrent_ts_size = iface->x_data_info.recurrent_ts_size;

    /* Allocate memory for the RNN input slice */
    MTB_ML_DATA_T * input_slice = (MTB_ML_DATA_T *) malloc (model_obj->input_size * sizeof(MTB_ML_DATA_T));

    if (input_slice == NULL)
    {
        printf("ERROR: Allocating memory for input slice\r\n");
        free(rx_buf);
        return MTB_ML_RESULT_ALLOC_ERR;
    }
#endif /* RNN_STREAMING */

    /* Do frame-by-frame (sample == frame) inference */
    for (int i = 0; i < iface->x_data_info.num_of_samples; i++)
    {

#if defined(RNN_STREAMING)
        result = mtb_ml_model_rnn_reset_all_parameters(model_obj);
        if (MTB_ML_RESULT_SUCCESS != result)
        {
            printf("ERROR: failed to reset model parameters\r\n");
            free(input_slice);
            free(rx_buf);
            return MTB_ML_RESULT_INFERENCE_ERROR;
        }
#endif /* RNN_STREAMING */

        /* Get input data */
        result = mtb_ml_stream_input_data(iface, rx_buf, DEFAULT_TIMEOUT_MS);
        if(MTB_ML_RESULT_SUCCESS != result)
        {
            printf("ERROR: Failed to receive input data from host.\r\n");
            break;
        }

#if defined(RNN_STREAMING)
        for (int j = 0; j < model_obj->recurrent_ts_size; j++)
        {
            /* Input data is 2D array squashed to 1D array by Coretools */
            for (int z = 0; z < model_obj->input_size; z++ )
            {
                input_slice[z] = ((MTB_ML_DATA_T *)rx_buf)[j * model_obj->input_size + z];
            }

            result = mtb_ml_model_run(model_obj, (MTB_ML_DATA_T *) input_slice);

            if (result != MTB_ML_RESULT_SUCCESS)
            {
                free(rx_buf);
                free(input_slice);
                return result;
            }
        }
#else
        /* Run the model */
        result = mtb_ml_model_run(model_obj, (MTB_ML_DATA_T *) rx_buf);
        if (MTB_ML_RESULT_SUCCESS != result)
        {
            free(rx_buf);
            return result;
        }

#endif /* RNN_STREAMING */

        /* Send output data */
        result = mtb_ml_stream_output_data(iface, model_obj->output, DEFAULT_TIMEOUT_MS);
        if(MTB_ML_RESULT_SUCCESS != result)
        {
            printf("ERROR: Failed to send output data to host\r\n");
            free(rx_buf);
            return MTB_ML_RESULT_ALLOC_ERR;
        }
    }

    /* Free allocated memory */
    free(rx_buf);
#if defined(RNN_STREAMING)
    free(input_slice);
#endif


    /* Generate profiling log if it is enabled */
    result = mtb_ml_model_profile_log(model_obj);
    if(MTB_ML_RESULT_SUCCESS != result)
    {
        printf("ERROR: Failed to generate profile log.\r\n");
        return MTB_ML_RESULT_BAD_MODEL;
    }

    return mtb_ml_inform_host_done(iface, DEFAULT_TIMEOUT_MS);
}

/* [] END OF FILE */