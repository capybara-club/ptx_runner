# ptx_runner

## Build

`mkdir build && cd build && cmake ..` 

needs cuda toolkit and driver, paths might be messed up.

## Usage 

`./build/ptx_runner test.ptx`

Change `--gpu-name=sm_89` in `main.c` to be the cc of your card.

You can change `IN_COLS` `OUT_COLS` in `main.c` as well.

You can edit a ptx file easily by going to godbolt and typing cuda code, make sure the filters are set so the variables are declared. Or can do `nvcc -ptx test.cu`. Kernel name needs to be `k0` with `extern "C"` to avoid name mangling.

See https://godbolt.org/z/eo5xEeqKh or `test.cu` compiled to `test.ptx`
