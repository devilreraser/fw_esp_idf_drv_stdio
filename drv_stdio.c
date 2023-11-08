/* *****************************************************************************
 * File:   drv_stdio.c
 * Author: Dimitar Lilov
 *
 * Created on 2022 06 18
 * 
 * Description: ...
 * 
 **************************************************************************** */

/* *****************************************************************************
 * Header Includes
 **************************************************************************** */
#include "drv_stdio.h"

#include <stdio.h>
#include <stdbool.h>
#include <sdkconfig.h>
#include "esp_vfs_dev.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"

#include "driver/uart.h"

#if CONFIG_APP_SOCKET_STDIO_USE /* consider how to implement splitted */
#include "app_socket_stdio.h"
#endif

#if CONFIG_DRV_CONSOLE_USE
#include "drv_console.h"
#endif

#if CONFIG_DRV_STREAM_USE
#include "drv_stream.h"
#endif


/* *****************************************************************************
 * Configuration Definitions
 **************************************************************************** */
#define TAG "drv_stdio"

/* *****************************************************************************
 * Constants and Macros Definitions
 **************************************************************************** */

/* *****************************************************************************
 * Enumeration Definitions
 **************************************************************************** */

/* *****************************************************************************
 * Type Definitions
 **************************************************************************** */

/* *****************************************************************************
 * Function-Like Macros
 **************************************************************************** */

/* *****************************************************************************
 * Variables Definitions
 **************************************************************************** */
static SemaphoreHandle_t flag_write_stdout = NULL;

bool stdio_bUseUart0 = false;


char uart0_tx_buffer[1024];

#if CONFIG_DRV_STREAM_USE
StreamBufferHandle_t redirect_stdio_stream_buffer_recv = NULL;
#endif

bool bSkipWriteSocket = false;

/* *****************************************************************************
 * Prototype of functions definitions
 **************************************************************************** */

/* *****************************************************************************
 * Functions
 **************************************************************************** */
static int std_write(void* cookie, const char* pData, int size)
{
    bool bSkippedWrite = false;
    bool bRenewedWrite = false;
    //if (drv_console_get_log_disabled()) return size;
    xSemaphoreTake(flag_write_stdout, portMAX_DELAY);
    
    //the real write
    int sizeSock = size;
    #if CONFIG_APP_SOCKET_STDIO_USE
    sizeSock = app_socket_stdio_send(pData, size);
    #endif
    
    if ((sizeSock != size) && (size > 0))
    {
        if (bSkipWriteSocket == false)
        {
            bSkippedWrite = true;
            bSkipWriteSocket = true;
        }
    }
    else
    {
        if (bSkipWriteSocket)
        {
            bRenewedWrite = true;
            bSkipWriteSocket = false;
        }
    }
    //size = sizeSock;

    if (stdio_bUseUart0)
    {
        if (uart_is_driver_installed(UART_NUM_0))
        {
            if (bSkippedWrite)
            {
                uart_write_bytes(UART_NUM_0, "\r\n Skipped Write Socket\r\n", sizeof("\r\n Skipped Write Socket\r\n"));
            }
            if (bRenewedWrite)
            {
                uart_write_bytes(UART_NUM_0, "\r\n Renewed Write Socket\r\n", sizeof("\r\n Renewed Write Socket\r\n"));
            }


            #if 1
            int sizeUart = size;
            
            if(sizeUart > sizeof(uart0_tx_buffer))
            {
                sizeUart = sizeof(uart0_tx_buffer);
            }
            char *p_uart0_tx_buffer = uart0_tx_buffer;
            int sizeUartSend = 0;
            // \n -> \r\n
            for (int i =0; i < sizeUart; i++)
            {
                if (pData[i] == '\n')
                {
                    char cr = '\r';
                    if (sizeUartSend < sizeof(uart0_tx_buffer))
                    {
                        p_uart0_tx_buffer[sizeUartSend++] = cr;
                    }

                }

                if (sizeUartSend < sizeof(uart0_tx_buffer))
                {
                    p_uart0_tx_buffer[sizeUartSend++] = pData[i];
                }            
            }
            uart_write_bytes(UART_NUM_0, p_uart0_tx_buffer, sizeUartSend);

            if (sizeSock == sizeUart)
            {
                size = sizeSock;
            }

            #else
            uart_write_bytes(UART_NUM_0, pData, size);        
            #endif
        }
    }

    xSemaphoreGive(flag_write_stdout);
    return size;
}

static int std_read(void* cookie, char* pData, int size)
{
    int nResult = 0;

    //the real read
    do
    {
        if (stdio_bUseUart0)
        {
            if (uart_is_driver_installed(UART_NUM_0))
            {

                nResult = uart_read_bytes(UART_NUM_0, pData, size, 0);
                if (nResult > 0) 
                {
                    #if CONFIG_DRV_CONSOLE_USE
                    #if CONFIG_DRV_CONSOLE_CUSTOM
                    #if CONFIG_DRV_CONSOLE_CUSTOM_LOG_DISABLE_FIX
                    if (drv_console_set_log_disabled_check_skipped(pData, nResult))
                    {

                    }
                    #endif
                    #endif
                    #endif
                    drv_stdio_redirect_stream_push((uint8_t*)pData, nResult);
                }
            }
            
        }
        #if CONFIG_APP_SOCKET_STDIO_USE
        if (nResult <= 0)
        {
            nResult = app_socket_stdio_recv(pData, size);
        }
        #endif
        if (nResult <= 0)
        {
            vTaskDelay(pdMS_TO_TICKS(40));
        }

    } while (nResult <= 0);

    /* Fix CR to LF */
    for(int i = 0; i < nResult; i++)
    {
        if (pData[i] == '\r')
        {
            pData[i]='\n';
        } 
    }


    //if (nResult <= 0)nResult = -2;


    return nResult;    
}

void drv_stdio_init_stdin(FILE* p_stdin)
{
    p_stdin = fropen(NULL, &std_read);
    setvbuf(p_stdin, NULL, _IONBF, 0);   
}

void drv_stdio_init_stdout(FILE* p_stdout)
{
    p_stdout = fwopen(NULL, &std_write);
    setvbuf(p_stdout, NULL, _IONBF, 0);   
}



void drv_stdio_redirect_local(void)
{
    if (flag_write_stdout == NULL)
    {
        flag_write_stdout = xSemaphoreCreateBinary();
        xSemaphoreGive(flag_write_stdout);
    }

    ESP_LOGW(TAG, "stdin : @0x%08X fileno: %d", (int)stdin , fileno(stdin ));
    ESP_LOGW(TAG, "stdout: @0x%08X fileno: %d", (int)stdout, fileno(stdout));
    ESP_LOGW(TAG, "stderr: @0x%08X fileno: %d", (int)stderr, fileno(stderr));

    fflush(stdin);
    fflush(stdout);
    fflush(stderr);


    fsync(fileno(stdin));
    fsync(fileno(stdout));
    fsync(fileno(stderr));
    //drv_stdio_init_stdout(stderr);
    //drv_stdio_init_stdout(stdout);
    //drv_stdio_init_stdin(stdin);

    stdin = fropen(NULL, &std_read);
    setvbuf(stdin, NULL, _IONBF, 0);   
    stdout = fwopen(NULL, &std_write);
    setvbuf(stdout, NULL, _IONBF, 0);   
    stderr = fwopen(NULL, &std_write);
    setvbuf(stderr, NULL, _IONBF, 0);   

}


void drv_stdio_redirect_global(void)
{
    
    //fflush(stdout);
    //fsync(fileno(stdout));

    if (flag_write_stdout == NULL)
    {
        flag_write_stdout = xSemaphoreCreateBinary();
        xSemaphoreGive(flag_write_stdout);
    }

    ESP_LOGW(TAG, "_GLOBAL_REENT->_stdin : @0x%08X fileno: %d", (int)_GLOBAL_REENT->_stdin , fileno(_GLOBAL_REENT->_stdin ));
    ESP_LOGW(TAG, "_GLOBAL_REENT->_stdout: @0x%08X fileno: %d", (int)_GLOBAL_REENT->_stdout, fileno(_GLOBAL_REENT->_stdout));
    ESP_LOGW(TAG, "_GLOBAL_REENT->_stderr: @0x%08X fileno: %d", (int)_GLOBAL_REENT->_stderr, fileno(_GLOBAL_REENT->_stderr));

    fflush(_GLOBAL_REENT->_stdin);
    fflush(_GLOBAL_REENT->_stdout);
    fflush(_GLOBAL_REENT->_stderr);


    fsync(fileno(_GLOBAL_REENT->_stdin));
    fsync(fileno(_GLOBAL_REENT->_stdout));
    fsync(fileno(_GLOBAL_REENT->_stderr));

    _GLOBAL_REENT->_stdin = fropen(NULL, &std_read);
    setvbuf(_GLOBAL_REENT->_stdin, NULL, _IONBF, 0);   
    _GLOBAL_REENT->_stdout = fwopen(NULL, &std_write);
    setvbuf(_GLOBAL_REENT->_stdout, NULL, _IONBF, 0);   
    _GLOBAL_REENT->_stderr = fwopen(NULL, &std_write);
    setvbuf(_GLOBAL_REENT->_stderr, NULL, _IONBF, 0);   
    //drv_stdio_init_stdout(_GLOBAL_REENT->_stderr);
    //drv_stdio_init_stdout(_GLOBAL_REENT->_stdout);
    //drv_stdio_init_stdin(_GLOBAL_REENT->_stdin);
    //fsync(fileno(stdout));
    //fsync(fileno(stdin));
}

size_t drv_stdio_redirect_stream_push(uint8_t *pData, size_t nSize)
{
    #if CONFIG_DRV_STREAM_USE
    return drv_stream_push(&redirect_stdio_stream_buffer_recv, pData, nSize);
    #else
    return 0;
    #endif
}

size_t drv_stdio_redirect_stream_pull(uint8_t *pData, size_t nSize)
{
    #if CONFIG_DRV_STREAM_USE
    return drv_stream_pull(&redirect_stdio_stream_buffer_recv, pData, nSize);
    #else
    return 0;
    #endif
}


void drv_stdio_redirect_uart0(bool bUseUart0)
{
    stdio_bUseUart0 = bUseUart0;
    
    #if CONFIG_DRV_STREAM_USE
    drv_stream_init(&redirect_stdio_stream_buffer_recv, 0, "redir_stdio_sock_recv");
    #endif
}

bool drv_stdio_is_redirect_uart0(void)
{
    return stdio_bUseUart0;
}