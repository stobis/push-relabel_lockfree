#include "cuda.h"
#include <cstdio>

int pushrelabel(int N, int s_x, int s_y, int t_x, int t_y, CUdeviceptr capacity) {
     
    //Tworzy modul z pliku binarnego "pushrelabel.ptx"
    CUmodule cuModule = (CUmodule)0;
    CUresult res = cuModuleLoad(&cuModule, "pushrelabel.ptx");
    if (res != CUDA_SUCCESS) {
        printf("cannot load module: %d\n", res);  
        exit(1); 
    }

    //Pobiera handler kernela z modulu
    CUfunction initialize;
    res = cuModuleGetFunction(&initialize, cuModule, "initialize");
    if (res != CUDA_SUCCESS){
        printf("cannot acquire init kernel handle\n");
        exit(1);
    }
    
    //Pobiera handler kernela z modulu
    CUfunction push_relabel;
    res = cuModuleGetFunction(&push_relabel, cuModule, "push_relabel");
    if (res != CUDA_SUCCESS){
        printf("cannot acquire push_relabel kernel handle\n");
        exit(1);
    }

	//Pobiera handler kernela z modulu
    CUfunction check_changes;
    res = cuModuleGetFunction(&check_changes, cuModule, "check_excess");
    if (res != CUDA_SUCCESS){
        printf("cannot acquire check_changes kernel handle\n");
        exit(1);
    }
	
    // Zalozenia: n % 32 = 0, n>=32, n<=4096
	// Grid settings 
	
    const int block_dim_x = 32;
    const int block_dim_y = 32;
    
    const int grid_dim_x = (N+31)/32;
    const int grid_dim_y = (N+31)/32;

	// Memory alloc
    CUdeviceptr flow, excess, height, dev_s_changed;
    res = cuMemAlloc(&flow, sizeof(int)*N*N*4);
    res = cuMemAlloc(&excess, sizeof(int)*N*N);
    res = cuMemAlloc(&height, sizeof(int)*N*N);
    res = cuMemAlloc(&dev_s_changed, sizeof(int)*2);
    
	// Run init kernel 
    void* args[] = {&capacity, &flow, &excess, &height, &s_x, &s_y, &N};
	
    res = cuLaunchKernel(initialize, grid_dim_x, grid_dim_y, 1, block_dim_x, block_dim_y, 1, 0, 0, args, 0);
    if (res != CUDA_SUCCESS){
        printf("cannot run kernel\n");
        exit(1);
    }    
    cuCtxSynchronize();
    
	// Prepare for main push_relabel loop
	
	int * something_changed = new int[2];
	res = cuMemHostRegister(something_changed, sizeof(int)*2, 0);
int iteracja = 1;
	something_changed[0] = 1;
	while (something_changed[0]) {
		//printf("still somewhere excess is > 0\n");
		
		void* args2[] = {&excess, &height, &flow, &N, &s_x, &s_y, &t_x, &t_y};
		void* argsSC[] = {&excess, &dev_s_changed, &N, &s_x, &s_y, &t_x, &t_y};
		
		res = cuLaunchKernel(push_relabel, grid_dim_x, grid_dim_y, 1, block_dim_x, block_dim_y, 1, 0, 0, args2, 0);
		if (res != CUDA_SUCCESS){
			printf("cannot run kernel\n");
			exit(1);
		}    
		//cuCtxSynchronize();
		
		iteracja++;
		if(iteracja%100 == 0) {
		something_changed[0] = 0;
		res = cuMemcpyHtoD(dev_s_changed, something_changed, sizeof(int)*2);
		
		res = cuLaunchKernel(check_changes, grid_dim_x, grid_dim_y, 1, block_dim_x, block_dim_y, 1, 0, 0, argsSC, 0);
		if (res != CUDA_SUCCESS){
			printf("cannot run kernel\n");
			exit(1);
		}    
		
		//cuCtxSynchronize();
		
		res = cuMemcpyDtoH(something_changed, dev_s_changed, sizeof(int)*2);
		}
	}
	cuCtxSynchronize();
    return something_changed[1];
}
