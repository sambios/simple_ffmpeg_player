rm -fr build
mkdir build
cd build

if [ "$1" == bmcodec ];then
    cmake -DUSE_BM_CODEC=ON -DUSE_SYSTEM_OPENCV=ON ..
else
    cmake -DUSE_SYSTEM_OPENCV=ON ..
fi

make -j
cd ..



