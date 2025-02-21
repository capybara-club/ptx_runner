#include "mmap_file.h"

#include <cuda.h>
#include <nvPTXCompiler.h>

#define ARRAY_NUM_ELEMS(array) sizeof((array)) / sizeof(*(array))

#define cuCheck(ans) gpuAssert((ans), __FILE__, __LINE__, 1)
#define cuCheck_no_abort(ans) gpuAssert((ans), __FILE__, __LINE__, 0)
static void gpuAssert(CUresult code, const char *file, int line, int abort)
{
    if (code != CUDA_SUCCESS) 
    {
        do {
            const char* error_name;
            CUresult result;
            result = cuGetErrorName(code, &error_name);
            if (result != CUDA_SUCCESS) {
                fprintf(stderr, "GPUassert: failed to get error name: %d %d %s %d", code, result, file, line);
                break;
            }
            const char* error_string;
            result = cuGetErrorString(code, &error_string);
            if (result != CUDA_SUCCESS) {
                fprintf(stderr, "GPUassert: failed to get error string: %d %d %s %d", code, result, file, line);
                break;
            }

            fprintf(stderr, "GPUassert: %s \n  %s \n  %s %d\n", error_name, error_string, file, line);
        } while(0);
        if (abort) exit(code);
    }
}

#define nvPTXCompilerCheck(ans) checkNVPTXCompilerStatus((ans), __FILE__, __LINE__)
static void checkNVPTXCompilerStatus(nvPTXCompileResult status, const char *file, int line) {
    if (status != NVPTXCOMPILE_SUCCESS) {
        printf("nvPTXCompiler API failed with status %d %s %d\n", status, file, line);
        exit(1);
    }
}

int main(int argc, char **argv) { 
    if (argc != 2) {
        printf("Not enough program arguments\n");
        exit(EXIT_FAILURE);
    }

    const char *file_name = argv[1];
    MRead mr = mmap_file_open_ro(file_name);
    uint8_t *file_data = mr.mapped_data;
    size_t file_data_size = mr.mapped_size;

    int major_version, minor_version;

    nvPTXCompilerCheck( 
        nvPTXCompilerGetVersion(&major_version, &minor_version) 
    );

    printf("%d : %d\n", major_version, minor_version);

    nvPTXCompilerHandle compiler;
    nvPTXCompileResult status;

    nvPTXCompilerCheck(
        nvPTXCompilerCreate(&compiler, file_data_size, file_data)
    );


    static const char* compile_options[] = { 
        "--gpu-name=sm_89",
        "--verbose"
    };

    static const size_t num_compile_options = ARRAY_NUM_ELEMS(compile_options);

    status = nvPTXCompilerCompile(compiler, num_compile_options, compile_options);
    mmap_file_close(mr);

    if (status != NVPTXCOMPILE_SUCCESS) {
        size_t error_log_size;
        nvPTXCompilerCheck( nvPTXCompilerGetErrorLogSize(compiler, &error_log_size) );

        if (error_log_size != 0) {
            char *error_log = (char*)malloc(error_log_size+1);
            nvPTXCompilerCheck( nvPTXCompilerGetErrorLog(compiler, error_log) );
            printf("Error log: \n\n%s\n", error_log);
            free(error_log);
        }
        exit(1);
    }

    size_t elf_size;
    nvPTXCompilerCheck( nvPTXCompilerGetCompiledProgramSize(compiler, &elf_size) );

    char *elf = (char*) malloc(elf_size);
    nvPTXCompilerCheck( nvPTXCompilerGetCompiledProgram(compiler, (void*)elf) );

    // CubinStatsKernelParams cubin_stats_kernel_params;

    const char *kernel_names[] = {
        "k0"
    };

    // int result = r_cubin_stats_kernel_params(elf, 1, kernel_names, &cubin_stats_kernel_params);
    // printf("result: %d\n", result);

    // r_cubin_stats_kernel_params_print(cubin_stats_kernel_params);

    size_t info_log_size;
    nvPTXCompilerCheck( nvPTXCompilerGetInfoLogSize(compiler, &info_log_size) );

    if (info_log_size != 0) {
        char *info_log = (char*)malloc(info_log_size+1);
        nvPTXCompilerCheck( nvPTXCompilerGetInfoLog(compiler, info_log) );
        printf("Info log: \n\n%s\n", info_log);
        free(info_log);
    }

    nvPTXCompilerCheck( nvPTXCompilerDestroy(&compiler) );
    
    printf("%zu\n", elf_size);

    cuCheck( cuInit(0) );
    
    const int device_num = 0;

    CUdevice device;
    cuCheck( cuDeviceGet(&device, device_num) );

    CUcontext context;
    cuCheck( cuCtxCreate(&context, 0, device) );

    CUmodule module;
    cuCheck( cuModuleLoadData(&module, elf) );
    free(elf);

    CUfunction function;
    cuCheck( cuModuleGetFunction(&function, module, "k0") );

    const size_t IN_COLS = 1;
    const size_t OUT_COLS = 3;

    CUdeviceptr d_input;
    CUdeviceptr d_output;

    const int num_blocks = 1;
    const int num_threads = 32;

    cuCheck( cuMemAlloc(&d_input, num_threads*sizeof(float)*IN_COLS) );
    cuCheck( cuMemAlloc(&d_output, num_threads*sizeof(float)*OUT_COLS) );

    float *h_input = malloc(num_threads * sizeof(float) * IN_COLS);
    float *h_output = malloc(num_threads * sizeof(float) * OUT_COLS);

    for (int i = 0; i < num_threads * IN_COLS; i++) {
        h_input[i] = i;
    }
    cuCheck( cuMemcpyHtoD(d_input, h_input, num_threads*sizeof(float)* IN_COLS) );


    size_t ldN = num_threads;
    float f1 = 0.33f;
    float f2 = 0.16f;
    float f3 = 0.0009f;

    void *args[] = {
        (void *)&d_input, 
        (void *)&d_output, 
        (void *)&ldN, 
        (void*)&f1,
        (void*)&f2,
        (void*)&f3,
    };

    cuCheck( 
        cuLaunchKernel(
            function, 
            num_blocks, 1, 1, 
            num_threads, 1, 1, 
            0, NULL, args, NULL
        ) 
    );
    cuCheck( cuCtxSynchronize() );
    cuCheck( cuModuleUnload(module) );

    cuCheck( cuMemcpyDtoH(h_input, d_input, num_threads * sizeof(float) * IN_COLS) );

    for (int col = 0; col < IN_COLS; col++) {
        printf("IN COL: %d\n", col);
        for (int i = 0; i < num_threads; i++) {
            printf("value: %d : %f\n", i, h_input[i + ldN * col]);
        }
    }

    
    cuCheck( cuMemcpyDtoH(h_output, d_output, num_threads * sizeof(float) * OUT_COLS));

    for (int col = 0; col < OUT_COLS; col++) {
        printf("OUT COL: %d\n", col);
        for (int i = 0; i < num_threads; i++) {
            printf("value: %d : %f\n", i, h_output[i + ldN * col]);
        }
    }

    cuCheck( cuMemFree(d_input) );
    cuCheck( cuMemFree(d_output) );
    free(h_input);
    free(h_output);

}
