#include "matrix.h"

Matrix matrix_product(Matrix a, Matrix b)
{
    Matrix output;
    size_t valid_size = MIN(a.columns, b.rows); /* maximum computable size if matrices are not fully multipliable */
    int i, j, k; /* row, column and operand indices for iterating over matrix elements */

    output.rows = a.rows;
    output.columns = b.columns;
    output.data = malloc(output.rows * output.columns * sizeof(*a.data));

    for (i = 0; i < output.rows; i++)
        for (k = 0; k < valid_size; k++)
            for (j = 0; j < output.columns; j++)
            {
                *(output.data + i * output.columns + j) +=
                    *(a.data + i * a.columns + k) * *(b.data + k * b.columns + j);
            }

    return output;
}

Matrix3x3 matrix3x3_product(Matrix3x3 a, Matrix3x3 b)
{
    Matrix3x3 output = { { 0 } };
    const int N = 3; /* square matrix dimension */
    int i, j, k; /* row, column, and operand indices for iterating over matrix elements */

    for (i = 0; i < N; i++)
        for (k = 0; k < N; k++)
            for (j = 0; j < N; j++)
            {
                output.data[i][j] += a.data[i][k] * b.data[k][j];
            }

    return output;
}

Matrix matrix_transpose(Matrix input)
{
    Matrix output;
    int i, j; /* row and column indices for iterating over matrix elements */

    output.rows = input.columns;
    output.columns = input.rows;
    output.data = malloc(output.rows * output.columns * sizeof(*input.data));

    for (i = 0; i < output.rows; i++)
        for (j = 0; j < output.columns; j++)
        {
            *(output.data + i * output.columns + j) = *(input.data + j * input.columns + i);
        }

    return output;
}
