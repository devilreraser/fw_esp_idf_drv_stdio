/* *****************************************************************************
 * File:   drv_stdio.h
 * Author: Dimitar Lilov
 *
 * Created on 2022 06 18
 * 
 * Description: ...
 * 
 **************************************************************************** */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/* *****************************************************************************
 * Header Includes
 **************************************************************************** */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* *****************************************************************************
 * Configuration Definitions
 **************************************************************************** */

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
 * Function-Like Macro
 **************************************************************************** */

/* *****************************************************************************
 * Variables External Usage
 **************************************************************************** */ 

/* *****************************************************************************
 * Function Prototypes
 **************************************************************************** */
void drv_stdio_redirect_local(void);
void drv_stdio_redirect_global(void);
size_t drv_stdio_redirect_stream_push(uint8_t *pData, size_t nSize);
size_t drv_stdio_redirect_stream_pull(uint8_t *pData, size_t nSize);
void drv_stdio_redirect_uart0(bool bUseUart0);
bool drv_stdio_is_redirect_uart0(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */


