#ifndef MATRIX_H
#define MATRIX_H

#include <stdlib.h>
#include "common.h"

typedef struct Matrix
{
    size_t rows;
    size_t columns;
    float *data;
} Matrix;

typedef struct Matrix3x3
{
    float data[3][3];
} Matrix3x3;

Matrix matrix_product(Matrix a, Matrix b);
Matrix3x3 matrix3x3_product(Matrix3x3 a, Matrix3x3 b);

#define MATRIX_3X3_IDENTITY { { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } } }

#endif /* MATRIX_H */
