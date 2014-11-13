__kernel void reduction(const int entries, __global float *a, __global float *reda){
  
  int group = get_group_id(0);
  int item = get_local_id(0);
  
  __local float s_a[p_Nred];
  
  s_a[item] = a[group*p_Nred + item];
  
  for(int alive=p_Nred/2;alive>=1;alive/=2){
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    if(item<alive)
      s_a[item] += s_a[item+alive];
  }
  
  if(item==0){
    reda[group] = s_a[0];
  }
}
