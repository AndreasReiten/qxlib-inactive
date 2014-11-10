__kernel void glowstick(
    __global float * samples,
    __global float * interpol,
    int3 dim // n, m, folder_size
    )
{
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));
    
    if ((id_glb.x < dim.x) && (id_glb.y < dim.y))
    {
        for (int i = 0; i < dim.z; i++)
        {
            interpol[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x] = samples[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x];
        }
    }
}
