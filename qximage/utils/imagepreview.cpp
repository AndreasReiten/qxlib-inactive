#include "imagepreview.h"
#include <QBrush>
#include <QRect>
#include <QColor>
#include <QDateTime>
#include <QCoreApplication>
#include <QFontMetrics>



ImagePreviewWorker::ImagePreviewWorker(QObject *parent) :
    isImageTexInitialized(false),
    isTsfTexInitialized(false),
    isCLInitialized(false),
    isFrameValid(false),
    isWeightCenterActive(false),
    isInterpolGpuInitialized(false),
//    bgCorrectionMode(0),
    isBGEstimated(true),
//    isAutoBackgroundCorrectionActive(false),
    rgb_style(1),
    alpha_style(2),
//    isSelectionAlphaActive(false),
//    isSelectionBetaActive(false),
    mode(0)
{
    Q_UNUSED(parent);

    isInitialized = false;

    parameter.reserve(1,16);
    parameter[0] = 0;
    parameter[1] = 1e9;
    parameter[2] = 0;
    parameter[3] = 1e9;
    parameter[12] = 0;
    parameter[13] = 1000;

    texture_view_matrix.setIdentity(4);
    translation_matrix.setIdentity(4);
    zoom_matrix.setIdentity(4);

    texel_view_matrix.setIdentity(4);
    texel_offset_matrix.setIdentity(4);
    
    image_tex_size.set(1,2,0);
    image_buffer_size.set(1,2,2);
    
    prev_pixel.set(1,2,0);

    QLibrary myLib("OpenCL");

    QOpenCLGetPlatformIDs = (PROTOTYPE_QOpenCLGetPlatformIDs) myLib.resolve("clGetPlatformIDs");
    if (!QOpenCLGetPlatformIDs) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetDeviceIDs = (PROTOTYPE_QOpenCLGetDeviceIDs) myLib.resolve("clGetDeviceIDs");
    if (!QOpenCLGetDeviceIDs) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetPlatformInfo = (PROTOTYPE_QOpenCLGetPlatformInfo) myLib.resolve("clGetPlatformInfo");
    if (!QOpenCLGetPlatformInfo) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetDeviceInfo = (PROTOTYPE_QOpenCLGetDeviceInfo) myLib.resolve("clGetDeviceInfo");
    if (!QOpenCLGetDeviceInfo) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateProgramWithSource = (PROTOTYPE_QOpenCLCreateProgramWithSource) myLib.resolve("clCreateProgramWithSource");
    if (!QOpenCLCreateProgramWithSource) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLGetProgramBuildInfo = (PROTOTYPE_QOpenCLGetProgramBuildInfo) myLib.resolve("clGetProgramBuildInfo");
    if (!QOpenCLGetProgramBuildInfo) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateContext = (PROTOTYPE_QOpenCLCreateContext) myLib.resolve("clCreateContext");
    if (!QOpenCLCreateContext) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateCommandQueue = (PROTOTYPE_QOpenCLCreateCommandQueue) myLib.resolve("clCreateCommandQueue");
    if (!QOpenCLCreateCommandQueue) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLSetKernelArg = (PROTOTYPE_QOpenCLSetKernelArg) myLib.resolve("clSetKernelArg");
    if (!QOpenCLSetKernelArg) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueNDRangeKernel = (PROTOTYPE_QOpenCLEnqueueNDRangeKernel) myLib.resolve("clEnqueueNDRangeKernel");
    if (!QOpenCLEnqueueNDRangeKernel) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLFinish= (PROTOTYPE_QOpenCLFinish) myLib.resolve("clFinish");
    if (!QOpenCLFinish) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueReleaseGLObjects = (PROTOTYPE_QOpenCLEnqueueReleaseGLObjects) myLib.resolve("clEnqueueReleaseGLObjects");
    if (!QOpenCLEnqueueReleaseGLObjects) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueAcquireGLObjects = (PROTOTYPE_QOpenCLEnqueueAcquireGLObjects) myLib.resolve("clEnqueueAcquireGLObjects");
    if (!QOpenCLEnqueueAcquireGLObjects) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateKernel = (PROTOTYPE_QOpenCLCreateKernel) myLib.resolve("clCreateKernel");
    if (!QOpenCLCreateKernel) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueReadBuffer = (PROTOTYPE_QOpenCLEnqueueReadBuffer) myLib.resolve("clEnqueueReadBuffer");
    if (!QOpenCLEnqueueReadBuffer) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateBuffer = (PROTOTYPE_QOpenCLCreateBuffer) myLib.resolve("clCreateBuffer");
    if (!QOpenCLCreateBuffer) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLReleaseMemObject = (PROTOTYPE_QOpenCLReleaseMemObject) myLib.resolve("clReleaseMemObject");
    if (!QOpenCLReleaseMemObject) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateFromGLTexture2D = (PROTOTYPE_QOpenCLCreateFromGLTexture2D) myLib.resolve("clCreateFromGLTexture2D");
    if (!QOpenCLCreateFromGLTexture2D) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateSampler = (PROTOTYPE_QOpenCLCreateSampler) myLib.resolve("clCreateSampler");
    if (!QOpenCLCreateSampler) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueWriteBuffer = (PROTOTYPE_QOpenCLEnqueueWriteBuffer) myLib.resolve("clEnqueueWriteBuffer");
    if (!QOpenCLEnqueueWriteBuffer) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLReleaseKernel = (PROTOTYPE_QOpenCLReleaseKernel) myLib.resolve("clReleaseKernel");
    if (!QOpenCLReleaseKernel) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateImage2D = (PROTOTYPE_QOpenCLCreateImage2D) myLib.resolve("clCreateImage2D");
    if (!QOpenCLCreateImage2D) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLCreateImage3D = (PROTOTYPE_QOpenCLCreateImage3D) myLib.resolve("clCreateImage3D");
    if (!QOpenCLCreateImage3D) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueReadImage = (PROTOTYPE_QOpenCLEnqueueReadImage) myLib.resolve("clEnqueueReadImage");
    if (!QOpenCLEnqueueReadImage) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLReleaseSampler = (PROTOTYPE_QOpenCLReleaseSampler) myLib.resolve("clReleaseSampler");
    if (!QOpenCLReleaseSampler) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());

    QOpenCLEnqueueCopyBufferToImage  = (PROTOTYPE_QOpenCLEnqueueCopyBufferToImage ) myLib.resolve("clEnqueueCopyBufferToImage");
    if (!QOpenCLEnqueueCopyBufferToImage ) qFatal(QString("Failed to resolve function:"+myLib.errorString()).toStdString().c_str());


}

ImagePreviewWorker::~ImagePreviewWorker()
{
    glDeleteBuffers(5, selections_vbo);
    glDeleteBuffers(5, weightpoints_vbo);
}

void ImagePreviewWorker::setSharedWindow(SharedContextWindow * window)
{
    this->shared_window = window;
}

void ImagePreviewWorker::imageCalcuclus(cl_mem data_buf_cl, cl_mem out_buf_cl, Matrix<float> & param, Matrix<size_t> &image_size, Matrix<size_t> & local_ws, float mean, float deviation, int task)
{
    // Prepare kernel parameters
    Matrix<size_t> global_ws(1,2);

    global_ws[0] = image_size[0] + (local_ws[0] - ((size_t) image_size[0])%local_ws[0]);
    global_ws[1] = image_size[1] + (local_ws[1] - ((size_t) image_size[1])%local_ws[1]);
    
    // Set kernel parameters
    err =   QOpenCLSetKernelArg(cl_image_calculus,  0, sizeof(cl_mem), (void *) &data_buf_cl);
    err |=   QOpenCLSetKernelArg(cl_image_calculus, 1, sizeof(cl_mem), (void *) &out_buf_cl);
    err |=   QOpenCLSetKernelArg(cl_image_calculus, 2, sizeof(cl_mem), &parameter_cl);
    err |=   QOpenCLSetKernelArg(cl_image_calculus, 3, sizeof(cl_int2), image_size.toInt().data());
    err |=   QOpenCLSetKernelArg(cl_image_calculus, 4, sizeof(cl_int), &isLorentzCorrected);
    err |=   QOpenCLSetKernelArg(cl_image_calculus, 5, sizeof(cl_int), &task);
    err |=   QOpenCLSetKernelArg(cl_image_calculus, 6, sizeof(cl_float), &mean);
    err |=   QOpenCLSetKernelArg(cl_image_calculus, 7, sizeof(cl_float), &deviation);
//    err |=   QOpenCLSetKernelArg(cl_image_calculus, 8, sizeof(cl_mem), (void *) &series_interpol_gpu_3Dimg);
//    err |=   QOpenCLSetKernelArg(cl_image_calculus, 9, sizeof(cl_sampler), &bg_sampler);
//    err |=   QOpenCLSetKernelArg(cl_image_calculus, 10, sizeof(cl_int), &bg_sample_interdist);
//    int image_number = p_set.current()->i();
//    err |=   QOpenCLSetKernelArg(cl_image_calculus, 11, sizeof(cl_int), &image_number);
//    err |=   QOpenCLSetKernelArg(cl_image_calculus, 12, sizeof(cl_int), &isBackgroundCorrected);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    
    // Launch the kernel
    err =   QOpenCLEnqueueNDRangeKernel(context_cl->queue(), cl_image_calculus, 2, NULL, global_ws.data(), local_ws.data(), 0, NULL, NULL);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    err =   QOpenCLFinish(context_cl->queue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void ImagePreviewWorker::imageDisplay(cl_mem data_buf_cl, cl_mem frame_image_cl, cl_mem tsf_image_cl, Matrix<float> &data_limit, Matrix<size_t> &image_size, Matrix<size_t> & local_ws, cl_sampler tsf_sampler, int log)
{
    /*
     * Display an image buffer object, matching intensity to color
     * */
    
    if (!isFrameValid) return;
        
    // Aquire shared CL/GL objects
    context_gl->makeCurrent(render_surface);
    
    glFinish();

    err =  QOpenCLEnqueueAcquireGLObjects(context_cl->queue(), 1, &frame_image_cl, 0, 0, 0);
    err |=  QOpenCLEnqueueAcquireGLObjects(context_cl->queue(), 1, &tsf_image_cl, 0, 0, 0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Prepare kernel parameters
    Matrix<size_t> global_ws(1,2);
    global_ws[0] = image_size[0] + (local_ws[0] - ((size_t) image_size[0])%local_ws[0]);
    global_ws[1] = image_size[1] + (local_ws[1] - ((size_t) image_size[1])%local_ws[1]);
    
    // Set kernel parameters
    err =   QOpenCLSetKernelArg(cl_display_image,  0, sizeof(cl_mem), (void *) &data_buf_cl);
    err |=   QOpenCLSetKernelArg(cl_display_image, 1, sizeof(cl_mem), (void *) &frame_image_cl);
    err |=   QOpenCLSetKernelArg(cl_display_image, 2, sizeof(cl_mem), (void *) &tsf_image_cl);
    err |=   QOpenCLSetKernelArg(cl_display_image, 3, sizeof(cl_sampler), &tsf_sampler);
    err |=   QOpenCLSetKernelArg(cl_display_image, 4, sizeof(cl_float2), data_limit.data());
    err |=   QOpenCLSetKernelArg(cl_display_image, 5, sizeof(cl_int), &log);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    
    // Launch the kernel
    err =   QOpenCLEnqueueNDRangeKernel(context_cl->queue(), cl_display_image, 2, NULL, global_ws.data(), local_ws.data(), 0, NULL, NULL);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    err =   QOpenCLFinish(context_cl->queue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    // Release shared CL/GL objects
    err =  QOpenCLEnqueueReleaseGLObjects(context_cl->queue(), 1, &frame_image_cl, 0, 0, 0);
    err |=  QOpenCLEnqueueReleaseGLObjects(context_cl->queue(), 1, &tsf_image_cl, 0, 0, 0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void ImagePreviewWorker::copyBufferRect(cl_mem buffer_cl, 
        cl_mem copy_cl, 
        Matrix<size_t> &buffer_size,
        Matrix<size_t> &buffer_origin,
        Matrix<size_t> &copy_size,
        Matrix<size_t> &copy_origin,
        Matrix<size_t> &local_ws)
{
    // Prepare kernel parameters
    Matrix<size_t> global_ws(1,2);
    global_ws[0] = copy_size[0] + (local_ws[0] - ((size_t) copy_size[0])%local_ws[0]);
    global_ws[1] = copy_size[1] + (local_ws[1] - ((size_t) copy_size[1])%local_ws[1]);
    
    int buffer_row_pitch = buffer_size[0];
    int copy_row_pitch = copy_size[0];
    
    // Set kernel parameters
    err =   QOpenCLSetKernelArg(context_cl->cl_rect_copy_float,  0, sizeof(cl_mem), (void *) &buffer_cl);
    err |=   QOpenCLSetKernelArg(context_cl->cl_rect_copy_float, 1, sizeof(cl_int2), buffer_size.toInt().data());
    err |=   QOpenCLSetKernelArg(context_cl->cl_rect_copy_float, 2, sizeof(cl_int2), buffer_origin.toInt().data());
    err |=   QOpenCLSetKernelArg(context_cl->cl_rect_copy_float, 3, sizeof(int), &buffer_row_pitch);
    err |=   QOpenCLSetKernelArg(context_cl->cl_rect_copy_float, 4, sizeof(cl_mem), (void *) &copy_cl);
    err |=   QOpenCLSetKernelArg(context_cl->cl_rect_copy_float, 5, sizeof(cl_int2), copy_size.toInt().data());
    err |=   QOpenCLSetKernelArg(context_cl->cl_rect_copy_float, 6, sizeof(cl_int2), copy_origin.toInt().data());
    err |=   QOpenCLSetKernelArg(context_cl->cl_rect_copy_float, 7, sizeof(int), &copy_row_pitch);
    err |=   QOpenCLSetKernelArg(context_cl->cl_rect_copy_float, 8, sizeof(cl_int2), copy_size.toInt().data());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    
    // Launch the kernel
    err =   QOpenCLEnqueueNDRangeKernel(context_cl->queue(), context_cl->cl_rect_copy_float, 2, NULL, global_ws.data(), local_ws.data(), 0, NULL, NULL);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    err =   QOpenCLFinish(context_cl->queue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

float ImagePreviewWorker::sumGpuArray(cl_mem cl_data, unsigned int read_size, Matrix<size_t> &local_ws)
{
    /* 
     * This function for parallel reduction does its work over multiple passes. First pass 
     * sums the data in blocks corresponding to the local size (work group size). The 
     * results from each block is written to an extension of the array, or padding. The 
     * next pass sums the data (again in blocks) in the padded section, and writes it to 
     * the beginning of the buffer. This back and forth cycle is repeated until the buffer 
     * is reduced to a single value, the sum.
     * */
    
    /* Set initial kernel parameters (they will change for each iteration)*/
    Matrix<size_t> global_ws(1,1);
    unsigned int read_offset = 0;
    unsigned int write_offset;

    global_ws[0] = read_size + (read_size % local_ws[0] ? local_ws[0] - (read_size % local_ws[0]) : 0);
    write_offset = global_ws[0];
    
    bool forth = true;
    float sum;

    /* Pass arguments to kernel */
    err =   QOpenCLSetKernelArg(context_cl->cl_parallel_reduction, 0, sizeof(cl_mem), (void *) &cl_data);
    err |=   QOpenCLSetKernelArg(context_cl->cl_parallel_reduction, 1, local_ws[0]*sizeof(cl_float), NULL);
    err |=   QOpenCLSetKernelArg(context_cl->cl_parallel_reduction, 2, sizeof(cl_uint), &read_size);
    err |=   QOpenCLSetKernelArg(context_cl->cl_parallel_reduction, 3, sizeof(cl_uint), &read_offset);
    err |=   QOpenCLSetKernelArg(context_cl->cl_parallel_reduction, 4, sizeof(cl_uint), &write_offset);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    /* Launch kernel repeatedly until the summing is done */
    while (read_size > 1)
    {
        err =   QOpenCLEnqueueNDRangeKernel(context_cl->queue(), context_cl->cl_parallel_reduction, 1, 0, global_ws.data(), local_ws.data(), 0, NULL, NULL);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        err =   QOpenCLFinish(context_cl->queue());
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        /* Extract the sum */
        err =   QOpenCLEnqueueReadBuffer ( context_cl->queue(),
            cl_data,
            CL_TRUE,
            forth ? global_ws[0]*sizeof(cl_float) : 0,
            sizeof(cl_float),
            &sum,
            0, NULL, NULL);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        /* Prepare the kernel parameters for the next iteration */
        forth = !forth;

        // Prepare to read memory in front of the separator and write to the memory behind it
        if (forth)
        {
            read_size = (global_ws[0])/local_ws[0];
            if (read_size % local_ws[0]) global_ws[0] = read_size + local_ws[0] - (read_size % local_ws[0]);
            else global_ws[0] = read_size;

            read_offset = 0;
            write_offset = global_ws[0];
        }
        // Prepare to read memory behind the separator and write to the memory in front of it
        else
        {
            read_offset = global_ws[0];
            write_offset = 0;

            read_size = global_ws[0]/local_ws[0];
            if (read_size % local_ws[0]) global_ws[0] = read_size + local_ws[0] - (read_size % local_ws[0]);
            else global_ws[0] = read_size;
        }

        err =   QOpenCLSetKernelArg(context_cl->cl_parallel_reduction, 2, sizeof(cl_uint), &read_size);
        err |=   QOpenCLSetKernelArg(context_cl->cl_parallel_reduction, 3, sizeof(cl_uint), &read_offset);
        err |=   QOpenCLSetKernelArg(context_cl->cl_parallel_reduction, 4, sizeof(cl_uint), &write_offset);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    }

    return sum;
}

void ImagePreviewWorker::calculus()
{
    /*
     * Carry out calculations on an image buffer, such as corrections and calculation of variance and skewness 
     * */
    if (!isFrameValid) return;

    Matrix<size_t> origin(2,1,0);
    
    Matrix<size_t> local_ws(1,2);
    local_ws[0] = 64;
    local_ws[1] = 1;
    
    Matrix<size_t> image_size(1,2);
    image_size[0] = frame.getFastDimension();
    image_size[1] = frame.getSlowDimension();

    if (mode == 0)
    {
        // Normal intensity
        {
            imageCalcuclus(image_data_raw_cl, image_data_corrected_cl, parameter, image_size, local_ws, 0, 0, 0);
//            imageCalcuclus(image_data_raw_cl, image_data_max_cl, parameter, image_size, local_ws, 0, 0, -1);
            
            // Calculate the weighted intensity position
            imageCalcuclus(image_data_corrected_cl, image_data_weight_x_cl, parameter, image_size, local_ws, 0, 0, 3);
            imageCalcuclus(image_data_corrected_cl, image_data_weight_y_cl, parameter, image_size, local_ws, 0, 0, 4);
        }

    }
    if (mode == 1)
    {
        // Variance
        {
            imageCalcuclus(image_data_raw_cl, image_data_corrected_cl, parameter, image_size, local_ws, 0, 0, 0);
//            imageCalcuclus(image_data_raw_cl, image_data_max_cl, parameter, image_size, local_ws, 0, 0, -1);
            
            // Calculate the variance
            copyBufferRect(image_data_corrected_cl, image_data_generic_cl, image_size, origin, image_size, origin, local_ws);
            
            float mean = sumGpuArray(image_data_generic_cl, image_size[0]*image_size[1], local_ws)/(image_size[0]*image_size[1]);
            
            imageCalcuclus(image_data_corrected_cl, image_data_variance_cl, parameter, image_size, local_ws, mean, 0, 1);
            
            // Calculate the weighted intensity position
            imageCalcuclus(image_data_variance_cl, image_data_weight_x_cl, parameter, image_size, local_ws, 0, 0, 3);
            imageCalcuclus(image_data_variance_cl, image_data_weight_y_cl, parameter, image_size, local_ws, 0, 0, 4);
        }
    }
    else if (mode == 2)
    {
        // Skewness
        {
            imageCalcuclus(image_data_raw_cl, image_data_corrected_cl, parameter, image_size, local_ws, 0, 0, 0);
//            imageCalcuclus(image_data_raw_cl, image_data_max_cl, parameter, image_size, local_ws, 0, 0, -1);
            
            // Calculate the variance
            copyBufferRect(image_data_corrected_cl, image_data_generic_cl, image_size, origin, image_size, origin, local_ws);
            
            float mean = sumGpuArray(image_data_generic_cl, image_size[0]*image_size[1], local_ws)/(image_size[0]*image_size[1]);
            
            imageCalcuclus(image_data_corrected_cl, image_data_variance_cl, parameter, image_size, local_ws, mean, 0, 1);
            
            // Calculate the skewness
            copyBufferRect(image_data_variance_cl, image_data_generic_cl, image_size, origin, image_size, origin, local_ws);
            
            float variance = sumGpuArray(image_data_generic_cl, image_size[0]*image_size[1], local_ws)/(image_size[0]*image_size[1]);
            
            imageCalcuclus(image_data_variance_cl, image_data_skewness_cl, parameter, image_size, local_ws, mean, sqrt(variance), 2);
            
            
            // Calculate the weighted intensity position
            imageCalcuclus(image_data_skewness_cl, image_data_weight_x_cl, parameter, image_size, local_ws, 0, 0, 3);
            imageCalcuclus(image_data_skewness_cl, image_data_weight_y_cl, parameter, image_size, local_ws, 0, 0, 4);
        }
    }
    else
    {
        // Should not happen
    }
}

void ImagePreviewWorker::setFrame()
{
    // Set the frame
//    frame_image = image;
    if (!frame.set(p_set.current()->current()->path())) return;
    if(!frame.readData()) return;

    isFrameValid = true;

//    frame.setNaive();
    
    Selection analysis_area = p_set.current()->current()->selection();

    // Restrict selection, this could be moved elsewhere and it would look better
    if (analysis_area.left() < 0) analysis_area.setLeft(0);
    if (analysis_area.right() >= frame.getFastDimension()) analysis_area.setRight(frame.getFastDimension()-1);
    if (analysis_area.top() < 0) analysis_area.setTop(0);
    if (analysis_area.bottom() >= frame.getSlowDimension()) analysis_area.setBottom(frame.getSlowDimension()-1);

//    if (background_area.left() < 0) background_area.setLeft(0);
//    if (background_area.right() >= frame.getFastDimension()) background_area.setRight(frame.getFastDimension()-1);
//    if (background_area.top() < 0) background_area.setTop(0);
//    if (background_area.bottom() >= frame.getSlowDimension()) background_area.setBottom(frame.getSlowDimension()-1);

    p_set.current()->current()->setSelection(analysis_area);

    Matrix<size_t> image_size(1,2);
    image_size[0] = frame.getFastDimension();
    image_size[1] = frame.getSlowDimension();
    
    clMaintainImageBuffers(image_size);

    // Write the frame data to the GPU
    err =  QOpenCLEnqueueWriteBuffer(
                context_cl->queue(),
                image_data_raw_cl,
                CL_TRUE, 
                0, 
                frame.data().bytes(), 
                frame.data().data(),
                0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    // Parameters essential to the frame
    parameter[4] = frame.getFlux();
    parameter[5] = frame.getExpTime();
    parameter[6] = frame.getWavelength();
    parameter[7] = frame.getDetectorDist();
    parameter[8] = frame.getBeamX();
    parameter[9] = frame.getBeamY();
    parameter[10] = frame.getPixSizeX();
    parameter[11] = frame.getPixSizeY();
    
    setParameter(parameter);
    
    // Do relevant calculations and render
//    refreshBackground(&background_area);
    
    calculus();
    refreshDisplay();
    refreshSelection(&analysis_area);
    
    p_set.current()->current()->setSelection(analysis_area);
//    p_set.current()->current()->setBackground(background_area);
    
    // Emit the image instead of components
//    emit imageChanged(frame_image);
    emit pathChanged(p_set.current()->current()->path());

}



void ImagePreviewWorker::clMaintainImageBuffers(Matrix<size_t> &image_size)
{
    if ((image_size[0] != image_buffer_size[0]) || (image_size[1] != image_buffer_size[1]))
    {
        err =  QOpenCLReleaseMemObject(image_data_raw_cl);
        err |=  QOpenCLReleaseMemObject(image_data_max_cl);
        err |=  QOpenCLReleaseMemObject(image_data_corrected_cl);
        err |=  QOpenCLReleaseMemObject(image_data_weight_x_cl);
        err |=  QOpenCLReleaseMemObject(image_data_weight_y_cl);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
        image_data_raw_cl =  QOpenCLCreateBuffer( context_cl->context(),
            CL_MEM_ALLOC_HOST_PTR,
            image_size[0]*image_size[1]*sizeof(cl_float),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        image_data_max_cl =  QOpenCLCreateBuffer( context_cl->context(),
            CL_MEM_ALLOC_HOST_PTR,
            image_size[0]*image_size[1]*sizeof(cl_float),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
        image_data_corrected_cl =  QOpenCLCreateBuffer( context_cl->context(),
            CL_MEM_ALLOC_HOST_PTR,
            image_size[0]*image_size[1]*sizeof(cl_float),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
        image_data_variance_cl =  QOpenCLCreateBuffer( context_cl->context(),
            CL_MEM_ALLOC_HOST_PTR,
            image_size[0]*image_size[1]*sizeof(cl_float),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
        image_data_skewness_cl =  QOpenCLCreateBuffer( context_cl->context(),
            CL_MEM_ALLOC_HOST_PTR,
            image_size[0]*image_size[1]*sizeof(cl_float),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
        image_data_weight_x_cl =  QOpenCLCreateBuffer( context_cl->context(),
            CL_MEM_ALLOC_HOST_PTR,
            image_size[0]*image_size[1]*sizeof(cl_float),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
        image_data_weight_y_cl =  QOpenCLCreateBuffer( context_cl->context(),
            CL_MEM_ALLOC_HOST_PTR,
            image_size[0]*image_size[1]*sizeof(cl_float),
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
        image_data_generic_cl =  QOpenCLCreateBuffer( context_cl->context(),
            CL_MEM_ALLOC_HOST_PTR,
            image_size[0]*image_size[1]*sizeof(cl_float)*2, // *2 so it can be used for parallel reduction
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
        image_buffer_size[0] = image_size[0];
        image_buffer_size[1] = image_size[1];
    }
}

void ImagePreviewWorker::refreshSelection(Selection * area)
{
    Matrix<size_t> local_ws(1,2);
    local_ws[0] = 64;
    local_ws[1] = 1;
    
    Matrix<size_t> image_size(1,2);
    image_size[0] = frame.getFastDimension();
    image_size[1] = frame.getSlowDimension();
    
//    if(area->width() > frame.getFastDimension()) area->setWidth(frame.getFastDimension());
//    if(area->height() > frame.getSlowDimension()) area->setHeight(frame.getSlowDimension();

    if (mode == 0)
    {
        // Normal intensity
        selectionCalculus(area, image_data_corrected_cl, image_data_weight_x_cl, image_data_weight_y_cl, image_size, local_ws);
    }
    if (mode == 1)
    {
        // Variance
        selectionCalculus(area, image_data_variance_cl, image_data_weight_x_cl, image_data_weight_y_cl, image_size, local_ws);
    }
    else if (mode == 2)
    {
        // Skewness
        selectionCalculus(area, image_data_skewness_cl, image_data_weight_x_cl, image_data_weight_y_cl, image_size, local_ws);
    }
    else
    {
        // Should not happen
    }
    

}

//void ImagePreviewWorker::refreshBackground(Selection * area)
//{
//    Matrix<size_t> local_ws(1,2);
//    local_ws[0] = 64;
//    local_ws[1] = 1;
    
//    Matrix<size_t> image_size(1,2);
//    image_size[0] = frame.getFastDimension();
//    image_size[1] = frame.getSlowDimension();

//    // Normal intensity
//    selectionCalculus(area, image_data_raw_cl, image_data_weight_x_cl, image_data_weight_y_cl, image_size, local_ws);
    
    
//    if (isAutoBackgroundCorrectionActive)
//    {
//        double noise = area->integral()/(double)(area->width()*area->height());
//        parameter[0] = noise;
//        setParameter(parameter);
//        emit noiseLowChanged(noise);
//    }
//}

void ImagePreviewWorker::refreshDisplay()
{
    /*
     * Refresh the image buffer 
     * */
    
    Matrix<size_t> local_ws(1,2);
    local_ws[0] = 8;
    local_ws[1] = 8;
    
    Matrix<size_t> image_size(1,2);
    image_size[0] = frame.getFastDimension();
    image_size[1] = frame.getSlowDimension();
    
    Matrix<float> data_limit(1,2);
    data_limit[0] = parameter[12];
    data_limit[1] = parameter[13];
    
    if (isFrameValid) maintainImageTexture(image_size);
    
    if (mode == 0)
    {
        // Normal intensity
        imageDisplay(image_data_corrected_cl, image_tex_cl, tsf_tex_cl, data_limit, image_size, local_ws, tsf_sampler, isLog);
        imageDisplay(image_data_max_cl, max_tex_cl, tsf_tex_cl, data_limit, image_size, local_ws, tsf_sampler, isLog);
    }
    if (mode == 1)
    {
        // Variance
        imageDisplay(image_data_variance_cl, image_tex_cl, tsf_tex_cl, data_limit, image_size, local_ws, tsf_sampler, isLog);
        imageDisplay(image_data_max_cl, max_tex_cl, tsf_tex_cl, data_limit, image_size, local_ws, tsf_sampler, isLog);
    }
    else if (mode == 2)
    {
        // Skewness
        imageDisplay(image_data_skewness_cl, image_tex_cl, tsf_tex_cl, data_limit, image_size, local_ws, tsf_sampler, isLog);
        imageDisplay(image_data_max_cl, max_tex_cl, tsf_tex_cl, data_limit, image_size, local_ws, tsf_sampler, isLog);
    }
    else
    {
        // Should not happen
    }
}

void ImagePreviewWorker::maintainImageTexture(Matrix<size_t> &image_size)
{
    if ((image_size[0] != image_tex_size[0]) || (image_size[1] != image_tex_size[1]) || !isImageTexInitialized)
    {
        if (isImageTexInitialized)
        {
            err =  QOpenCLReleaseMemObject(image_tex_cl);
            err |=  QOpenCLReleaseMemObject(max_tex_cl);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
            glDeleteTextures(1, &image_tex_gl);
            glDeleteTextures(1, &max_tex_gl);
        }
        
        context_gl->makeCurrent(render_surface);
        
        glGenTextures(1, &image_tex_gl);
        glBindTexture(GL_TEXTURE_2D, image_tex_gl);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA32F,
            image_size[0],
            image_size[1],
            0,
            GL_RGBA,
            GL_FLOAT,
            NULL);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenTextures(1, &max_tex_gl);
        glBindTexture(GL_TEXTURE_2D, max_tex_gl);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA32F,
            image_size[0],
            image_size[1],
            0,
            GL_RGBA,
            GL_FLOAT,
            NULL);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        image_tex_size[0] = image_size[0];
        image_tex_size[1] = image_size[1];
        
        // Share the texture with the OpenCL runtime
        image_tex_cl =  QOpenCLCreateFromGLTexture2D(context_cl->context(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, image_tex_gl, &err);
        max_tex_cl =  QOpenCLCreateFromGLTexture2D(context_cl->context(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, max_tex_gl, &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
        isImageTexInitialized = true;
    }
}

QString ImagePreviewWorker::integrationFrameString(DetectorFile &f, ImageInfo & image)
{
    Matrix<double> Q = getScatteringVector(f, image.selection().weighted_x(), image.selection().weighted_y());
    double value = 180*getScatteringAngle(f, image.selection().weighted_x(), image.selection().weighted_y())/pi;
    
    QString str;
    str += QString::number(image.selection().integral(),'E')+" "
            +QString::number(image.selection().left())+" "
            +QString::number(image.selection().top())+" "
            +QString::number(image.selection().width())+" "
            +QString::number(image.selection().height())+" "
            +QString::number(image.selection().weighted_x(),'E')+" "
            +QString::number(image.selection().weighted_y(),'E')+" "
            +QString::number(Q[0],'E')+" "
            +QString::number(Q[1],'E')+" "
            +QString::number(Q[2],'E')+" "
            +QString::number(vecLength(Q),'E')+" "
            +QString::number(value,'E')+" "
            +QString::number(image.background().integral()/(image.background().width()*image.background().height()),'E')+" "
            +QString::number(image.background().left())+" "
            +QString::number(image.background().top())+" "
            +QString::number(image.background().width())+" "
            +QString::number(image.background().height())+" "
            +image.path()+"\n";
    return str;
}


void ImagePreviewWorker::peakHuntSingle(ImageInfo image)
{
}

void ImagePreviewWorker::peakHuntFolder(ImageSeries series)
{
}

void ImagePreviewWorker::peakHuntSet(SeriesSet set)
{
}

void ImagePreviewWorker::analyzeSingle()
{
    // Draw the frame and update the intensity OpenCL buffer prior to further operations 
    setFrame();
    {
        QPainter painter(paint_device_gl);
        render(&painter);
    }
    
    // Force a buffer swap
    context_gl->swapBuffers(render_surface);

    QString result;
    result += "# Analysis of single frame\n";
    result += "# "+QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm:ss t")+"\n";
    result += "#\n";
    result += "# (integral, origin x, origin y, width, height, weight x, weight y, Qx, Qy, Qz, |Q|, 2theta, background, origin x, origin y, width, height, path)\n";

    result += integrationFrameString(frame, *p_set.current()->current());
    emit resultFinished(result);
}

void ImagePreviewWorker::analyzeFolder()
{
    double integral = 0;
    Matrix<double> weightpoint(1,3,0);
    
    QString frames;        
    for (int i = 0; i < p_set.current()->size(); i++)
    {
        // Draw the frame and update the intensity OpenCL buffer prior to further operations 
        setFrame();
        {
            QPainter painter(paint_device_gl);
            render(&painter);
        }
        // Force a buffer swap
        context_gl->swapBuffers(render_surface);

        // Math
        Matrix<double> Q = getScatteringVector(frame, p_set.current()->current()->selection().weighted_x(), p_set.current()->current()->selection().weighted_y());
        
        weightpoint += p_set.current()->current()->selection().integral()*Q;
        
        
        integral += p_set.current()->current()->selection().integral();
        
        frames += integrationFrameString(frame, *p_set.current()->current());
    
        p_set.current()->next();
    }
    
    if (integral > 0)
    {
        weightpoint = weightpoint / integral;
    }
    else
    {
        weightpoint.set(1,3,0);
    }
    
    QString result;
    result += "# Analysis of frames in series "+p_set.current()->path()+"\n";
    result += "# "+QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm:ss t")+"\n";
    result += "#\n";
    result += "# Sum of total integrated area in series "+QString::number(integral,'E')+"\n";
    result += "# Weightpoint xyz "+QString::number(weightpoint[0],'E')+" "+QString::number(weightpoint[1],'E')+" "+QString::number(weightpoint[2],'E')+" "+QString::number(vecLength(weightpoint),'E')+"\n";
    result += "# Analysis of the individual frames (integral, origin x, origin y, width, height, weight x, weight y, Qx, Qy, Qz, |Q|, 2theta, background, origin x, origin y, width, height, path)\n";
    result += frames;
    
    emit resultFinished(result);
}

void ImagePreviewWorker::applySelectionToSeriesSet()
{
    if (p_set.size() > 0)
    {
        Selection selection = p_set.current()->current()->selection();
        Selection background = p_set.current()->current()->background();
        
        p_set.saveCurrentIndex();

        p_set.begin();

        for (int i = 0; i < p_set.size(); i++)
        {        
            p_set.current()->saveCurrentIndex();
            p_set.current()->begin();
            
            for (int i = 0; i < p_set.current()->size(); i++)
            {
                p_set.current()->current()->setSelection(selection);
                p_set.current()->current()->setBackground(background);
                p_set.current()->next();
            }
            p_set.current()->loadSavedIndex();

            p_set.next();
        }
        p_set.loadSavedIndex();
    }
}

void ImagePreviewWorker::applySelectionToSeries()
{
    if (p_set.size() > 0)
    {
        Selection selection = p_set.current()->current()->selection();
        Selection background = p_set.current()->current()->background();
        
        p_set.current()->saveCurrentIndex();
        
        p_set.current()->begin();
        
        for (int i = 0; i < p_set.current()->size(); i++)
        {
            p_set.current()->current()->setSelection(selection);
            p_set.current()->current()->setBackground(background);
            p_set.current()->next();
        }
        
        p_set.current()->loadSavedIndex();
    }
}


void ImagePreviewWorker::setSet(SeriesSet s)
{
    
    if (!s.isEmpty())
    {
        p_set = s;
        
        emit imageRangeChanged(0,p_set.current()->size()-1);
        setFrame();
        

//        emit selectionChanged(p_set.current()->current()->selection());
        centerImage();

        isBGEstimated = false;
    }
}

void ImagePreviewWorker::removeCurrentImage()
{
    if (!p_set.isEmpty())
    {
        emit pathRemoved(p_set.current()->current()->path());
        
        p_set.current()->removeCurrent();
        p_set.current()->next();

        setFrame();

        isBGEstimated = false;
    }
}


void ImagePreviewWorker::setFrameByIndex(int i)
{
    if (!p_set.isEmpty())
    {
        p_set.current()->at(i);
        setFrame();
    }
}

void ImagePreviewWorker::nextSeries()
{
    if (!p_set.isEmpty())
    {
        p_set.current()->saveCurrentIndex();
        p_set.next();
        p_set.current()->loadSavedIndex();

        emit imageRangeChanged(0,p_set.current()->size()-1);
        emit currentIndexChanged(p_set.current()->i());

        setSeriesMaxFrame();
//        setSeriesBackgroundBuffer();
        setFrame();
    }
}
void ImagePreviewWorker::prevSeries()
{
    if (!p_set.isEmpty())
    {
        p_set.current()->saveCurrentIndex();
        p_set.previous();
        p_set.current()->loadSavedIndex();

        emit imageRangeChanged(0,p_set.current()->size()-1);
        emit currentIndexChanged(p_set.current()->i());

        setSeriesMaxFrame();
//        setSeriesBackgroundBuffer();
        setFrame();
    }
}

void ImagePreviewWorker::traceSeries()
{
    emit visibilityChanged(false);

    set_trace.clear();

    p_set.saveCurrentIndex();
    
    frame.set(p_set.begin()->begin()->path());
    
    // For each series
    for (int i = 0; i < p_set.size(); i++)
    {
        emit progressRangeChanged(0, p_set.current()->size());

        p_set.current()->saveCurrentIndex();
        
        Matrix<float> zeros_like_frame(frame.getSlowDimension(), frame.getFastDimension(), 0.0f);

        cl_mem max_image_gpu = QOpenCLCreateBuffer( context_cl->context(),
                CL_MEM_COPY_HOST_PTR,
                zeros_like_frame.bytes(),
                zeros_like_frame.data(),
                &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


        // For each image in the series
        for (int j = 0; j < p_set.current()->size(); j++)
        {
            // Read data and send to a VRAM buffer. 
            frame.readData();

            cl_mem image_gpu = QOpenCLCreateBuffer( context_cl->context(),
                    CL_MEM_COPY_HOST_PTR,
                    frame.data().bytes(),
                    frame.data().data(),
                    &err);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
            
            // Use OpenCL to check data against a second VRAM buffer, keeping the max value
            Matrix<size_t> global_ws(1,2);
            Matrix<size_t> local_ws(1,2);

            local_ws[0] = 16;
            local_ws[1] = 16;

            global_ws[0] = frame.getFastDimension() + local_ws[0] - frame.getFastDimension()%local_ws[0];
            global_ws[1] = frame.getSlowDimension() + local_ws[1] - frame.getSlowDimension()%local_ws[1];

            Matrix<int> image_size(1,2);
            image_size[0] = frame.getFastDimension();
            image_size[1] = frame.getSlowDimension();

            err =   QOpenCLSetKernelArg(cl_buffer_max,  0, sizeof(cl_mem), (void *) &image_gpu);
            err |=   QOpenCLSetKernelArg(cl_buffer_max, 1, sizeof(cl_mem), (void *) &max_image_gpu);
            err |=   QOpenCLSetKernelArg(cl_buffer_max, 2, sizeof(cl_int2), image_size.data());
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

            err = QOpenCLEnqueueNDRangeKernel(context_cl->queue(), cl_buffer_max, 2, NULL, global_ws.data(), local_ws.data(), 0, NULL, NULL);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

            err =  QOpenCLReleaseMemObject(image_gpu);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
            
            frame.set(p_set.current()->next()->path());
            
            emit progressChanged(j+1);
        }

        // Read back the second VRAM buffer and store in system RAM for later usage 
        Matrix<float> max_image(frame.getSlowDimension(),frame.getFastDimension());

        err =   QOpenCLEnqueueReadBuffer ( context_cl->queue(),
            max_image_gpu,
            CL_TRUE,
            0,
            max_image.bytes(),
            max_image.data(),
            0, NULL, NULL);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


        set_trace << max_image;

        err =  QOpenCLReleaseMemObject(max_image_gpu);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        p_set.current()->loadSavedIndex();
        p_set.next();
    }
    
    p_set.loadSavedIndex();

    emit visibilityChanged(true);

    setSeriesMaxFrame();
    setFrame();    
}


// A function to approximate background for the current set
//void ImagePreviewWorker::estimateBackground()
//{
//    emit visibilityChanged(false);

//    set_tools.clear();
    
//    p_set.saveCurrentIndex();
    
//    // Use the first frame as an example:
//    frame.set(p_set.begin()->begin()->path());
//    frame.readData();
    
//    bg_sample_interdist = 8;
    
//    // Prepare the storage buffer
//    size_t m = frame.getSlowDimension()/bg_sample_interdist;
//    size_t n = frame.getFastDimension()/bg_sample_interdist;
    
//    for (int i = 0; i < p_set.size(); i++)
//    {

//        emit progressRangeChanged(0, p_set.current()->size());

//        p_set.current()->saveCurrentIndex();
        
//        // Background correction: Note: It is assumed that all images a series have the same dimensions
//        SeriesToolShed tool;
        
//        // Given a set of rules for sample selection. Samples are taken on a regular, equidistant grid. Samples are taken from the entire frame.
//        Matrix<float> series_samples_cpu(1, m*n*p_set.current()->size());
//        tool.series_interpol_cpu.set(m*p_set.current()->size(), n);


//        // For each image in the series
//        for (int j = 0; j < p_set.current()->size(); j++)
//        {
//            // Move relevant samples into a separate buffer
//            for (int k = 0; k < m; k++) // Slow dimension
//            {
//                for (int l = 0; l < n; l++) // Fast dimension
//                {
//                    // Note: In a better world this memory would be aligned in according to gpu memory optimization. This is bank conflict incarnate. Easy enough to fix. For example: Pad n and m with empty values to put adjacent pixel lines in adjacent memory banks.
//                    series_samples_cpu[j*n*m+k*n+l] = frame.data()[k*bg_sample_interdist*frame.getFastDimension()+l*bg_sample_interdist];
//                }
//            }
//            frame.set(p_set.current()->next()->path());
//            frame.readData();

//            emit progressChanged(j+1);
//        }

//        // Move series storage buffer into gpu memory
//        cl_mem series_samples_gpu = QOpenCLCreateBuffer( context_cl->context(),
//                CL_MEM_COPY_HOST_PTR,
//                series_samples_cpu.bytes(),
//                series_samples_cpu.data(),
//                &err);
//        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

//        cl_mem series_interpol_gpu = QOpenCLCreateBuffer( context_cl->context(),
//                CL_MEM_ALLOC_HOST_PTR,
//                tool.series_interpol_cpu.bytes(),
//                NULL,
//                &err);
//        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));



//        // Do GPU magic on series, saving an interpolation object in gpu memory
//        Matrix<int> dim(1,3);
//        dim[0] = n;
//        dim[1] = m;
//        dim[2] = p_set.current()->size();
        
//        err =   QOpenCLSetKernelArg(cl_glowstick,  0, sizeof(cl_mem), (void *) &series_samples_gpu);
//        err |=   QOpenCLSetKernelArg(cl_glowstick, 1, sizeof(cl_mem), (void *) &series_interpol_gpu);
//        err |=   QOpenCLSetKernelArg(cl_glowstick, 2, sizeof(cl_int3), dim.data());
//        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

//        Matrix<size_t> global_ws(1,2);
//        Matrix<size_t> local_ws(1,2);
//        local_ws[0] = 16;
//        local_ws[1] = 16;

//        global_ws[0] = n + local_ws[0] - n%local_ws[0];
//        global_ws[1] = m + local_ws[1] - m%local_ws[1];
        
////        local_ws.print(0,"local_ws");
////        global_ws.print(0,"global_ws");

//        err = QOpenCLEnqueueNDRangeKernel(context_cl->queue(), cl_glowstick, 2, NULL, global_ws.data(), local_ws.data(), 0, NULL, NULL);
//        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

//        err = QOpenCLFinish(context_cl->queue());
//        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
//        tool.dim[0] = dim[0];
//        tool.dim[1] = dim[1];
//        tool.dim[2] = dim[2];
//        p_set.next();
        
//        // Read back relevant data
//        err =   QOpenCLEnqueueReadBuffer ( context_cl->queue(),
//            series_interpol_gpu,
//            CL_TRUE,
//            0,
//            tool.series_interpol_cpu.bytes(),
//            tool.series_interpol_cpu.data(),
//            0, NULL, NULL);
//        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
        
////        tool.series_interpol_cpu.print(1,"series interpol cpu");
        
//        set_tools << tool;
        
        
        
//        // (The 3D buffer can now be used for BG approximation in other kernels)
//        err =  QOpenCLReleaseMemObject(series_samples_gpu);
//        err |=  QOpenCLReleaseMemObject(series_interpol_gpu);
//        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        
//        p_set.current()->loadSavedIndex();
//    }
    
//    p_set.loadSavedIndex();
    

//    emit visibilityChanged(true);

//    isBGEstimated = true;

//    setSeriesBackgroundBuffer();
    
//    setFrame();
//}

void ImagePreviewWorker::setSeriesMaxFrame()
{
    qDebug() << p_set.i();

    err =  QOpenCLReleaseMemObject(image_data_max_cl);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    image_data_max_cl =  QOpenCLCreateBuffer( context_cl->context(),
        CL_MEM_COPY_HOST_PTR,
        set_trace[p_set.i()].bytes(),
        set_trace[p_set.i()].data(),
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}




//void ImagePreviewWorker::setSeriesBackgroundBuffer() // Call this function when swapping sets, and relevant dat will be put in gpu memory. This can then be used on demand by the imaging kernel
//{
//    if (isBGEstimated)
//    {
//        // Copy interpolation data over to image buffer
//        Matrix<size_t> region(1,3);
//        region[0] = set_tools[p_set.i()].dim[0];
//        region[1] = set_tools[p_set.i()].dim[1];
//        region[2] = set_tools[p_set.i()].dim[2];
        
////        region.print(0,"Region on buffer load");
        
//        cl_image_format format_3Dimg;
//        format_3Dimg.image_channel_order = CL_INTENSITY;
//        format_3Dimg.image_channel_data_type = CL_FLOAT;

//    //    if (isInterpolGpuInitialized)
//    //    {
//        err =  QOpenCLReleaseMemObject(series_interpol_gpu_3Dimg);
//        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
//    //    }

//        series_interpol_gpu_3Dimg = QOpenCLCreateImage3D ( context_cl->context(),
//            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
//            &format_3Dimg,
//            region[0],
//            region[1],
//            region[2],
//            0,
//            0,
//            set_tools[p_set.i()].series_interpol_cpu.data(),
//            &err);
//        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

//    //    isInterpolGpuInitialized = true;
////        bgCorrectionMode = 1;
//    }
//}

void ImagePreviewWorker::analyzeSet()
{
    double integral = 0;
    QStringList series_integral;
    QStringList series_weightpoint;
    QStringList series_frames;
    QString str;

    
    for (int i = 0; i < p_set.size(); i++)
    {
        Matrix<double> weightpoint(1,3,0);
        
        for (int j = 0; j < p_set.current()->size(); j++)
        {
            // Draw the frame and update the intensity OpenCL buffer prior to further operations 
            setFrame();
            {
                QPainter painter(paint_device_gl);
                render(&painter);
            }
            // Force a buffer swap
            context_gl->swapBuffers(render_surface);

            
            // Math
            Matrix<double> Q = getScatteringVector(frame, p_set.current()->current()->selection().weighted_x(), p_set.current()->current()->selection().weighted_y());
            
            weightpoint += p_set.current()->current()->selection().integral()*Q;
            
            integral += p_set.current()->current()->selection().integral();
            
            str += integrationFrameString(frame, *p_set.current()->current());
        
            p_set.current()->next(); 
        }
        
        if (integral > 0)
        {
            weightpoint = weightpoint / integral;
        }
        else
        {
            weightpoint.set(1,3,0);
        }
        
        series_weightpoint << QString::number(weightpoint[0],'E')+" "+QString::number(weightpoint[1],'E')+" "+QString::number(weightpoint[2],'E')+" "+QString::number(vecLength(weightpoint),'E')+"\n";
        
        series_integral << QString(QString::number(integral,'E')+"\n");
        integral = 0;
        
        series_frames << str;
        str.clear();
        
        p_set.next();
    }
    
    QString result;
    
    result += "# Analysis of frames in several seriess\n";
    result += "# "+QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm:ss t")+"\n";
    result += "#\n";
    result += "# Sum of total integrated area in seriess\n";
    foreach(const QString &str, series_integral)
    {
        result += str;
    }
    result += "# Weightpoints in seriess (Qx, Qy, Qz, |Q|)\n";
    foreach(const QString &str, series_weightpoint)
    {
        result += str;
    }
    
    result += "# Analysis of the individual frames for each series (integral, origin x, origin y, width, height, weight x, weight y, Qx, Qy, Qz, |Q|, 2theta, background, origin x, origin y, width, height, path)\n";
    for (int i = 0; i < series_integral.size(); i++)
    {
        result += "# Folder integral "+series_integral.at(i);
        result += series_frames.at(i);
    }
    
    emit resultFinished(result);
}


void ImagePreviewWorker::showWeightCenter(bool value)
{
    isWeightCenterActive = value;
}

void ImagePreviewWorker::selectionCalculus(Selection * area, cl_mem image_data_cl, cl_mem image_pos_weight_x_cl_new, cl_mem image_pos_weight_y_cl_new, Matrix<size_t> &image_size, Matrix<size_t> &local_ws)
{
    /*
     * When an image is processed by the imagepreview kernel, it saves data into GPU buffers that can be used 
     * for further calculations. This functions copies data from these buffers into smaller buffers depending
     * on the selected area. The buffers are then summed, effectively doing operations such as integration
     * */
    
    if (!isFrameValid) return;
    
    // Set the size of the cl buffer that will be used to store the data in the marked selection. The padded size is neccessary for the subsequent parallel reduction
    int selection_read_size = area->width()*area->height();
    int selection_local_size = local_ws[0]*local_ws[1];
    int selection_global_size = selection_read_size + (selection_read_size % selection_local_size ? selection_local_size - (selection_read_size % selection_local_size) : 0);
    int selection_padded_size = selection_global_size + selection_global_size/selection_local_size;
    
    if (selection_read_size <= 0) return;

    if (0)
    {
        qDebug() << *area;
        qDebug() << selection_read_size;
        qDebug() << selection_local_size;
        qDebug() << selection_global_size;
        qDebug() << selection_padded_size;
    }

    // Copy a chunk of GPU memory for further calculations
    local_ws[0] = 64;
    local_ws[1] = 1;
    
    // The memory area to be copied from
    Matrix<size_t> buffer_size(1,2);
    buffer_size[0] = image_size[0];
    buffer_size[1] = image_size[1];
    
    Matrix<size_t> buffer_origin(1,2);
    buffer_origin[0] = area->left();
    buffer_origin[1] = area->top();
    
    // The memory area to be copied into
    Matrix<size_t> copy_size(1,2);
    copy_size[0] = area->width();
    copy_size[1] = area->height();
    
    Matrix<size_t> copy_origin(1,2);
    copy_origin[0] = 0;
    copy_origin[1] = 0;
    
    
    // Prepare buffers to put data into that coincides with the selected area
    cl_mem selection_intensity_cl =  QOpenCLCreateBuffer( context_cl->context(),
        CL_MEM_ALLOC_HOST_PTR,
        selection_padded_size*sizeof(cl_float),
        NULL,
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    cl_mem selection_pos_weight_x_cl =  QOpenCLCreateBuffer( context_cl->context(),
        CL_MEM_ALLOC_HOST_PTR,
        selection_padded_size*sizeof(cl_float),
        NULL,
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    cl_mem selection_pos_weight_y_cl =  QOpenCLCreateBuffer( context_cl->context(),
        CL_MEM_ALLOC_HOST_PTR,
        selection_padded_size*sizeof(cl_float),
        NULL,
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    // Transfer data to above buffers
    copyBufferRect(image_data_cl, selection_intensity_cl, buffer_size, buffer_origin, copy_size, copy_origin, local_ws);
    copyBufferRect(image_pos_weight_x_cl_new, selection_pos_weight_x_cl, buffer_size, buffer_origin, copy_size, copy_origin, local_ws);
    copyBufferRect(image_pos_weight_y_cl_new, selection_pos_weight_y_cl, buffer_size, buffer_origin, copy_size, copy_origin, local_ws);

    local_ws[0] = local_ws[0]*local_ws[1];
    local_ws[1] = 1;
    
    // Do parallel reduction of the chunks and save the results
    area->setSum(sumGpuArray(selection_intensity_cl, selection_read_size, local_ws));
    
    if (area->integral() > 0)
    {
        area->setWeightedX(sumGpuArray(selection_pos_weight_x_cl, selection_read_size, local_ws)/area->integral());
        area->setWeightedY(sumGpuArray(selection_pos_weight_y_cl, selection_read_size, local_ws)/area->integral());
    }
    else
    {
        area->setWeightedX(0);
        area->setWeightedY(0);
    }
    
    err =  QOpenCLReleaseMemObject(selection_intensity_cl);
    err |=  QOpenCLReleaseMemObject(selection_pos_weight_x_cl);
    err |=  QOpenCLReleaseMemObject(selection_pos_weight_y_cl);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}


void ImagePreviewWorker::initOpenCL()
{
    // Build program from OpenCL kernel source
    QStringList paths;
    paths << "cl/image_preview.cl";
//    paths << "cl/background_filter.cl";

    program = context_cl->createProgram(paths, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    context_cl->buildProgram(&program, "-Werror");

    // Kernel handles
    cl_display_image =  QOpenCLCreateKernel(program, "imageDisplay", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_image_calculus =  QOpenCLCreateKernel(program, "imageCalculus", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_buffer_max =  QOpenCLCreateKernel(program, "bufferMax", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

//    cl_glowstick =  QOpenCLCreateKernel(program, "glowstick", &err);
//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    // Image sampler
    image_sampler =  QOpenCLCreateSampler(context_cl->context(), false, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_NEAREST, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    // Background sampler
    bg_sampler =  QOpenCLCreateSampler(context_cl->context(), false, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    

    // Tsf sampler
    tsf_sampler =  QOpenCLCreateSampler(context_cl->context(), true, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Parameters
    parameter_cl =  QOpenCLCreateBuffer(context_cl->context(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        parameter.bytes(),
        NULL, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    // Image buffers
    image_data_raw_cl =  QOpenCLCreateBuffer( context_cl->context(),
        CL_MEM_ALLOC_HOST_PTR,
        image_buffer_size[0]*image_buffer_size[1]*sizeof(cl_float),
        NULL,
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    image_data_max_cl =  QOpenCLCreateBuffer( context_cl->context(),
        CL_MEM_ALLOC_HOST_PTR,
        image_buffer_size[0]*image_buffer_size[1]*sizeof(cl_float),
        NULL,
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    image_data_corrected_cl =  QOpenCLCreateBuffer( context_cl->context(),
        CL_MEM_ALLOC_HOST_PTR,
        image_buffer_size[0]*image_buffer_size[1]*sizeof(cl_float),
        NULL,
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    image_data_variance_cl =  QOpenCLCreateBuffer( context_cl->context(),
        CL_MEM_ALLOC_HOST_PTR,
        image_buffer_size[0]*image_buffer_size[1]*sizeof(cl_float),
        NULL,
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    image_data_skewness_cl =  QOpenCLCreateBuffer( context_cl->context(),
        CL_MEM_ALLOC_HOST_PTR,
        image_buffer_size[0]*image_buffer_size[1]*sizeof(cl_float),
        NULL,
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    image_data_weight_x_cl =  QOpenCLCreateBuffer( context_cl->context(),
        CL_MEM_ALLOC_HOST_PTR,
        image_buffer_size[0]*image_buffer_size[1]*sizeof(cl_float),
        NULL,
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    image_data_weight_y_cl =  QOpenCLCreateBuffer( context_cl->context(),
        CL_MEM_ALLOC_HOST_PTR,
        image_buffer_size[0]*image_buffer_size[1]*sizeof(cl_float),
        NULL,
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    image_data_generic_cl =  QOpenCLCreateBuffer( context_cl->context(),
        CL_MEM_ALLOC_HOST_PTR,
        image_buffer_size[0]*image_buffer_size[1]*sizeof(cl_float)*2, // *2 so it can be used for parallel reduction
        NULL,
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    // BG buffer
    Matrix<size_t> region(1,3);
    region[0] = 16;
    region[1] = 16;
    region[2] = 16;
    
    cl_image_format format_3Dimg;
    format_3Dimg.image_channel_order = CL_INTENSITY;
    format_3Dimg.image_channel_data_type = CL_FLOAT;
    
    series_interpol_gpu_3Dimg = QOpenCLCreateImage3D ( context_cl->context(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        &format_3Dimg,
        region[0],
        region[1],
        region[2],
        0,
        0,
        NULL,
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    isCLInitialized = true;
    
    setParameter(parameter);
}

void ImagePreviewWorker::setTsf(TransferFunction & tsf)
{
    if (isTsfTexInitialized)
    {
        err =  QOpenCLReleaseMemObject(tsf_tex_cl);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        glDeleteTextures(1, &tsf_tex_gl);
    }
    
    // Buffer for tsf_tex_gl
    glGenTextures(1, &tsf_tex_gl);
    glBindTexture(GL_TEXTURE_2D, tsf_tex_gl);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA32F,
        tsf.getSplined()->n(),
        1,
        0,
        GL_RGBA,
        GL_FLOAT,
        tsf.getSplined()->colmajor().toFloat().data());
    glBindTexture(GL_TEXTURE_2D, 0);

    isTsfTexInitialized = true;

    tsf_tex_cl =  QOpenCLCreateFromGLTexture2D(context_cl->context(), CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, tsf_tex_gl, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void ImagePreviewWorker::initialize()
{
    initOpenCL();

    glGenBuffers(5, selections_vbo);
    glGenBuffers(5, weightpoints_vbo);
}


void ImagePreviewWorker::setTsfTexture(int value)
{
    rgb_style = value;

    tsf.setColorScheme(rgb_style, alpha_style);
    tsf.setSpline(256);

    if (isInitialized) setTsf(tsf);
    
    refreshDisplay();
}
void ImagePreviewWorker::setTsfAlpha(int value)
{
    alpha_style = value;

    tsf.setColorScheme(rgb_style, alpha_style);
    tsf.setSpline(256);

    if (isInitialized) setTsf(tsf);

    refreshDisplay();
}
void ImagePreviewWorker::setLog(bool value)
{
    isLog = (int) value;
    
    refreshDisplay();
}

void ImagePreviewWorker::setDataMin(double value)
{
    parameter[12] = value;
    setParameter(parameter);

    refreshDisplay();
}
void ImagePreviewWorker::setDataMax(double value)
{
    parameter[13] = value;
    setParameter(parameter);

    refreshDisplay();
}

void ImagePreviewWorker::setThresholdNoiseLow(double value)
{
    parameter[0] = value;
    setParameter(parameter);
    
    calculus();
    refreshDisplay();
    
    if(!p_set.isEmpty())
    {
        Selection analysis_area = p_set.current()->current()->selection();
        refreshSelection(&analysis_area);
        p_set.current()->current()->setSelection(analysis_area);
    }
    
}



void ImagePreviewWorker::setThresholdNoiseHigh(double value)
{
    parameter[1] = value;
    setParameter(parameter);

    calculus();
    refreshDisplay();

    if(!p_set.isEmpty())
    {
        Selection analysis_area = p_set.current()->current()->selection();
        refreshSelection(&analysis_area);
        p_set.current()->current()->setSelection(analysis_area);
    }
}
void ImagePreviewWorker::setThresholdPostCorrectionLow(double value)
{
    parameter[2] = value;
    setParameter(parameter);

    calculus();
    refreshDisplay();

    if(!p_set.isEmpty())
    {
        Selection analysis_area = p_set.current()->current()->selection();
        refreshSelection(&analysis_area);
        p_set.current()->current()->setSelection(analysis_area);
    }
}
void ImagePreviewWorker::setThresholdPostCorrectionHigh(double value)
{
    parameter[3] = value;
    setParameter(parameter);

    calculus();
    refreshDisplay();

    if(!p_set.isEmpty())
    {
        Selection analysis_area = p_set.current()->current()->selection();
        refreshSelection(&analysis_area);
        p_set.current()->current()->setSelection(analysis_area);
    }
}

void ImagePreviewWorker::beginRawGLCalls(QPainter * painter)
{
    painter->beginNativePainting();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
}

void ImagePreviewWorker::endRawGLCalls(QPainter * painter)
{
    glDisable(GL_BLEND);
    glDisable(GL_MULTISAMPLE);
    painter->endNativePainting();
}

void ImagePreviewWorker::render(QPainter *painter)
{
    painter->setRenderHint(QPainter::Antialiasing);
    
    beginRawGLCalls(painter);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    const qreal retinaScale = render_surface->devicePixelRatio();
    glViewport(0, 0, render_surface->width() * retinaScale, render_surface->height() * retinaScale);

    endRawGLCalls(painter);

    if(!p_set.isEmpty())
    {
        drawImage(painter);

        ColorMatrix<float> analysis_area_color(0.0,0,0,0.7);
        ColorMatrix<float> analysis_wp_color(0.0,0.0,0.0,1.0);

        drawSelection(p_set.current()->current()->selection(), painter, analysis_area_color);

        if (isWeightCenterActive) drawWeightpoint(p_set.current()->current()->selection(), painter, analysis_wp_color);

        drawToolTip(painter);
    }
}

void ImagePreviewWorker::drawImage(QPainter * painter)
{
    beginRawGLCalls(painter);

    shared_window->rect_hl_2d_tex_program->bind();

    shared_window->rect_hl_2d_tex_program->setUniformValue(shared_window->rect_hl_2d_tex_texture, 0);

    GLfloat texpos[] = {
        0.0, 0.0,
        1.0, 0.0,
        1.0, 1.0,
        0.0, 1.0
    };
    
    GLuint indices[] = {0,1,3,1,2,3};

    texture_view_matrix = zoom_matrix*translation_matrix;

    glUniformMatrix4fv(shared_window->rect_hl_2d_tex_transform, 1, GL_FALSE, texture_view_matrix.colmajor().toFloat().data());
    
    // The bounds that enclose the highlighted area of the texture are passed to the shader
    Matrix<double> bounds(1,4); // left, top, right, bottom
    

    bounds[0] = (double) p_set.current()->current()->selection().left() / (double) frame.getFastDimension();
    bounds[1] = 1.0 - (double) (p_set.current()->current()->selection().y()+p_set.current()->current()->selection().height()) / (double) frame.getSlowDimension();
    bounds[2] = (double) (p_set.current()->current()->selection().x()+p_set.current()->current()->selection().width()) / (double) frame.getFastDimension();
    bounds[3] = 1.0 - (double) (p_set.current()->current()->selection().top()) / (double) frame.getSlowDimension();
    
    glUniform4fv(shared_window->rect_hl_2d_tex_bounds, 1, bounds.toFloat().data());
    
    // Set the size of a pixel (in image units)
    GLfloat pixel_size = (1.0f / (float) frame.getFastDimension()) / zoom_matrix[0];
    
    glUniform1f(shared_window->rect_hl_2d_tex_pixel_size, pixel_size);
    
    glEnableVertexAttribArray(shared_window->rect_hl_2d_tex_fragpos);
    glEnableVertexAttribArray(shared_window->rect_hl_2d_tex_pos);

    Matrix<GLfloat> fragpos;

    // Draw image
    QRectF image_rect(QPoint(0,0),QSizeF(frame.getFastDimension(), frame.getSlowDimension()));
    image_rect.moveTopLeft(QPointF((qreal) render_surface->width()*0.5, (qreal) render_surface->height()*0.5));

    glBindTexture(GL_TEXTURE_2D, image_tex_gl);
    fragpos = glRect(image_rect);
    glVertexAttribPointer(shared_window->rect_hl_2d_tex_fragpos, 2, GL_FLOAT, GL_FALSE, 0, fragpos.data());
    glVertexAttribPointer(shared_window->rect_hl_2d_tex_pos, 2, GL_FLOAT, GL_FALSE, 0, texpos);

    glDrawElements(GL_TRIANGLES,  6,  GL_UNSIGNED_INT,  indices);

    // Draw max frame
    QRectF bg_rect(QPoint(0,0),QSizeF(frame.getFastDimension(), frame.getSlowDimension()));
    bg_rect.moveTopLeft(image_rect.topRight() + QPointF(20,0));

    glBindTexture(GL_TEXTURE_2D, max_tex_gl);
    fragpos = glRect(bg_rect);
    glVertexAttribPointer(shared_window->rect_hl_2d_tex_fragpos, 2, GL_FLOAT, GL_FALSE, 0, fragpos.data());
    glVertexAttribPointer(shared_window->rect_hl_2d_tex_pos, 2, GL_FLOAT, GL_FALSE, 0, texpos);

    glDrawElements(GL_TRIANGLES,  6,  GL_UNSIGNED_INT,  indices);

    glBindTexture(GL_TEXTURE_2D, 0);

    glDisableVertexAttribArray(shared_window->rect_hl_2d_tex_pos);
    glDisableVertexAttribArray(shared_window->rect_hl_2d_tex_fragpos);


    shared_window->rect_hl_2d_tex_program->release();

    endRawGLCalls(painter);
}

void ImagePreviewWorker::centerImage()
{
    translation_matrix[3] = (qreal) -frame.getFastDimension()/( (qreal) render_surface->width());
    translation_matrix[7] = (qreal) frame.getSlowDimension()/( (qreal) render_surface->height());
    
    zoom_matrix[0] = (qreal) render_surface->width() / (qreal) frame.getFastDimension();
    zoom_matrix[5] = (qreal) render_surface->width() / (qreal) frame.getFastDimension();
    zoom_matrix[10] = (qreal) render_surface->width() / (qreal) frame.getFastDimension();
}




void ImagePreviewWorker::drawSelection(Selection area, QPainter *painter, Matrix<float> &color)
{
    glLineWidth(2.0);

    float selection_left = (((qreal) area.left() + 0.5*render_surface->width()) / (qreal) render_surface->width()) * 2.0 - 1.0; // Left
    float selection_right = (((qreal) area.x() + area.width()  + 0.5*render_surface->width())/ (qreal) render_surface->width()) * 2.0 - 1.0; // Right
    
    float selection_top = (1.0 - (qreal) (area.top() + 0.5*render_surface->height())/ (qreal) render_surface->height()) * 2.0 - 1.0; // Top
    float selection_bot = (1.0 - (qreal) (area.y() + area.height() + 0.5*render_surface->height())/ (qreal) render_surface->height()) * 2.0 - 1.0; // Bottom
    
    float frame_left = (((qreal) -10 + 0.5*render_surface->width()) / (qreal) render_surface->width()) * 2.0 - 1.0; // Left
    float frame_right = (((qreal) frame.getFastDimension() + 10  + 0.5*render_surface->width())/ (qreal) render_surface->width()) * 2.0 - 1.0; // Right
    
    float frame_top = (1.0 - (qreal) (-10 + 0.5*render_surface->height())/ (qreal) render_surface->height()) * 2.0 - 1.0; // Top
    float frame_bot = (1.0 - (qreal) (frame.getSlowDimension() + 10 + 0.5*render_surface->height())/ (qreal) render_surface->height()) * 2.0 - 1.0; // Bottom
    
    // Points
    Matrix<GLfloat> point(8,2);
    point[0] = frame_left;
    point[1] = frame_top;
    point[2] = frame_left;
    point[3] = frame_bot;
    
    point[4] = frame_right;
    point[5] = frame_top;
    point[6] = frame_right;
    point[7] = frame_bot;
    
    point[8] = selection_left;
    point[9] = selection_top;
    point[10] = selection_left;
    point[11] = selection_bot;
    
    point[12] = selection_right;
    point[13] = selection_top;
    point[14] = selection_right;
    point[15] = selection_bot;

    setVbo(selections_vbo[0], point.data(), point.size(), GL_DYNAMIC_DRAW);

    beginRawGLCalls(painter);

    shared_window->std_2d_col_program->bind();
    glEnableVertexAttribArray(shared_window->std_2d_col_fragpos);

    glUniform4fv(shared_window->std_2d_col_color, 1, color.data());

    glBindBuffer(GL_ARRAY_BUFFER, selections_vbo[0]);
    glVertexAttribPointer(shared_window->std_2d_col_fragpos, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    texture_view_matrix = zoom_matrix*translation_matrix;

    glUniformMatrix4fv(shared_window->std_2d_col_transform, 1, GL_FALSE, texture_view_matrix.colmajor().toFloat().data());
    
    
//    GLuint indices[] = {0,1,3,2,0, 4,6,7,5,4,0};
    GLuint indices[] = {0,1,4, 1,4,5, 1,3,5, 3,5,7, 2,3,7, 2,6,7, 0,2,6, 0,4,6};
    glDrawElements(GL_TRIANGLES,  3*8, GL_UNSIGNED_INT, indices);

    glDisableVertexAttribArray(shared_window->std_2d_col_fragpos);

    shared_window->std_2d_col_program->release();

    endRawGLCalls(painter);
}

void ImagePreviewWorker::drawWeightpoint(Selection area, QPainter *painter, Matrix<float> &color)
{
    // Change to draw a faded polygon
    ColorMatrix<float> selection_lines_color(0.0f,0.0f,0.0f,1.0f);
    
    glLineWidth(2.5);

    float x0 = (((qreal) area.left() + 0.5*render_surface->width()) / (qreal) render_surface->width()) * 2.0 - 1.0; // Left
    float x2 = (((qreal) area.x() + area.width()  + 0.5*render_surface->width())/ (qreal) render_surface->width()) * 2.0 - 1.0; // Right
    float x1 = (((qreal) area.weighted_x()  + 0.5*render_surface->width())/ (qreal) render_surface->width()) * 2.0 - 1.0; // Center
    
    float y0 = (1.0 - (qreal) (area.top() + 0.5*render_surface->height())/ (qreal) render_surface->height()) * 2.0 - 1.0; // Top
    float y2 = (1.0 - (qreal) (area.y() + area.height() + 0.5*render_surface->height())/ (qreal) render_surface->height()) * 2.0 - 1.0; // Bottom
    float y1 = (1.0 - (qreal) (area.weighted_y() + 0.5*render_surface->height())/ (qreal) render_surface->height()) * 2.0 - 1.0; // Center 
    
    float x_offset = (x2 - x0)*0.02;
    float y_offset = (y2 - y0)*0.02;
    
    Matrix<GLfloat> selection_lines(8,2);
    selection_lines[0] = x0;
    selection_lines[1] = y1;
    selection_lines[2] = x1 - x_offset;
    selection_lines[3] = y1;
    
    selection_lines[4] = x1 + x_offset;
    selection_lines[5] = y1;
    selection_lines[6] = x2;
    selection_lines[7] = y1;
    
    selection_lines[8] = x1;
    selection_lines[9] = y1 - y_offset;
    selection_lines[10] = x1;
    selection_lines[11] = y0;
    
    selection_lines[12] = x1;
    selection_lines[13] = y1 + y_offset;
    selection_lines[14] = x1;
    selection_lines[15] = y2 ;
    
    

    setVbo(weightpoints_vbo[0], selection_lines.data(), selection_lines.size(), GL_DYNAMIC_DRAW);

    beginRawGLCalls(painter);

    shared_window->std_2d_col_program->bind();
    glEnableVertexAttribArray(shared_window->std_2d_col_fragpos);

    glUniform4fv(shared_window->std_2d_col_color, 1, color.data());

    glBindBuffer(GL_ARRAY_BUFFER, weightpoints_vbo[0]);
    glVertexAttribPointer(shared_window->std_2d_col_fragpos, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    texture_view_matrix = zoom_matrix*translation_matrix;

    glUniformMatrix4fv(shared_window->std_2d_col_transform, 1, GL_FALSE, texture_view_matrix.colmajor().toFloat().data());

    glDrawArrays(GL_LINES,  0, 8);

    glDisableVertexAttribArray(shared_window->std_2d_col_fragpos);

    shared_window->std_2d_col_program->release();

    endRawGLCalls(painter);
}

Matrix<double> ImagePreviewWorker::getScatteringVector(DetectorFile & f, double x, double y)
{
    // Assumes that the incoming ray is parallel to the z axis.

    double k = 1.0f/f.wavelength; // Multiply with 2pi if desired

    Matrix<double> k_i(1,3,0);
    k_i[0] = -k;
    
    Matrix<double> k_f(1,3,0);
    k_f[0] =    -f.detector_distance;
    k_f[1] =    f.pixel_size_x * ((double) (f.getSlowDimension() - y) - f.beam_x);
    k_f[2] =    f.pixel_size_y * ((double) x - f.beam_y);
    k_f = vecNormalize(k_f);
    k_f = k*k_f;
    

    Matrix<double> Q(1,3,0);
    Q = k_f - k_i;
    
    return Q;
}

double ImagePreviewWorker::getScatteringAngle(DetectorFile & f, double x, double y)
{
    // Assumes that the incoming ray is parallel to the z axis.

    double k = 1.0f/f.wavelength; // Multiply with 2pi if desired

    Matrix<double> k_i(1,3,0);
    k_i[0] = -k;

    Matrix<double> k_f(1,3,0);
    k_f[0] =    -f.detector_distance;
    k_f[1] =    f.pixel_size_x * ((double) (f.getSlowDimension() - y) - f.beam_x);
    k_f[2] =    f.pixel_size_y * ((double) x - f.beam_y);
    k_f = vecNormalize(k_f);
    k_f = k*k_f;

    return acos(vecDot(k_f, k_i)/(k*k));
}

void ImagePreviewWorker::drawToolTip(QPainter *painter)
{
    if (isFrameValid == false) return;
    
    // The tooltip text
    
    //Position
    Matrix<double> screen_pixel_pos(4,1,0); // Uses GL coordinates
    screen_pixel_pos[0] = 2.0 * (double) pos.x()/(double) render_surface->width() - 1.0;
    screen_pixel_pos[1] = 2.0 * (1.0 - (double) pos.y()/(double) render_surface->height()) - 1.0;
    screen_pixel_pos[2] = 0;
    screen_pixel_pos[3] = 1.0;
    
    Matrix<double> image_pixel_pos(4,1); // Uses GL coordinates
    
    image_pixel_pos = texture_view_matrix.inverse4x4() * screen_pixel_pos;
    
    double pixel_x = image_pixel_pos[0] * render_surface->width() * 0.5;
    double pixel_y = - image_pixel_pos[1] * render_surface->height() * 0.5;
    
    if (pixel_x < 0) pixel_x = 0;
    if (pixel_y < 0) pixel_y = 0;
    if (pixel_x >= frame.getFastDimension()) pixel_x = frame.getFastDimension()-1;
    if (pixel_y >= frame.getSlowDimension()) pixel_y = frame.getSlowDimension()-1;
    
    QString tip;
    tip += "Pixel (x,y) "+QString::number((int) pixel_x)+" "+QString::number((int) pixel_y)+"\n";
    
    // Intensity
    float value = 0;
    
    if (mode == 0)
    {
        err =   QOpenCLEnqueueReadBuffer ( context_cl->queue(),
            image_data_corrected_cl,
            CL_TRUE,
            ((int) pixel_y * frame.getFastDimension() + (int) pixel_x)*sizeof(cl_float),
            sizeof(cl_float),
            &value,
            0, NULL, NULL);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
    else if (mode == 1)
    {
        err =   QOpenCLEnqueueReadBuffer ( context_cl->queue(),
            image_data_variance_cl,
            CL_TRUE,
            ((int) pixel_y * frame.getFastDimension() + (int) pixel_x)*sizeof(cl_float),
            sizeof(cl_float),
            &value,
            0, NULL, NULL);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
    else if (mode == 2)
    {
        err =   QOpenCLEnqueueReadBuffer ( context_cl->queue(),
            image_data_skewness_cl,
            CL_TRUE,
            ((int) pixel_y * frame.getFastDimension() + (int) pixel_x)*sizeof(cl_float),
            sizeof(cl_float),
            &value,
            0, NULL, NULL);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
    
    
    tip += "Intensity "+QString::number(value,'g',4)+"\n";
    
    // The scattering angle 2-theta
    tip += "Scattering angle (2ϴ) "+QString::number(180*getScatteringAngle(frame, pixel_x, pixel_y)/pi,'f',2)+"°\n";
    
    // Position
    Matrix<double> Q(1,3);
    Q = getScatteringVector(frame, pixel_x, pixel_y);

    tip += "Position (x,y,z) "+QString::number(Q[0],'f',2)+" "+QString::number(Q[1],'f',2)+" "+QString::number(Q[2],'f',2)+" ("+QString::number(sqrt(Q[0]*Q[0] + Q[1]*Q[1] + Q[2]*Q[2]),'f',2)+")\n";
    
    
    // Intensity center
    tip += "Weight center (x,y) "+QString::number(p_set.current()->current()->selection().weighted_x(),'f',2)+" "+QString::number(p_set.current()->current()->selection().weighted_y(),'f',2)+"\n";
    
    // Sum
    tip += "Integral "+QString::number(p_set.current()->current()->selection().integral(),'f',2);
    
    
    QFont font("Helvetica", 10);
    QFontMetrics fm(font);
    
    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    brush.setColor(QColor(255,255,255,155));
            
    painter->setFont(font);
    painter->setBrush(brush);
    
    
    
    // Define the area assigned to displaying the tooltip
    QRect area = fm.boundingRect (render_surface->geometry(), Qt::AlignLeft, tip);
    
    area.moveBottomLeft(QPoint(5,render_surface->height()-5));
    
    area += QMargins(2,2,2,2);
    painter->drawRoundedRect(area,5,5);
    area -= QMargins(2,2,2,2);
    
    // Draw tooltip
    painter->drawText(area, Qt::AlignLeft, tip);

}


void ImagePreviewWorker::setMode(int value)
{
    mode = value;
    calculus();
    refreshDisplay();

    if(!p_set.isEmpty())
    {
        Selection analysis_area = p_set.current()->current()->selection();
        refreshSelection(&analysis_area);
        p_set.current()->current()->setSelection(analysis_area);
    }
}

void ImagePreviewWorker::setCorrectionLorentz(bool value)
{
    isLorentzCorrected = (int) value;

    calculus();
    refreshDisplay();

    if(!p_set.isEmpty())
    {
        Selection analysis_area = p_set.current()->current()->selection();
        refreshSelection(&analysis_area);
        p_set.current()->current()->setSelection(analysis_area);
    }
}

void ImagePreviewWorker::setCorrectionBackground(bool value)
{
    isBackgroundCorrected = (int) value;

    calculus();
    refreshDisplay();

    if(!p_set.isEmpty())
    {
        Selection analysis_area = p_set.current()->current()->selection();
        refreshSelection(&analysis_area);
        p_set.current()->current()->setSelection(analysis_area);
    }
}

//void ImagePreviewWorker::setAutoBackgroundCorrection(bool value)
//{
//    isAutoBackgroundCorrectionActive = (int) value;
    
//    Selection analysis_area = p_set.current()->current()->selection();
//    Selection background_area = p_set.current()->current()->background();
    
//    refreshBackground(&background_area);
//    calculus();
//    refreshDisplay();
    
//    refreshSelection(&analysis_area);
//    p_set.current()->current()->setSelection(analysis_area);
//    p_set.current()->current()->setBackground(background_area);
//}

void ImagePreviewWorker::setParameter(Matrix<float> & data)
{
    if (0)
    {
        qDebug() << "th_a_low" << data[0];
        qDebug() << "th_a_high" << data[1];
        qDebug() << "th_b_low" << data[2];
        qDebug() << "th_b_high" << data[3];
        qDebug() << "flux" << data[4];
        qDebug() << "exp_time" << data[5];
        qDebug() << "wavelength" << data[6];
        qDebug() << "det_dist" << data[7];
        qDebug() << "beam_x" << data[8];
        qDebug() << "beam_y" << data[9];
        qDebug() << "pix_size_x" << data[10];
        qDebug() << "pix_size_y" << data[11];
        qDebug() << "intensity_min" << data[12];
        qDebug() << "intensity_max" << data[13];
    }

    if (isCLInitialized)
    {
        err =  QOpenCLEnqueueWriteBuffer (context_cl->queue(),
            parameter_cl,
            CL_TRUE,
            0,
            data.bytes(),
            data.data(),
            0,0,0);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
}

//void ImagePreviewWorker::setSelectionAlphaActive(bool value)
//{
//    isSelectionAlphaActive = value;
//    if (value) isSelectionBetaActive = false;
//    emit selectionBetaChanged(isSelectionBetaActive);
//}

//void ImagePreviewWorker::setSelectionBetaActive(bool value)
//{
//    isSelectionBetaActive = value;
//    if (value) isSelectionAlphaActive = false;
//    emit selectionAlphaChanged(isSelectionAlphaActive);
//}

Matrix<int> ImagePreviewWorker::getImagePixel(int x, int y)
{
    // Find the OpenGL coordinate of the cursor
    Matrix<double> screen_pos_gl(4,1); 
    screen_pos_gl[0] = 2.0 * (double) x / (double) render_surface->width() - 1.0;
    screen_pos_gl[1] = - 2.0 * (double) y / (double) render_surface->height() + 1.0;
    screen_pos_gl[2] = 0;
    screen_pos_gl[3] = 1.0;
    
    // Use the inverse transform to find the corresponding image pixel, rounding to nearest
    Matrix<double> image_pos_gl(4,1); 
    image_pos_gl = texture_view_matrix.inverse4x4()*screen_pos_gl;
    
    Matrix<int> image_pixel(2,1);
    
    image_pixel[0] = 0.5*image_pos_gl[0]*render_surface->width();
    image_pixel[1] = -0.5*image_pos_gl[1]*render_surface->height();
    
    if (image_pixel[0] < 0) image_pixel[0] = 0;
    if (image_pixel[0] >= frame.getFastDimension()) image_pixel[0] = frame.getFastDimension() - 1;
    
    if (image_pixel[1] < 0) image_pixel[1] = 0;
    if (image_pixel[1] >= frame.getSlowDimension()) image_pixel[1] = frame.getSlowDimension() - 1;
    
    return image_pixel;
}

void ImagePreviewWorker::metaMouseMoveEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
    Q_UNUSED(mid_button);
    Q_UNUSED(right_button);
    Q_UNUSED(ctrl_button);

    float move_scaling = 1.0;
    
    pos = QPoint(x,y);

    if (left_button)
    {
        if (shift_button)
        {
            Selection analysis_area = p_set.current()->current()->selection();
            
            Matrix<int> pixel = getImagePixel(pos.x(), pos.y());
            
            analysis_area.setBottomRight(QPoint(pixel[0], pixel[1]));
        
            analysis_area = analysis_area.normalized();
            
            refreshSelection(&analysis_area);
            
            p_set.current()->current()->setSelection(analysis_area);
        }
        else
        {
            double dx = (pos.x() - prev_pos.x())*2.0/(render_surface->width()*zoom_matrix[0]);
            double dy = -(pos.y() - prev_pos.y())*2.0/(render_surface->height()*zoom_matrix[0]);

            translation_matrix[3] += dx*move_scaling;
            translation_matrix[7] += dy*move_scaling;
        }
    }

    prev_pos = pos;
}

void ImagePreviewWorker::metaMousePressEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
    Q_UNUSED(mid_button);
    Q_UNUSED(right_button);
    Q_UNUSED(ctrl_button);
    Q_UNUSED(shift_button);
    
    pos = QPoint(x,y);
    
    if (shift_button && left_button)
    {
        Selection analysis_area = p_set.current()->current()->selection();
        
        Matrix<int> pixel = getImagePixel(pos.x(), pos.y());
        
        analysis_area.setTopLeft(QPoint(pixel[0], pixel[1]));
    
        analysis_area = analysis_area.normalized();
        
        refreshSelection(&analysis_area);
        
        p_set.current()->current()->setSelection(analysis_area);
    }
    
    
}

void ImagePreviewWorker::metaMouseReleaseEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
    Q_UNUSED(mid_button);
    Q_UNUSED(right_button);
    Q_UNUSED(ctrl_button);
    Q_UNUSED(shift_button);

    pos = QPoint(x,y);
    
    if (shift_button && left_button)
    {
        Selection analysis_area = p_set.current()->current()->selection();
        
        Matrix<int> pixel = getImagePixel(pos.x(), pos.y());
        
        analysis_area.setBottomRight(QPoint(pixel[0], pixel[1]));
    
        analysis_area = analysis_area.normalized();
        
        refreshSelection(&analysis_area);
        
        p_set.current()->current()->setSelection(analysis_area);
        

//        emit selectionChanged(p_set.current()->current()->selection());
//        emit imageChanged(frame_image);
    }
    
    
}   

void ImagePreviewWindow::keyPressEvent(QKeyEvent *ev)
{
    // This is an example of letting the QWindow take care of event handling. In all fairness, only swapbuffers and heavy work (OpenCL and calculations) need to be done in a separate thread. 
    //Currently bugged somehow. A signal to the worker appears to block further key events.
//    qDebug() << "Press key " << ev->key();
    if (ev->key() == Qt::Key_Shift) 
    {
        emit selectionActiveChanged(true);
    }
}

void ImagePreviewWindow::keyReleaseEvent(QKeyEvent *ev)
{
//    qDebug() << "Release key " << ev->key();
    if (ev->key() == Qt::Key_Shift) 
    {
        emit selectionActiveChanged(false);
    }
}

void ImagePreviewWorker::wheelEvent(QWheelEvent* ev)
{

    float move_scaling = 1.0;
    if(ev->modifiers() & Qt::ShiftModifier) move_scaling = 5.0;
    else if(ev->modifiers() & Qt::ControlModifier) move_scaling = 0.2;

    double delta = move_scaling*((double)ev->delta())*0.0008;

    if ((zoom_matrix[0] + zoom_matrix[0]*delta < 256))// && (isRendering == false))
    {
        /*
         * Zooming happens around the GL screen coordinate (0,0), i.e. the middle,
         * but by first tranlating the frame to the position of the cursor, we can
         * make zooming happen on the cursor instead. Then the frame should be
         * translated back such that the object under the cursor does not appear
         * to actually have been translated.
         * */

        // Translate cursor position to middle
        double dx = -(ev->x() - render_surface->width()*0.5)*2.0/(render_surface->width()*zoom_matrix[0]);
        double dy = -((render_surface->height() - ev->y()) - render_surface->height()*0.5)*2.0/(render_surface->height()*zoom_matrix[0]);

        translation_matrix[3] += dx;
        translation_matrix[7] += dy;

        double tmp = zoom_matrix[0];

        // Zoom
        zoom_matrix[0] += zoom_matrix[0]*delta;
        zoom_matrix[5] += zoom_matrix[5]*delta;
        zoom_matrix[10] += zoom_matrix[10]*delta;

        // Translate from middle back to cursor position, taking into account the new zoom
        translation_matrix[3] -= dx*tmp/zoom_matrix[0];
        translation_matrix[7] -= dy*tmp/zoom_matrix[0];
    }


}
void ImagePreviewWorker::resizeEvent(QResizeEvent * ev)
{
    Q_UNUSED(ev);

    if (paint_device_gl) paint_device_gl->setSize(render_surface->size());
}

ImagePreviewWindow::ImagePreviewWindow()
    : isInitialized(false)
    , gl_worker(0)
{

}
ImagePreviewWindow::~ImagePreviewWindow()
{

}

void ImagePreviewWindow::setSharedWindow(SharedContextWindow * window)
{
    this->shared_window = window;
    shared_context = window->getGLContext();
}
ImagePreviewWorker * ImagePreviewWindow::worker()
{
    return gl_worker;
}

void ImagePreviewWindow::initializeWorker()
{
    initializeGLContext();

    gl_worker = new ImagePreviewWorker;
    gl_worker->setRenderSurface(this);
    gl_worker->setOpenGLContext(context_gl);
    gl_worker->setOpenCLContext(context_cl);
    gl_worker->setSharedWindow(shared_window);
    gl_worker->setMultiThreading(isThreaded);

    if (isThreaded)
    {
        // Set up worker thread
        gl_worker->moveToThread(worker_thread);
        connect(this, SIGNAL(render()), gl_worker, SLOT(process()));
        connect(this, SIGNAL(stopRendering()), worker_thread, SLOT(quit()));
        connect(gl_worker, SIGNAL(finished()), this, SLOT(setSwapState()));

        // Transfering mouse events
        connect(this, SIGNAL(metaMouseMoveEventCaught(int, int, int, int, int, int, int)), gl_worker, SLOT(metaMouseMoveEvent(int, int, int, int, int, int, int)));
        connect(this, SIGNAL(metaMousePressEventCaught(int, int, int, int, int, int, int)), gl_worker, SLOT(metaMousePressEvent(int, int, int, int, int, int, int)));
        connect(this, SIGNAL(metaMouseReleaseEventCaught(int, int, int, int, int, int, int)), gl_worker, SLOT(metaMouseReleaseEvent(int, int, int, int, int, int, int)));
        connect(this, SIGNAL(resizeEventCaught(QResizeEvent*)), gl_worker, SLOT(resizeEvent(QResizeEvent*)));
        connect(this, SIGNAL(wheelEventCaught(QWheelEvent*)), gl_worker, SLOT(wheelEvent(QWheelEvent*)), Qt::DirectConnection);
        
        emit render(); // A call to render is necessary to make sure initialize() is callled
    }

    isInitialized = true;
}

void ImagePreviewWindow::renderNow()
{
    
    if (!isExposed())
    {
        emit stopRendering();
        return;
    }
    else if (!isWorkerBusy)
    {
        if (!isInitialized) initializeWorker();

        if (gl_worker)
        {
            if (isThreaded)
            {
                isWorkerBusy = true;
                worker_thread->start(); // Reaching this point will activate the thread
                emit render();
            }
        }
    }
    renderLater();
}

SeriesToolShed::SeriesToolShed()
{
    dim.set(1,3,0);
}

SeriesToolShed::~SeriesToolShed()
{

}

//SeriesTrace::SeriesTrace()
//{

//}

//SeriesTrace::~SeriesTrace()
//{

//}

SeriesSet ImagePreviewWorker::set()
{
    return p_set;
}

//void ImagePreviewWorker::populateSeriesBackgroundSamples(ImageSeries * series)
//{
    // Note: It is assumed that all images in a series have the same dimensions

    // Given a set of rules for sample selection. Samples are taken on a regular, equidistant grid.
//    size_t bg_sample_interdist = 8;

    // Prepare the storage buffer
//    size_t m = series->

//    main_series.series_samples_cpu.set(m,n,0);

    // For each image in the series

    // Move relevant samples into a separate buffer

//}
