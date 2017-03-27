#include<cstdio>
#include<algorithm>
#include<climits>

extern "C" {

__global__
void initialize(int const *capacity, int *flow, int *excess, int *height, int const s_x, int const s_y, int const N) {
    
    // count coords
    int const x = (blockIdx.x * 32) + threadIdx.x;
    int const y = (blockIdx.y * 32) + threadIdx.y;
    
    if (x>=N || y>=N) return;
    
	int u = y*N + x;
    if (x == s_x && y == s_y) {
        height[u] = N;  // h(s) <- |V|
		excess[u] = INT_MAX/4;
		//printf("START HEIGHT SET TO %d %d\n", height[y*N+x], excess[y*N+x]);
    }
    else {
        height[u] = 0;  // h(u) <- 0
		excess[u] = 0;	
        //Dla dowolnego wierzchołka (x,y) przepustowości krawędzi prowadzących do jego sąsiadów wynoszą: capacity[4∗(N∗y+x)] (krawędź w górę), [...+1] (krawędź w prawo), [...+2] (krawędź w dół), [...3] (krawędź w lewo).
        
        int edge = 4*u; 
        for (int i = 0; i < 4; ++i) {
            flow[edge] = capacity[edge];    // Cf(u, v) <- Cuv
            edge++;
        }
    }
    
    __syncthreads();
    
    if (x == s_x && y == s_y) {
    
        int const vertex_x[] = {0, 1, 0, -1};
        int const vertex_y[] = {-1, 0, 1, 0};
        int edge = 4*u; 
        
        for (int i = 0; i < 4; ++i) {
            flow[edge] = 0; // Cf(s, u) <- 0
            
            int const tmpx = x+vertex_x[i];
            int const tmpy = y+vertex_y[i];
            
            if(tmpx >= 0 && tmpx < N && tmpy>=0 && tmpy < N) {
                excess[N*tmpy+tmpx] = capacity[edge];	// e(u) = C(s, u)
                flow[4*(N*tmpy+tmpx)+((i+2)%4)] += capacity[edge];// + capacity[4*(N*tmpy+tmpx)+(i+2)%4];
            }
            edge++;
            /*
            (x, y-1)
            (x+1, y)
            (x, y+1)
            (x-1, y)
            */
        }
        
        
    }    
    
    return;
}


__global__ 
void push_relabel(int *excess, int *height, int *flow, int const N, int const s_x, int const s_y, int const t_x, int const t_y) {

    // count coords
    int const x = (blockIdx.x * 32) + threadIdx.x;
    int const y = (blockIdx.y * 32) + threadIdx.y;

  if (x>=N || y>=N) return;
   
    if (x == s_x && y == s_y) return;
    if (x == t_x && y == t_y) return;

if(excess[N*y+x]>0){	
  int u = N*y + x;
	
    int temp_e = excess[u];
    int temp_v_x = -1, temp_v_y = -1;
    int temp_h = INT_MAX/2;
    int temp_v_it = -1;
    
    int const vertex_x[] = {0, 1, 0, -1};
    int const vertex_y[] = {-1, 0, 1, 0};
        
	int edge = 4*u; 
	
    for (int i = 0; i < 4; ++i, ++edge) {
    
		if(flow[edge] <= 0) continue;

        int const tmpx = x+vertex_x[i];
        int const tmpy = y+vertex_y[i];
        if(tmpx < 0 || tmpx >= N || tmpy<0 || tmpy >= N) continue;
        	
        int it = N*tmpy + tmpx;
        int try_h = height[it];
		
		if(temp_v_it == -1 || try_h < height[N*temp_v_y+temp_v_x]) {
            temp_h = try_h;
            temp_v_x = tmpx;
            temp_v_y = tmpy; 
            temp_v_it = i;
        }
    }
  
    if (temp_h < height[u]) {
    
        int d = min(temp_e, flow[4*u+temp_v_it]);
    
        atomicAdd(&flow[4*u+temp_v_it], -d);
        atomicAdd(&flow[4*(N*temp_v_y+temp_v_x)+((temp_v_it+2)%4)], d);
        atomicAdd(&excess[u], -d);
        atomicAdd(&excess[N*temp_v_y+temp_v_x], d);
        
    }
    else {
        height[u] = temp_h+1;
    }
}
     

}

__global__
void check_excess(int * excess, int * place_info, int const N, int const s_x, int const s_y, int const t_x, int const t_y) {

    // count coords
    int const x = (blockIdx.x * 32) + threadIdx.x;
    int const y = (blockIdx.y * 32) + threadIdx.y;
    
    if (x>=N || y>=N) return;
    if (s_x == x && s_y == y) return;
	
	
	if (t_x == x && t_y == y) {
		place_info[1] = excess[y*N+x];
	}
	else if (excess[y*N+x] > 0) {
		atomicAdd(&place_info[0], 1);
	}
	
}

}
