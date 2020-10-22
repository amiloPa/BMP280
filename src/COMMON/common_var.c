/*
 * common_var.c
 *
 *  Created on: 22.10.2020
 *      Author: Piotr
 */


#include "common_var.h"

int my_abs(int x)
{
    return x < 0 ? -x : x;
}


uint32_t my_abs_uint(uint32_t x)
{
    return x < 0 ? -x : x;
}
