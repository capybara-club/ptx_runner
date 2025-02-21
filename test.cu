extern "C"
__global__ 
void 
k0(
    float *in,
    float *out,
    size_t ldN,
    float f1,
    float f2,
    float f3
) {
    size_t COL_0 = 0;
    size_t COL_1 = ldN;
    size_t COL_2 = 2*ldN;

    if (threadIdx.x < ldN) {
        out[COL_0 + threadIdx.x] = f3;
        out[COL_1 + threadIdx.x] = f2;

        out[COL_2 + threadIdx.x] = in[COL_0 + threadIdx.x] + 99.0f;
    }
}
