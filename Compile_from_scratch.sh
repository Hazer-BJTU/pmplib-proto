rm -rf bin/*
rm -rf build/*
rm -rf lib/*
cmake -B build -DGENERATE_TEST_TARGETS=ON -DBUILD_RELEASE=OFF
cmake --build build -j$(nproc)
