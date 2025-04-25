rm -rf bin/tests/*
rm -rf build/*
cd build
cmake ..
make -j$(nproc)
