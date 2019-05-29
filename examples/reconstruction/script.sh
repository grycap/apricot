#!/bin/bash
#SBATCH --output=results/__RUN_ID__/log-__0__-__1__-__2__

#Create a directory for this job
echo "Create job folder"
mkdir results/__RUN_ID__/__0__-__1__-__2__

#Copy executables
echo "Copy executables"
cp input/reconstructor results/__RUN_ID__/__0__-__1__-__2__
cp input/evaluateImage results/__RUN_ID__/__0__-__1__-__2__

#Move to directory
cd results/__RUN_ID__/__0__-__1__-__2__

echo "NVoxels: __0__ __0__ __1__"

echo "N chunks: __2__"

#Reconstruct image
time ./reconstructor --LOR-format 0 --nvox __0__ __0__ __1__ --dvox `python -c "print(28.0/float(__0__))"` `python -c "prin
t(28.0/float(__0__))"` `python -c "print(28.0/float(__1__))"` --nChunks __2__ --niter 1 --kdim 2 --sensitivity NONE /home/u
buntu/input/completeSimulationData.dat

#Compare result with reference image
./evaluateImage normalizedCompleteData.txt __0__ __0__ __1__ `python -c "print(28.0/float(__0__))"` `python -c "print(28.0/
float(__0__))"` `python -c "print(28.0/float(__1__))"`

#Remove reconstructed image to save disk space
rm normalizedCompleteData.txt
