ndk-build APP_PLATFORM=33 APP_BUILD_SCRIPT=Android.mk APP_STL:=c++_static APP_ABI=all NDK_PROJECT_PATH=. NDK_OUT=./../ndk-builds
for dir in ../ndk-builds/local/*; do \
    abi=$(basename "$dir"); \
    mkdir -p "../ndk-builds/$abi"; \
    mv "$dir"/*.a "../ndk-builds/$abi/"; \
done && rm -rf ../ndk-builds/local
