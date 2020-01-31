
import perfusion
import numpy as np
import nibabel as nib
from mpi4py import MPI
import sys

# Get MPI information

comm = MPI.COMM_WORLD

size = comm.Get_size()
rank = comm.Get_rank()

if rank == 0:
    print("Fitting with %d MPI processes" % (size))

###############
## Read data ##
###############

if rank == 0:
    #Read time and Concaif data
    fileIn = open("CAIF.dat","r")
    
    data = fileIn.read()
    
    # Extract data lines
    data = data.strip().split('\n')
    
    # Extract input data
    tiempos = []
    Ca = []
    for line in data:
        words = line.strip().split()
        tiempos.append(float(words[0]))
        Ca.append(float(words[1]))
        
        fileIn.close()
    del data
        
    #Send times and Cas
    print("Rank %d: Broadcast time and Ca data to all ranks... " % (rank))
    comm.bcast(tiempos,root=0)
    comm.bcast(Ca,root=0)
    print("Rank %d: done!" % (rank))
        
    #Read voxel data	
    print("Rank %d: Read voxel data..." % (rank))
        
    # Read nifty file
    img = nib.load("target/Perfusion/20160310_075835s014a1001.nii")
    # Store information
    xvox = img.shape[0]
    yvox = img.shape[1]
    xyvox = xvox*yvox
    nplanes = img.shape[2]
    ntimes = img.shape[3]
    nvox = xyvox*nplanes
        
    print("Rank 0: Voxels on x,y: %d %d" % (xvox,yvox))
    print("Rank 0: Time points: %d" % (ntimes))
    print("Rank 0: Planes: %d" % (nplanes))    
    print("Rank 0: Total voxels: %d" % (nvox))
        
    #Check dimensions
    if ntimes != len(tiempos):
        print("Rank %d: Number of time points mismatch!!" % (rank))
        print("%d != %d" % (ntimes,len(tiempos)))
        sys.exit()
            
            
    #Get image voxel data from nifty file
    voxels = np.reshape(img.get_fdata(),(xvox*yvox*nplanes,ntimes))
    print("Rank %d: done!" % (rank))


    #Calculate image partitions to be fitted by each process
    nvoxPerProcess = int(nvox/size)
    nvoxResidual = nvox - nvoxPerProcess*size

    nvoxLocal = nvoxPerProcess + nvoxResidual

    localVoxels = np.array(voxels[0:nvoxLocal])
    pos = nvoxLocal
    #Send partial data to each process
    for i in list(range(size-1)):
        print("Rank %d: Send partial voxel data to rank %d... " % (rank,i+1))
        comm.send(voxels[pos:pos+nvoxPerProcess], dest=i+1, tag=11)
        pos = pos + nvoxPerProcess
        print("Rank %d: done!" % (rank))
    #Free memory
    del voxels

else:

    #Send times and Cas
    tiempos = None
    Ca = None

    print("Rank %d: Receive time and Ca data..." % (rank))
    tiempos = comm.bcast(tiempos,root=0)
    Ca      = comm.bcast(Ca,root=0)
    print("Rank %d: done!" % (rank))

    ntimes = len(tiempos)

    #Recive voxels
    print("Rank %d: Receive partial voxel data... " % (rank))
    localVoxels = None
    localVoxels = comm.recv(source=0, tag=11)
    print("Rank %d: done!" % (rank))


    nvoxLocal = len(localVoxels)

#Create the fitter
fitter = perfusion.perfusion(nvoxLocal,ntimes)
print("Rank %d: Fitter created" % (rank))

#Set data
fitter.storeCa(tiempos,Ca)
fitter.storeData(localVoxels)

del tiempos
del Ca
del localVoxels

print("Rank %d: Data stored" % (rank))


#Fit with custom parameters
nthreads = 1

initKtrans = 0.03
initKep    = 0.18

lowKtrans  = 1.0e-6
topKtrans  = 1.0e5

lowKep  = 1.0e-6
topKep  = 1.0e5

stepSize = 1.0e-4
tolerance = 1.0e-2

fitter.fitWithParam(nthreads,initKtrans,initKep,lowKtrans,topKtrans,lowKep,topKep,stepSize,tolerance)

print("Rank %d: Fit done" % (rank))

#Create results array
localResults = np.array(np.zeros(shape=(nvoxLocal,4)), dtype=np.double)

#read results
fitter.readResults(localResults)

#Join results at rank 0

if rank == 0:

    print("Rank %d: Join results... " % (rank))

    results = np.zeros(shape = (nvox,4), dtype=np.double)
    results[0:nvoxLocal] = localResults
    pos = nvoxLocal
    auxData = np.zeros(shape = (nvoxPerProcess,4), dtype=np.double)
    for i in list(range(size-1)):
        print("Rank %d: Receive results from %d" % (rank,i+1))
        auxData = comm.recv(source=i+1,tag=10)
        results[pos:pos+nvoxPerProcess] = auxData
        pos = pos + nvoxPerProcess
    del auxData
    print("Rank %d: done!" % (rank))
    print("Rank %d: Print results..." % (rank))

    #Write results
    with open("results.dat", "w") as f:    
        for vox in results:
            f.write(' '.join(map(str,vox)))
            f.write('\n')
    print("Rank %d: done" % (rank))
    print('Rank %d: Results stored at "results.dat"' % (rank))
    
else:
    print("Rank %d: Send partial results" % (rank))
    comm.send(localResults,dest=0,tag=10)
    print("Rank %d: done" % (rank))
print('Rank %d: Computation done!' % (rank))
