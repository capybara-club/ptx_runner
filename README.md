# ptx_runner

## Build

`mkdir build && cd build && cmake ..` 

needs cuda toolkit and driver, paths might be messed up.

## Usage 

./build/ptx_runner test.ptx

You can change `IN_COLS` `OUT_COLS`

You can edit a ptx file easily by going to godbolt and typing cuda code, make sure the filters are set so the variables are declared. 

See https://godbolt.org/z/eo5xEeqKh