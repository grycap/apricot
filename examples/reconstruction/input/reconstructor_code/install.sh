
#Per defecte compilem sense tot els warnings
COMPILE_WITH_WARNINGS="ON"
OMP="ON"
CCXX=""

#Clean objects
cd src/code
rm -rf *.x *.o &> /dev/null
cd ../../

rm -r build &> /dev/null
mkdir build
cd build

if [[ $CCXX = *[!\ ]* ]]; then
    cmake -D CMAKE_CXX_COMPILER=${CCXX} -DWITH_WARNINGS=${COMPILE_WITH_WARNINGS} -DWITH_OMP=${OMP} ../
else
	cmake -DWITH_WARNINGS=${COMPILE_WITH_WARNINGS} -DWITH_OMP=${OMP} ../
fi

make

cp reconstructor ../

echo "Complete"
