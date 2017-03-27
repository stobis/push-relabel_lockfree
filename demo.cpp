#include "pushrelabel.h"
#include <cstdio>
#include <algorithm>
#include <cuda.h>
using namespace std;

int rand1(int lim) {
    static long a = 12100001;
    a = (a * 125) % 2796203;
    return ((a % lim) + 1);
}

int main() {
    int N = 32;
    int* capacity = (int*) malloc(sizeof(int)*4*N*N);
    for (int i=0; i<4*N*N; ++i) capacity[i] = 0;
    int zrodlo_x = 30;
    int zrodlo_y = 5;
    int ujscie_x = 29;
    int ujscie_y = 0;
    for (int i = 0; i < N; ++i){
        for(int j = 0; j < N; ++j){
        	if (j+1 < N){
        	    //left(i, j) == right(i, j+1)
        		capacity[4*(N*i+j)+1] = capacity[4*(N*i+j+1)+3] = rand1(1000000); 
        	}
            if (i+1 < N){
                //down(i, j) == up(i+1, j)
                capacity[4*(N*i+j)+2] = capacity[4*(N*(i+1)+j)] = rand1(100000);
            }
        }
    }
    //kasowanie krawedzi z ujscia
    //up
    capacity[4*(N*ujscie_y+ujscie_x)] = 0;
    //left
    capacity[4*(N*ujscie_y+ujscie_x)+1] = 0;
    //down
    capacity[4*(N*ujscie_y+ujscie_x)+2] = 0;
    //right
    capacity[4*(N*ujscie_y+ujscie_x)+3] = 0;

    cuInit(0);
    CUdevice cuDevice;
    CUresult res = cuDeviceGet(&cuDevice, 0);
    if (res != CUDA_SUCCESS){
        printf("fail in line %d\n", __LINE__);
    }
    CUcontext cuContext;
    res = cuCtxCreate(&cuContext, 0, cuDevice);
    if (res != CUDA_SUCCESS){
        printf("fail in line %d\n", __LINE__);
    }
        CUdeviceptr dCapacity;
    res = cuMemAlloc(&dCapacity, 4*N*N*sizeof(int));
    if (res != CUDA_SUCCESS){
        printf("fail in line %d\n", __LINE__);
    }
    res = cuMemcpyHtoD(dCapacity, capacity, 4*N*N*sizeof(int));
    if (res != CUDA_SUCCESS){
        printf("fail in line %d\n", __LINE__);
    }

    int flowRes = pushrelabel(N, zrodlo_x, zrodlo_y, ujscie_x, ujscie_y, dCapacity);
    printf("%d\n", flowRes);
    cuMemFree(dCapacity);
    free(capacity);
    cuCtxDestroy(cuContext);
    return 0;
}
