git submodule update --init --depth=1 --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DNRI_STATIC_LIBRARY=ON -DBUILD_CPU_DEMOS=OFF -G "Visual Studio 17 2022"
cmake --build build -j35