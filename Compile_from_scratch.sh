rm -rf bin/*
rm -rf build/*
rm -rf lib/*
cmake -B build
cmake --build build -j$(nproc)
