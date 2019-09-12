* Making the builder image
```sh
cd cppbuild/dockerize
docker build -t aeron/builder -f build.Dockerfile .
```

* Running the build
```sh
cd ../..
docker run -it -v $(pwd):/src --entrypoint bash aeron/builder
cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_COMPILER=gcc-9 \
    -DCMAKE_CXX_COMPILER=g++-9 \
    -DCMAKE_C_FLAGS=-fdiagnostics-color=always \
    -DCMAKE_CXX_FLAGS=-fdiagnostics-color=always \
    -DAERON_BUILD_SAMPLES=OFF \
    -DAERON_TESTS=OFF \
    -DAERON_SYSTEM_TESTS=OFF \
    -DAERON_BUILD_DOCUMENTATION=OFF \
    -DAERON_INSTALL_TARGETS=OFF \
    -DBUILD_AERON_ARCHIVE_API=ON \
    -GNinja \
    ../..
```
