static void selectionSort(float * a, int n);

__kernel void glowstick(
    __global float * samples,
    __global float * interpol,
    int3 dim // n, m, folder_size
    )
{
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));
    
    if ((id_glb.x < dim.x) && (id_glb.y < dim.y))
    {
//        // Moving average smoothing
//        // Number of samples to include on each side
//        int tail = 2;

//        for (int i = 0; i < dim.z; i++)
//        {
//            interpol[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x] = 0.0f;

//            for (int j = i-tail; j <= i+tail; j++)
//            {
//                interpol[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x] += samples[dim.x*dim.y*clamp(j, 0, dim.z-1) +  dim.x*id_glb.y + id_glb.x];
//            }

//            interpol[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x] /= (float)(tail*2+1);
//        }

//        // Mova new data back to samples buffer
//        for (int i = 0; i < dim.z; i++)
//        {
//            samples[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x] = interpol[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x];
//        }

        // Moving median filter
        // Implement a basic sorting algorithm for a few values and use median filter
        int tail = 4;
        float median[9]; // tail * 2 + 1

        for (int i = 0; i < dim.z; i++)
        {
            int iter = 0;
            for (int j = i-tail; j <= i+tail; j++)
            {
                median[iter] = samples[dim.x*dim.y*clamp(j, 0, dim.z-1) +  dim.x*id_glb.y + id_glb.x];
                iter++;
            }
            
            selectionSort(median, tail*2+1);

            interpol[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x] = median[tail];
        }

    }
}

static void selectionSort(float * a, int n)
{
    /* a[0] to a[n-1] is the array to sort */
    int i,j;
    int iMin;

    /* advance the position through the entire array */
    /*   (could do j < n-1 because single element is also min element) */
    for (j = 0; j < n-1; j++)
    {
        /* find the min element in the unsorted a[j .. n-1] */

        /* assume the min is the first element */
        iMin = j;
        /* test against elements after j to find the smallest */
        for ( i = j+1; i < n; i++)
        {
            /* if this element is less, then it is the new minimum */
            if (a[i] < a[iMin])
            {
                /* found new minimum; remember its index */
                iMin = i;
            }
        }

        /* iMin is the index of the minimum element. Swap it with the current position */
        if ( iMin != j )
        {
            float tmp = a[j];
            a[j] = a[iMin];
            a[iMin] = tmp;
        }
    }
}

//        float average = 0;

//        for (int i = 0; i < dim.z; i++)
//        {
//            average += samples[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x];
//        }
//        average /= (float)(dim.z);


//        float variance = 0;
//        for (int i = 0; i < dim.z; i++)
//        {
//            variance += pow(samples[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x] - variance, 2.0f);
//        }
//        variance /= (float)(dim.z);

//        float std_dev = sqrt(variance);

//        float skewness;

//        for (int i = 0; i < dim.z; i++)
//        {
//            skewness = pow((samples[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x] - average)/std_dev, 3.0f);

//            if (skewness >= 1) interpol[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x] = 0.0f;
//            else interpol[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x] = samples[dim.x*dim.y*i +  dim.x*id_glb.y + id_glb.x];
//        }
