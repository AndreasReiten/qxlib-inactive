__kernel void rectCopyFloat(
    __global float * in,
    int2 in_size,
    int2 in_origin,
    int in_row_pitch, 
    __global float * out,
    int2 out_origin,
    int out_row_pitch,
    int2 region)
{
    // Use to copy a rectangular region from a buffer into another buffer.
    
    if ((get_global_id(0) + in_origin.x < in_size.x) && (get_global_id(1) + in_origin.y < in_size.y))
    {
        int2 in_id = (int2)(in_origin.x + get_global_id(0), in_origin.y + get_global_id(1));
        
        int2 out_id = (int2)(out_origin.x + get_global_id(0), out_origin.y + get_global_id(1));
        
        out[out_id.y*out_row_pitch + out_id.x] = in[in_id.y*in_row_pitch + in_id.x];
    }
}
