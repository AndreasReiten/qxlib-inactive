__kernel void imagePreview(
    __write_only image2d_t preview,
    __read_only image2d_t source,
    __read_only image2d_t tsf_source,
    __constant float * parameter,
    sampler_t tsf_sampler,
    sampler_t source_sampler,
    int correction,
    int mode,
    int log,
    __global float * intensity_buffer,
    __global float * pos_weight_x,
    __global float * pos_weight_y
    )
{
    // The frame has its axes like this, looking from the source to
    // the detector in the zero rotation position. We use the
    // cartiesian coordinate system described in
    // doi:10.1107/S0021889899007347
    //         y
    //         ^
    //         |
    //         |
    // z <-----x------ (fast)
    //         |
    //         |
    //       (slow)

    // Thresholds and other parameters essential to the file
    float noise_low = parameter[0];
    float noise_high = parameter[1];
    float pct_low = parameter[2]; // Post correction threshold
    float pct_high = parameter[3];
    float flux = parameter[4];
    float exp_time = parameter[5];
    float wavelength = parameter[6];
    float det_dist = parameter[7];
    float beam_x = parameter[8];
    float beam_y = parameter[9];
    float pix_size_x = parameter[10];
    float pix_size_y = parameter[11];
    float intensity_min = parameter[12];
    float intensity_max = parameter[13];


    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));
    int2 image_dim = get_image_dim(preview);

    if ((id_glb.x < image_dim.x) && (id_glb.y < image_dim.y))
    {
        float intensity = read_imagef(source, source_sampler, id_glb).w; /* DANGER */

        // Noise filter
        intensity = clamp(intensity, noise_low, noise_high);
        

        if (correction == 1)
        {
            float4 Q = (float4)(0.0f);
            float k = 1.0f/wavelength; // Multiply with 2pi if desired

            float3 k_i = (float3)(-k,0,0);
            float3 k_f = k*normalize((float3)(
                -det_dist,
                pix_size_x * ((float) (image_dim.y - id_glb.y) - beam_x), /* DANGER */
                pix_size_y * ((float) id_glb.x - beam_y))); /* DANGER */

            Q.xyz = k_f - k_i;
            Q.w = intensity;

            {
                float lab_theta = asin(native_divide(Q.y, k)); // This is 2theta, not theta!
                float lab_phi = atan2(Q.z,-Q.x); // This is wrong

                // Lorentz correction: Assuming rotation around the z-axis of the lab frame:
                float L = fabs(native_sin(lab_theta));

                Q.w *= L;

                // The polarization correction needs a bit more work...
                
            }

            // Post correction filter
            Q.w = clamp(Q.w, pct_low, pct_high);

            intensity = Q.w;
        }

        // Write the intensity value to a normal floating point buffer. The value can then be used later without doing all of this.
        intensity_buffer[id_glb.y*image_dim.x + id_glb.x] = intensity;
        
        // Also write the intensity value times the position to use later to find the pixel of maximum weight 
        pos_weight_x[id_glb.y*image_dim.x + id_glb.x] = intensity*(float)id_glb.x;
        pos_weight_y[id_glb.y*image_dim.x + id_glb.x] = intensity*(float)id_glb.y;
        

        float2 tsf_position;
        float4 sample;

        if (log)
        {
            if (intensity_min <= 0.001) intensity_min = 0.001;
            if (intensity <= 0.001)
            {
                tsf_position = (float2)(1.0f, 0.5f);
                sample = read_imagef(tsf_source, tsf_sampler, tsf_position) + (float4)(0.0,0.0,1.0,0.2);
            }
            else
            {
                tsf_position = (float2)(native_divide(log10(intensity) - log10(intensity_min), log10(intensity_max) - log10(intensity_min)), 0.5f);
                sample = read_imagef(tsf_source, tsf_sampler, tsf_position);
            }
        }
        else
        {
            tsf_position = (float2)(native_divide(intensity - intensity_min, intensity_max - intensity_min), 0.5f);
            sample = read_imagef(tsf_source, tsf_sampler, tsf_position);
        }
        
        write_imagef(preview, id_glb, sample);
    }
}
