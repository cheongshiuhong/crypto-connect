# Example Strategy Implementation in C++

## Overview
- There should be a `config.yaml` file here, providing the API and secret keys to the exchange adapter.

## Example files
- `strategy.hpp` and `strategy.cpp` is a demo implementation of the BaseStrategy.
- `main.cpp` is the main entrypoint instantiating the strategy and adapter and starting it.
- `downloader.cpp` and `filter.cpp` show some simple implementations of using the adapter to get product details and historical data for the purpose of filtering our initial trading universe.


## Additional Details

1. Makefile
The Makefile here links the .so shared object and the include directory based on relative paths since it wouldn't make sense to copy the headers into this example directory. Do adjust your Makefile accordingly.


1. We can use helpers from Python
Use Cython API to call compiled Python code shown in the `pyhelpers/`
directory, then use the Python C-API's Interpreter to run the Python code.
