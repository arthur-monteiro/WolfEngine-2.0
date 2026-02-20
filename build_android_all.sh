set -e
echo "Starting Wolf-Engine android build..."
chmod +x gradlew
./gradlew assembleDebug copyStaticLibs
if [ $? -ne 0 ]; then
    echo ""
    echo "Build failed with code $?"
    exit $?
fi

echo ""
echo "Build successful!"