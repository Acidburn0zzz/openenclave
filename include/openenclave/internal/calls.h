// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifndef _OE_CALLS_H
#define _OE_CALLS_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <openenclave/internal/cpuid.h>
#include <openenclave/internal/defs.h>
#include <openenclave/internal/syscall/unistd.h>
#include "backtrace.h"

OE_EXTERNC_BEGIN

typedef struct _oe_enclave oe_enclave_t;

typedef void (*oe_ecall_function)(uint64_t arg_in, uint64_t* arg_out);

typedef void (*oe_ocall_function)(uint64_t arg_in, uint64_t* arg_out);

/*
**==============================================================================
**
** The flags parameter for oe_ocall()
**
** Flags stack with the ones on the current thread (i.e., are or'd together)
** for the duration of the ocall.
**
**==============================================================================
*/

/*
**==============================================================================
**
** oe_code_t
**
**     The code parameter for oe_ecall() and oe_ocall()
**
**==============================================================================
*/

typedef enum _oe_code
{
    OE_CODE_NONE = 0,
    OE_CODE_ECALL = 1,
    OE_CODE_ERET = 2,
    OE_CODE_OCALL = 3,
    OE_CODE_ORET = 4,
    __OE_CODE_MAX = OE_ENUM_MAX,
} oe_code_t;

OE_STATIC_ASSERT(sizeof(oe_code_t) == sizeof(unsigned int));

/*
**==============================================================================
**
** oe_func_t
**
**     The func parameter for oe_ecall() and oe_ocall()
**
**==============================================================================
*/

/* ECALL function numbers are in the range: [0:32765] */
#define OE_ECALL_BASE 0

/* OCALL function numbers are in the range: [32768:65535] */
#define OE_OCALL_BASE 0x8000

/* Function numbers are 16 bit integers */
typedef enum _oe_func
{
    OE_ECALL_DESTRUCTOR = OE_ECALL_BASE,
    OE_ECALL_INIT_ENCLAVE,
    OE_ECALL_CALL_ENCLAVE_FUNCTION,
    OE_ECALL_VIRTUAL_EXCEPTION_HANDLER,
    OE_ECALL_CALL_AT_EXIT_FUNCTIONS,
    /* Caution: always add new ECALL function numbers here */
    OE_ECALL_MAX,

    OE_OCALL_CALL_HOST_FUNCTION = OE_OCALL_BASE,
    OE_OCALL_THREAD_WAKE,
    OE_OCALL_THREAD_WAIT,
    OE_OCALL_MALLOC,
    OE_OCALL_FREE,
    OE_OCALL_GET_TIME,
    /* Caution: always add new OCALL function numbers here */
    OE_OCALL_MAX, /* This value is never used */

    __OE_FUNC_MAX = OE_ENUM_MAX,
} oe_func_t;

OE_STATIC_ASSERT(sizeof(oe_func_t) == sizeof(unsigned int));

/*
**==============================================================================
**
** oe_make_call_arg1()
**
**     Form the 'arg1' parameter to both oe_enter() and oe_exit(). This
**     parameter is a 64-bit integer that contains:
**
**         code -- indicating whether ECALL, OCALL, ERET, or ORET
**         func -- the number of the function being called
**         flags -- any bit flags
**         result -- the result of the transport (not the function)
**
**==============================================================================
*/

OE_INLINE uint64_t oe_make_call_arg1(
    oe_code_t code,
    oe_func_t func,
    uint16_t flags,
    oe_result_t result)
{
    /* [ CODE:16 | FUNC:16 | FLAGS:16 | RESULT:16 ] */
    return ((uint64_t)code << 48) | ((uint64_t)func << 32) |
           ((uint64_t)flags << 16) | ((uint64_t)result);
}

/*
**==============================================================================
**
** oe_get_code_from_call_arg1()
**
**==============================================================================
*/

OE_INLINE oe_code_t oe_get_code_from_call_arg1(uint64_t arg)
{
    return (oe_code_t)((0xffff000000000000 & arg) >> 48);
}

/*
**==============================================================================
**
** oe_get_func_from_call_arg1()
**
**==============================================================================
*/

OE_INLINE uint16_t oe_get_func_from_call_arg1(uint64_t arg)
{
    return (uint16_t)((0x0000ffff00000000 & arg) >> 32);
}

/*
**==============================================================================
**
** oe_get_flags_from_call_arg1()
**
**==============================================================================
*/

OE_INLINE uint16_t oe_get_flags_from_call_arg1(uint64_t arg)
{
    return (uint16_t)((0x00000000ffff0000 & arg) >> 16);
}

/*
**==============================================================================
**
** oe_get_result_from_call_arg1()
**
**==============================================================================
*/

OE_INLINE uint16_t oe_get_result_from_call_arg1(uint64_t arg)
{
    return (uint16_t)(0x000000000000ffff & arg);
}

/*
**==============================================================================
**
** oe_call_enclave_function_args_t
**
**==============================================================================
*/

typedef struct _oe_call_enclave_function_args
{
    uint64_t function_id;
    const void* input_buffer;
    size_t input_buffer_size;
    void* output_buffer;
    size_t output_buffer_size;
    size_t output_bytes_written;
    oe_result_t result;
} oe_call_enclave_function_args_t;

/*
**==============================================================================
**
** oe_call_host_function_args_t
**
**==============================================================================
*/

typedef struct _oe_call_host_function_args
{
    uint64_t function_id;
    const void* input_buffer;
    size_t input_buffer_size;
    void* output_buffer;
    size_t output_buffer_size;
    size_t output_bytes_written;
    oe_result_t result;
} oe_call_host_function_args_t;

/*
**==============================================================================
**
** oe_call_function_return_args_t
**
**==============================================================================
*/

typedef struct _oe_call_function_return_args
{
    oe_result_t result;
    void* deepcopy_out_buffer;
    size_t deepcopy_out_buffer_size;
} oe_call_function_return_args_t;

/*
**==============================================================================
**
** oe_call_host_function_internal()
**
**==============================================================================
*/

oe_result_t oe_call_host_function_internal(
    size_t function_id,
    const void* input_buffer,
    size_t input_buffer_size,
    void* output_buffer,
    size_t output_buffer_size,
    size_t* output_bytes_written,
    bool switchless);

/**
 * Perform a low-level enclave function call (ECALL).
 *
 * This function performs a low-level enclave function call by invoking the
 * function indicated by the **func** parameter. The enclave defines a
 * corresponding function with the following signature.
 *
 *     void (*)(uint64_t arg_in, uint64_t* arg_out);
 *
 * The meaning of the **arg_in** arg **arg_out** parameters is defined by the
 * implementer of the function and either may be null.
 *
 * Open Enclave uses the low-level ECALL interface to implement internal calls,
 * used by oe_call_enclave() and oe_terminate_enclave(). Enclave application
 * developers are encouraged to use oe_call_enclave() instead.
 *
 * At the software layer, this function sends an **ECALL** message to the
 * enclave and waits for an **ERET** message. Note that the ECALL implementation
 * may call back into the host (an OCALL) before returning.
 *
 * At the hardware layer, this function executes the **ENCLU.EENTER**
 * instruction to enter the enclave. When the enclave returns from the ECALL,
 * it executes the **ENCLU.EEXIT** instruction exit the enclave and to resume
 * host execution.
 *
 * Note that the return value only indicates whether the ECALL was called and
 * not whether it was successful. The ECALL implementation must define its own
 * error reporting scheme based on its parameters.
 *
 * @param func The number of the function to be called.
 * @param args_in The input argument passed to the function.
 * @param arg_out The output argument passed back from the function.
 *
 * @retval OE_OK The function was successful.
 * @retval OE_FAILED The function failed.
 * @retval OE_INVALID_PARAMETER One or more parameters is invalid.
 * @retval OE_OUT_OF_THREADS No enclave threads are available to make the call.
 * @retval OE_UNEXPECTED An unexpected error occurred.
 *
 */
oe_result_t oe_ecall(
    oe_enclave_t* enclave,
    uint16_t func,
    uint64_t arg_in,
    uint64_t* arg_out);

/**
 * Perform a low-level host function call (OCALL).
 *
 * This function performs a low-level host function call by invoking the
 * function indicated by the **func** parameter. The host defines a
 * corresponding function with the following signature.
 *
 *     void (*)(uint64_t arg_in, uint64_t* arg_out);
 *
 * The meaning of the **arg_in** arg **arg_out** parameters is defined by the
 * implementer of the function and either may be null.
 *
 * Open Enclave uses this interface to implement internal calls. Enclave
 * application developers are encouraged to use oe_call_host() instead.
 *
 * At the software layer, this function sends an **OCALL** message to the
 * enclave and waits for an **ORET** message. Note that the OCALL implementation
 * may call back into the enclave (an ECALL) before returning.
 *
 * At the hardware layer, this function executes the **ENCLU.EEXIT**
 * instruction to exit the enclave. When the host returns from the OCALL,
 * it executes the **ENCLU.EENTER** instruction to reenter the enclave and
 * resume execution.
 *
 * Note that the return value only indicates whether the OCALL was called
 * not whether it was successful. The ECALL implementation must define its own
 * error reporting scheme based on its parameters.
 *
 * @param func The number of the function to be called.
 * @param arg_in The input argument passed to the function.
 * @param arg_out The output argument passed back from the function.
 *
 * @retval OE_OK The function was successful.
 * @retval OE_FAILED The function failed.
 * @retval OE_INVALID_PARAMETER One or more parameters is invalid.
 * @retval OE_OUT_OF_THREADS No enclave threads are available to make the call.
 * @retval OE_UNEXPECTED An unexpected error occurred.
 *
 */
oe_result_t oe_ocall(uint16_t func, uint64_t arg_in, uint64_t* arg_out);

OE_EXTERNC_END

#endif /* _OE_CALLS_H */
