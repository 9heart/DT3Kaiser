export ANDROID_NDK_ROOT=/Developer/android-ndk-r9d
export ANDROID_NDK=/Developer/android-ndk-r9d

cmake -DCMAKE_TOOLCHAIN_FILE=android.toolchain.cmake -DANDROID_NATIVE_API_LEVEL=13 "-DANDROID_TOOLCHAIN_NAME=arm-linux-androideabi-4.8" "-DANDROID_ABI=armeabi-v7a"
make -j8 VERBOSE=1

cp ../../install/data.pack project/assets
cp ../../install/data/config.txt project/assets