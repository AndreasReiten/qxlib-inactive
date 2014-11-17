__kernel void glowstick(
    __global float * samples,
    __global float * interpol,
    int3 dim // n, m, folder_size
    )
{
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));
    
    if ((id_glb.x < dim.x) && (id_glb.y < dim.y))
    {
        float average = 0;    

        for (int i = 0; i < dim.z; i++)
        {
            average += samples[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x];
        }
        average /= (float)(dim.z);
        

        float variance = 0;
        for (int i = 0; i < dim.z; i++)
        {
            variance += pow(samples[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x] - variance, 2.0f);
        }
        
        variance /= (float)(dim.z); 
        
        float std_dev = sqrt(variance); 
        
        for (int i = 0; i < dim.z; i++)
        {
            
            if (samples[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x] > average + std_dev) interpol[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x] = 1000;
            else interpol[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x] = samples[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x];
        }
    }
}
