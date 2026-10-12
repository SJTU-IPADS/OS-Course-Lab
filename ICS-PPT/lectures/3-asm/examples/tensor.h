/* A matrix whose size is known only when the program runs. */
struct tensor {
    long rows, cols;
    const int *data;            /* rows * cols ints, one row after another */
};
