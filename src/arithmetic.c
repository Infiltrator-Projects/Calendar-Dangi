// SPDX-License-Identifier: GPL-3.0-or-later
#include "infiltratr/arithmetic.h"

#include <limits.h>
#include <stdlib.h>


bool infiltratr_size_add_checked(size_t left, size_t right, size_t *result)
{
    if (!result || right > SIZE_MAX - left) return false;
    *result = left + right;
    return true;
}

bool infiltratr_size_multiply_checked(size_t left, size_t right,
                                      size_t *result)
{
    if (!result || (left != 0U && right > SIZE_MAX / left)) return false;
    *result = left * right;
    return true;
}


