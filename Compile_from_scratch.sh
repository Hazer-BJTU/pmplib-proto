rm -rf bin/*
touch bin/.bin
rm -rf build/*
touch build/.build
rm -rf lib/*
touch lib/.lib
rm -rf config/*
touch config/.config
cmake -B build -DGENERATE_TEST_TARGETS=ON -DBUILD_RELEASE=OFF
cmake --build build -j$(nproc)
