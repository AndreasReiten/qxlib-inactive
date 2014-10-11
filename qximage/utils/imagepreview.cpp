#include "imagepreview.h"
#include <QPen>
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
    isAutoBackgroundCorrectionActive(false),
    rgb_style(1),
    alpha_style(2),
    isSelectionAlphaActive(false),
    isSelectionBetaActive(false)
{
    Q_UNUSED(parent);

    isInitialized = false;

    parameter.reserve(1,16);

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

void ImagePreviewWorker::imageCalcuclus(cl_mem data_buf_cl, cl_mem out_buf_cl, Matrix<float> & param, Matrix<size_t> &image_size, Matrix<size_t> & local_ws, int correction, float mean, float deviation, int task)
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
    err |=   QOpenCLSetKernelArg(cl_image_calculus, 4, sizeof(cl_int), &correction);
    err |=   QOpenCLSetKernelArg(cl_image_calculus, 5, sizeof(cl_int), &task);
    err |=   QOpenCLSetKernelArg(cl_image_calculus, 6, sizeof(cl_float), &mean);
    err |=   QOpenCLSetKernelArg(cl_image_calculus, 7, sizeof(cl_float), &deviation);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    
    // Launch the kernel
    err =   QOpenCLEnqueueNDRangeKernel(context_cl->queue(), cl_image_calculus, 2, NULL, global_ws.data(), local_ws.data(), 0, NULL, NULL);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    err =   QOpenCLFinish(context_cl->queue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void ImagePreviewWorker::imageDisplay(cl_mem data_buf_cl, cl_mem frame_image_cl, cl_mem tsf_image_cl, Matrix<float> &data_limit, Matrix<size_t> &image_size, Matrix<size_t> & local_ws, cl_sampler tsf_sampler, int log)
{
    if (!isFrameValid) return;
        
    // Aquire shared CL/GL objects
    context_gl->makeCurrent(render_surface);
    
    glFinish();

//    qDebug() << "it is now required";
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
//    data_limit.print(2,"data_limit");
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
            imageCalcuclus(image_data_raw_cl, image_data_corrected_cl, parameter, image_size, local_ws, isCorrected, 0, 0, 0);
            
            // Calculate the weighted intensity position
            imageCalcuclus(image_data_corrected_cl, image_data_weight_x_cl, parameter, image_size, local_ws, isCorrected, 0, 0, 3);
            imageCalcuclus(image_data_corrected_cl, image_data_weight_y_cl, parameter, image_size, local_ws, isCorrected, 0, 0, 4);
        }

    }
    if (mode == 1)
    {
        // Variance
        {
            imageCalcuclus(image_data_raw_cl, image_data_corrected_cl, parameter, image_size, local_ws, isCorrected, 0, 0, 0);
            
            // Calculate the variance
            copyBufferRect(image_data_corrected_cl, image_data_generic_cl, image_size, origin, image_size, origin, local_ws);
            
            float mean = sumGpuArray(image_data_generic_cl, image_size[0]*image_size[1], local_ws)/(image_size[0]*image_size[1]);
            
            imageCalcuclus(image_data_corrected_cl, image_data_variance_cl, parameter, image_size, local_ws, isCorrected, mean, 0, 1);
            
            // Calculate the weighted intensity position
            imageCalcuclus(image_data_variance_cl, image_data_weight_x_cl, parameter, image_size, local_ws, isCorrected, 0, 0, 3);
            imageCalcuclus(image_data_variance_cl, image_data_weight_y_cl, parameter, image_size, local_ws, isCorrected, 0, 0, 4);
        }
    }
    else if (mode == 2)
    {
        // Skewness
        {
            imageCalcuclus(image_data_raw_cl, image_data_corrected_cl, parameter, image_size, local_ws, isCorrected, 0, 0, 0);
            
            // Calculate the variance
            copyBufferRect(image_data_corrected_cl, image_data_generic_cl, image_size, origin, image_size, origin, local_ws);
            
            float mean = sumGpuArray(image_data_generic_cl, image_size[0]*image_size[1], local_ws)/(image_size[0]*image_size[1]);
            
            imageCalcuclus(image_data_corrected_cl, image_data_variance_cl, parameter, image_size, local_ws, isCorrected, mean, 0, 1);
            
            // Calculate the skewness
            copyBufferRect(image_data_variance_cl, image_data_generic_cl, image_size, origin, image_size, origin, local_ws);
            
            float variance = sumGpuArray(image_data_generic_cl, image_size[0]*image_size[1], local_ws)/(image_size[0]*image_size[1]);
            
            imageCalcuclus(image_data_variance_cl, image_data_skewness_cl, parameter, image_size, local_ws, isCorrected, mean, sqrt(variance), 2);
            
            
            // Calculate the weighted intensity position
            imageCalcuclus(image_data_skewness_cl, image_data_weight_x_cl, parameter, image_size, local_ws, isCorrected, 0, 0, 3);
            imageCalcuclus(image_data_skewness_cl, image_data_weight_y_cl, parameter, image_size, local_ws, isCorrected, 0, 0, 4);
        }
    }
    else
    {
        // Should not happen
    }
}

void ImagePreviewWorker::setFrame(ImageInfo image)
{
    // Set the frame
    frame_image = image;
    if (!frame.set(frame_image.path())) return;
    if(!frame.readData()) return;

    isFrameValid = true;

//    frame.setNaive();
    
    Selection analysis_area = frame_image.selection();
    Selection background_area = frame_image.background();

    // Restrict selection, this could be moved elsewhere and it would look better
    if (analysis_area.left() < 0) analysis_area.setLeft(0);
    if (analysis_area.right() >= frame.getFastDimension()) analysis_area.setRight(frame.getFastDimension()-1);
    if (analysis_area.top() < 0) analysis_area.setTop(0);
    if (analysis_area.bottom() >= frame.getSlowDimension()) analysis_area.setBottom(frame.getSlowDimension()-1);

    if (background_area.left() < 0) background_area.setLeft(0);
    if (background_area.right() >= frame.getFastDimension()) background_area.setRight(frame.getFastDimension()-1);
    if (background_area.top() < 0) background_area.setTop(0);
    if (background_area.bottom() >= frame.getSlowDimension()) background_area.setBottom(frame.getSlowDimension()-1);

    frame_image.setSelection(analysis_area);
    frame_image.setBackground(background_area);

//    emit selectionChanged(analysis_area);
//    emit backgroundChanged(background_area);

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
    refreshBackground(&background_area);
    
    calculus();
    refreshDisplay();
    refreshSelection(&analysis_area);
    
    frame_image.setSelection(analysis_area);
    frame_image.setBackground(background_area);
    
    // Emit the image instead of components
    emit imageChanged(frame_image);

}



void ImagePreviewWorker::clMaintainImageBuffers(Matrix<size_t> &image_size)
{
    if ((image_size[0] != image_buffer_size[0]) || (image_size[1] != image_buffer_size[1]))
    {
        err =  QOpenCLReleaseMemObject(image_data_raw_cl);
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

void ImagePreviewWorker::refreshBackground(Selection * area)
{
    Matrix<size_t> local_ws(1,2);
    local_ws[0] = 64;
    local_ws[1] = 1;
    
    Matrix<size_t> image_size(1,2);
    image_size[0] = frame.getFastDimension();
    image_size[1] = frame.getSlowDimension();

    // Normal intensity
    selectionCalculus(area, image_data_raw_cl, image_data_weight_x_cl, image_data_weight_y_cl, image_size, local_ws);
    
    
    if (isAutoBackgroundCorrectionActive)
    {
        double noise = area->integral()/(double)(area->width()*area->height());
//        setThresholdNoiseLow(noise);
        
        parameter[0] = noise;
        setParameter(parameter);
//        qDebug() << area->integral()/(double)(area->width()*area->height()) << area->integral() << area->width() << area->height();
        emit noiseLowChanged(noise);
    }
    
    
//    emit backgroundChanged(background_area);
}

void ImagePreviewWorker::refreshDisplay()
{
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
    }
    if (mode == 1)
    {
        // Variance
        imageDisplay(image_data_variance_cl, image_tex_cl, tsf_tex_cl, data_limit, image_size, local_ws, tsf_sampler, isLog);
    }
    else if (mode == 2)
    {
        // Skewness
        imageDisplay(image_data_skewness_cl, image_tex_cl, tsf_tex_cl, data_limit, image_size, local_ws, tsf_sampler, isLog);
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
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
            glDeleteTextures(1, &image_tex_gl);
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
        
        image_tex_size[0] = image_size[0];
        image_tex_size[1] = image_size[1];
        
        // Share the texture with the OpenCL runtime
        image_tex_cl =  QOpenCLCreateFromGLTexture2D(context_cl->context(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, image_tex_gl, &err);
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

void ImagePreviewWorker::peakHuntFolder(ImageFolder folder)
{
}

void ImagePreviewWorker::peakHuntSet(FolderSet set)
{
}

void ImagePreviewWorker::analyzeSingle(ImageInfo image)
{
    // Draw the frame and update the intensity OpenCL buffer prior to further operations 
    setFrame(image);
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

    result += integrationFrameString(frame, frame_image);
    emit resultFinished(result);
}

void ImagePreviewWorker::analyzeFolder(ImageFolder folder)
{
    double integral = 0;
    Matrix<double> weightpoint(1,3,0);
    
    QString frames;        
    for (int i = 0; i < folder.size(); i++)
    {
        // Draw the frame and update the intensity OpenCL buffer prior to further operations 
        setFrame(*folder.current());
        {
            QPainter painter(paint_device_gl);
            render(&painter);
        }
        // Force a buffer swap
        context_gl->swapBuffers(render_surface);

        // Math
        Matrix<double> Q = getScatteringVector(frame, frame_image.selection().weighted_x(), frame_image.selection().weighted_y());
        
        weightpoint += frame_image.selection().integral()*Q;
        
        
        integral += frame_image.selection().integral();
        
        frames += integrationFrameString(frame, frame_image);
    
        folder.next();
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
    result += "# Analysis of frames in folder "+folder.path()+"\n";
    result += "# "+QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm:ss t")+"\n";
    result += "#\n";
    result += "# Sum of total integrated area in folder "+QString::number(integral,'E')+"\n";
    result += "# Weightpoint xyz "+QString::number(weightpoint[0],'E')+" "+QString::number(weightpoint[1],'E')+" "+QString::number(weightpoint[2],'E')+" "+QString::number(vecLength(weightpoint),'E')+"\n";
    result += "# Analysis of the individual frames (integral, origin x, origin y, width, height, weight x, weight y, Qx, Qy, Qz, |Q|, 2theta, background, origin x, origin y, width, height, path)\n";
    result += frames;
    
    emit resultFinished(result);
}

void ImagePreviewWorker::analyzeSet(FolderSet set)
{
    double integral = 0;
    QStringList folder_integral;
    QStringList folder_weightpoint;
    QStringList folder_frames;
    QString str;
    
    for (int i = 0; i < set.size(); i++)
    {
        Matrix<double> weightpoint(1,3,0);
        
        for (int j = 0; j < set.current()->size(); j++)
        {
            // Draw the frame and update the intensity OpenCL buffer prior to further operations 
            setFrame(*set.current()->current());
            {
                QPainter painter(paint_device_gl);
                render(&painter);
            }
            // Force a buffer swap
            context_gl->swapBuffers(render_surface);

            
            // Math
            Matrix<double> Q = getScatteringVector(frame, frame_image.selection().weighted_x(), frame_image.selection().weighted_y());
            
            weightpoint += frame_image.selection().integral()*Q;
            
            integral += frame_image.selection().integral();
            
            str += integrationFrameString(frame, frame_image);
        
            set.current()->next(); 
        }
        
        if (integral > 0)
        {
            weightpoint = weightpoint / integral;
        }
        else
        {
            weightpoint.set(1,3,0);
        }
        
        folder_weightpoint << QString::number(weightpoint[0],'E')+" "+QString::number(weightpoint[1],'E')+" "+QString::number(weightpoint[2],'E')+" "+QString::number(vecLength(weightpoint),'E')+"\n";
        
        folder_integral << QString(QString::number(integral,'E')+"\n"); //+" "+set.current()->path()+
        integral = 0;
        
        folder_frames << str;
        str.clear();
        
        set.next();
    }
    
    QString result;
    
    result += "# Analysis of frames in several folders\n";
    result += "# "+QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm:ss t")+"\n";
    result += "#\n";
    result += "# Sum of total integrated area in folders\n";
    foreach(const QString &str, folder_integral)
    {
        result += str;
    }
    result += "# Weightpoints in folders (Qx, Qy, Qz, |Q|)\n";
    foreach(const QString &str, folder_weightpoint)
    {
        result += str;
    }
    
    result += "# Analysis of the individual frames for each folder (integral, origin x, origin y, width, height, weight x, weight y, Qx, Qy, Qz, |Q|, 2theta, background, origin x, origin y, width, height, path)\n";
    for (int i = 0; i < folder_integral.size(); i++)
    {
        result += "# Folder integral "+folder_integral.at(i);
        result += folder_frames.at(i);
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
//    qDebug() << area->integral() << area->weighted_x() << area->weighted_y();
    
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

    program = context_cl->createProgram(paths, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    context_cl->buildProgram(&program, "-Werror");

    // Kernel handles
    cl_display_image =  QOpenCLCreateKernel(program, "imageDisplay", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_image_calculus =  QOpenCLCreateKernel(program, "imageCalculus", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    // Image sampler
    image_sampler =  QOpenCLCreateSampler(context_cl->context(), false, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_NEAREST, &err);
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

//    qDebug() << "A tsf was set";

    tsf_tex_cl =  QOpenCLCreateFromGLTexture2D(context_cl->context(), CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, tsf_tex_gl, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void ImagePreviewWorker::initialize()
{
    initOpenCL();

//    glGenBuffers(2, texel_line_vbo);
    glGenBuffers(5, selections_vbo);
    glGenBuffers(5, weightpoints_vbo);
}


void ImagePreviewWorker::setTsfTexture(int value)
{
    rgb_style = value;

    tsf.setColorScheme(rgb_style, alpha_style);
    tsf.setSpline(256);

//    qDebug() << "now we try to set tsf" << isInitialized;
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
    
    Selection analysis_area = frame_image.selection();
    refreshSelection(&analysis_area);
    frame_image.setSelection(analysis_area);
    
}



void ImagePreviewWorker::setThresholdNoiseHigh(double value)
{
    parameter[1] = value;
    setParameter(parameter);

    calculus();
    refreshDisplay();
    Selection analysis_area = frame_image.selection();
    refreshSelection(&analysis_area);
    frame_image.setSelection(analysis_area);
}
void ImagePreviewWorker::setThresholdPostCorrectionLow(double value)
{
    parameter[2] = value;
    setParameter(parameter);

    calculus();
    refreshDisplay();
    Selection analysis_area = frame_image.selection();
    refreshSelection(&analysis_area);
    frame_image.setSelection(analysis_area);
}
void ImagePreviewWorker::setThresholdPostCorrectionHigh(double value)
{
    parameter[3] = value;
    setParameter(parameter);

    calculus();
    refreshDisplay();
    Selection analysis_area = frame_image.selection();
    refreshSelection(&analysis_area);
    frame_image.setSelection(analysis_area);
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

    drawImage(painter);
    
    ColorMatrix<float> analysis_area_color(0.0,0,0,1);
    ColorMatrix<float> background_area_color(0.0,1,0,1);
    ColorMatrix<float> analysis_wp_color(0.0,0.0,0.0,1.0);
    
    drawSelection(frame_image.selection(), painter, analysis_area_color);
    drawSelection(frame_image.background(), painter, background_area_color);
    
    if (isWeightCenterActive) drawWeightpoint(frame_image.selection(), painter, analysis_wp_color);
    
    drawToolTip(painter);
    
    // Fps
//    QString fps_string("Fps: "+QString::number(getFps(), 'f', 0));
//    painter->drawText(QPointF(5,render_surface->height()-5), fps_string);
}

void ImagePreviewWorker::drawImage(QPainter * painter)
{
    beginRawGLCalls(painter);

    shared_window->rect_hl_2d_tex_program->bind();

    glBindTexture(GL_TEXTURE_2D, image_tex_gl);
    shared_window->rect_hl_2d_tex_program->setUniformValue(shared_window->rect_hl_2d_tex_texture, 0);

    QRectF image_rect(QPoint(0,0),QSizeF(frame.getFastDimension(), frame.getSlowDimension()));
    image_rect.moveTopLeft(QPointF((qreal) render_surface->width()*0.5, (qreal) render_surface->height()*0.5));

    Matrix<GLfloat> fragpos = glRect(image_rect);

    GLfloat texpos[] = {
        0.0, 0.0,
        1.0, 0.0,
        1.0, 1.0,
        0.0, 1.0
    };
    
    GLuint indices[] = {0,1,3,1,2,3};

    texture_view_matrix = zoom_matrix*translation_matrix;

    glUniformMatrix4fv(shared_window->rect_hl_2d_tex_transform, 1, GL_FALSE, texture_view_matrix.colmajor().toFloat().data());
//    if ( glGetError() != GL_NO_ERROR) qFatal(gl_error_cstring(glGetError()));
    
    // The bounds that enclose the highlighted area of the texture are passed to the shader
    Matrix<double> bounds(1,4); // left, top, right, bottom
    
//    qDebug() << selection << selection.right();

    bounds[0] = (double) frame_image.selection().left() / (double) frame.getFastDimension();
    bounds[1] = 1.0 - (double) (frame_image.selection().y()+frame_image.selection().height()) / (double) frame.getSlowDimension();
    bounds[2] = (double) (frame_image.selection().x()+frame_image.selection().width()) / (double) frame.getFastDimension();
    bounds[3] = 1.0 - (double) (frame_image.selection().top()) / (double) frame.getSlowDimension();
    
    glUniform4fv(shared_window->rect_hl_2d_tex_bounds, 1, bounds.toFloat().data());
    
    // The center of the intensity maximum
//    Matrix<double> center(1,4); // left, top, right, bottom
    
//    center[0] = analysis_area.weighted_x() / (double) frame.getFastDimension();
//    center[1] = 1.0 - analysis_area.weighted_y() / (double) frame.getSlowDimension();
    
//    glUniform2fv(shared_window->rect_hl_2d_tex_center, 1, center.toFloat().data());
    
    
    // Set the size of a pixel (in image units)
    GLfloat pixel_size = (1.0f / (float) frame.getFastDimension()) / zoom_matrix[0];
    
    glUniform1f(shared_window->rect_hl_2d_tex_pixel_size, pixel_size);
    

    glVertexAttribPointer(shared_window->rect_hl_2d_tex_fragpos, 2, GL_FLOAT, GL_FALSE, 0, fragpos.data());
    glVertexAttribPointer(shared_window->rect_hl_2d_tex_pos, 2, GL_FLOAT, GL_FALSE, 0, texpos);

    glEnableVertexAttribArray(shared_window->rect_hl_2d_tex_fragpos);
    glEnableVertexAttribArray(shared_window->rect_hl_2d_tex_pos);

    glDrawElements(GL_TRIANGLES,  6,  GL_UNSIGNED_INT,  indices);

    glDisableVertexAttribArray(shared_window->rect_hl_2d_tex_pos);
    glDisableVertexAttribArray(shared_window->rect_hl_2d_tex_fragpos);
    glBindTexture(GL_TEXTURE_2D, 0);

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
    // Change to draw a faded polygon
//    ColorMatrix<float> selection_lines_color(0.0f,0.0f,0.0f,1.0f);
    
    glLineWidth(2.0);

    float x0 = (((qreal) area.left() + 0.5*render_surface->width()) / (qreal) render_surface->width()) * 2.0 - 1.0; // Left
    float x1 = (((qreal) area.x() + area.width()  + 0.5*render_surface->width())/ (qreal) render_surface->width()) * 2.0 - 1.0; // Right
    
    float y0 = (1.0 - (qreal) (area.top() + 0.5*render_surface->height())/ (qreal) render_surface->height()) * 2.0 - 1.0; // Top
    float y1 = (1.0 - (qreal) (area.y() + area.height() + 0.5*render_surface->height())/ (qreal) render_surface->height()) * 2.0 - 1.0; // Bottom
    
    Matrix<GLfloat> selection_lines(8,2);
    selection_lines[0] = x0;
    selection_lines[1] = y0;
    selection_lines[2] = x0;
    selection_lines[3] = y1;
    
    selection_lines[4] = x1;
    selection_lines[5] = y0;
    selection_lines[6] = x1;
    selection_lines[7] = y1;
    
    selection_lines[8] = x0;
    selection_lines[9] = y0;
    selection_lines[10] = x1;
    selection_lines[11] = y0;
    
    selection_lines[12] = x0;
    selection_lines[13] = y1;
    selection_lines[14] = x1;
    selection_lines[15] = y1;
    
    

    setVbo(selections_vbo[0], selection_lines.data(), selection_lines.size(), GL_DYNAMIC_DRAW);

    beginRawGLCalls(painter);

    shared_window->std_2d_col_program->bind();
    glEnableVertexAttribArray(shared_window->std_2d_col_fragpos);

    glUniform4fv(shared_window->std_2d_col_color, 1, color.data());

    glBindBuffer(GL_ARRAY_BUFFER, selections_vbo[0]);
    glVertexAttribPointer(shared_window->std_2d_col_fragpos, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    texture_view_matrix = zoom_matrix*translation_matrix;

    glUniformMatrix4fv(shared_window->std_2d_col_transform, 1, GL_FALSE, texture_view_matrix.colmajor().toFloat().data());

    glDrawArrays(GL_LINES,  0, 8);

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
    selection_lines[0] = x0 - x_offset;
    selection_lines[1] = y1;
    selection_lines[2] = x1 - x_offset;
    selection_lines[3] = y1;
    
    selection_lines[4] = x1 + x_offset;
    selection_lines[5] = y1;
    selection_lines[6] = x2 + x_offset;
    selection_lines[7] = y1;
    
    selection_lines[8] = x1;
    selection_lines[9] = y1 - y_offset;
    selection_lines[10] = x1;
    selection_lines[11] = y0 - y_offset;
    
    selection_lines[12] = x1;
    selection_lines[13] = y1 + y_offset;
    selection_lines[14] = x1;
    selection_lines[15] = y2 + y_offset;
    
    

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


//    k_i.print(2,"k_i");
//    k_f.print(2,"k_f");
//    qDebug() << vecDot(k_f, k_i) << vecDot(k_f, k_i)/(2*k) << acos(vecDot(k_f, k_i)/(k*k));

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
//    float lab_theta = 180*asin(Q[1] / (1.0/frame.wavelength))/pi*0.5;
    
    tip += "Scattering angle (2ϴ) "+QString::number(180*getScatteringAngle(frame, pixel_x, pixel_y)/pi,'f',2)+"°\n";
    
    // Position
    Matrix<double> Q(1,3);
    Q = getScatteringVector(frame, pixel_x, pixel_y);

    tip += "Position (x,y,z) "+QString::number(Q[0],'f',2)+" "+QString::number(Q[1],'f',2)+" "+QString::number(Q[2],'f',2)+" ("+QString::number(sqrt(Q[0]*Q[0] + Q[1]*Q[1] + Q[2]*Q[2]),'f',2)+")\n";
    
    
    // Intensity center
    tip += "Weight center (x,y) "+QString::number(frame_image.selection().weighted_x(),'f',2)+" "+QString::number(frame_image.selection().weighted_y(),'f',2)+"\n";
    
    // Sum
    tip += "Integral "+QString::number(frame_image.selection().integral(),'f',2);
    
    
    QFont font("monospace", 10);
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


//void ImagePreviewWorker::drawTexelOverlay(QPainter *painter)
//{
    /*
     * Draw lines between the texels to better distinguish them.
     * Only happens when the texels occupy a certain number of pixels.
     */

    // Find size of texels in GL coordinates
//    double w = zoom_matrix[0] * 2.0 /(double) render_surface->width();
//    double h = zoom_matrix[0] * 2.0 /(double) render_surface->height();

//    // Find size of texels in pixels
//    double wh_pix = zoom_matrix[0];

//    // The number of texels that fit in
//    double n_texel_x = 2.0 / w;
//    double n_texel_y = 2.0 / h;

//    if((wh_pix > 26.0) && (n_texel_x < 64) && (n_texel_y < 64))
//    {
//        beginRawGLCalls(painter);

//        // Move to resize event
//            Matrix<float> vertical_lines_buf(65,4);
//            for (int i = 0; i < vertical_lines_buf.m(); i++)
//            {
//                vertical_lines_buf[i*4+0] = ((float) (i - (int) vertical_lines_buf.m()/2)) * 2.0 / (float) render_surface->width();
//                vertical_lines_buf[i*4+1] = 1;
//                vertical_lines_buf[i*4+2] = ((float) (i - (int) vertical_lines_buf.m()/2)) * 2.0 / (float) render_surface->width();
//                vertical_lines_buf[i*4+3] = -1;
//            }
//            setVbo(texel_line_vbo[0], vertical_lines_buf.data(), vertical_lines_buf.size(), GL_DYNAMIC_DRAW);

//            Matrix<float> horizontal_lines_buf(65,4);
//            for (int i = 0; i < horizontal_lines_buf.m(); i++)
//            {
//                horizontal_lines_buf[i*4+0] = 1.0;
//                horizontal_lines_buf[i*4+1] = ((float) (i - (int) horizontal_lines_buf.m()/2)) * 2.0 / (float) render_surface->height();
//                horizontal_lines_buf[i*4+2] = -1.0;
//                horizontal_lines_buf[i*4+3] = ((float) (i - (int) horizontal_lines_buf.m()/2)) * 2.0 / (float) render_surface->height();
//            }
//            setVbo(texel_line_vbo[1], horizontal_lines_buf.data(), horizontal_lines_buf.size(), GL_DYNAMIC_DRAW);

//        // Draw lines
//        ColorMatrix<float> texel_line_color(0.0f,0.0f,0.0f,0.7f);

//        glLineWidth(2.0);

//        shared_window->std_2d_col_program->bind();
//        glEnableVertexAttribArray(shared_window->std_2d_col_fragpos);

//        glUniform4fv(shared_window->std_2d_col_color, 1, texel_line_color.data());

//        // Vertical
//        texel_offset_matrix[3] = fmod(translation_matrix[3], 2.0 /(double) render_surface->width());
//        texel_offset_matrix[7] = 0;
//        texel_view_matrix = zoom_matrix*texel_offset_matrix;

//        glBindBuffer(GL_ARRAY_BUFFER, texel_line_vbo[0]);
//        glVertexAttribPointer(shared_window->std_2d_col_fragpos, 2, GL_FLOAT, GL_FALSE, 0, 0);
//        glBindBuffer(GL_ARRAY_BUFFER, 0);

//        glUniformMatrix4fv(shared_window->std_2d_col_transform, 1, GL_FALSE, texel_view_matrix.colmajor().toFloat().data());

//        glDrawArrays(GL_LINES,  0, vertical_lines_buf.m()*2);

//        // Horizontal
//        texel_offset_matrix[3] = 0;
//        texel_offset_matrix[7] = fmod(translation_matrix[7], 2.0 /(double) render_surface->height());
//        texel_view_matrix = zoom_matrix*texel_offset_matrix;

//        glBindBuffer(GL_ARRAY_BUFFER, texel_line_vbo[1]);
//        glVertexAttribPointer(shared_window->std_2d_col_fragpos, 2, GL_FLOAT, GL_FALSE, 0, 0);
//        glBindBuffer(GL_ARRAY_BUFFER, 0);

//        glUniformMatrix4fv(shared_window->std_2d_col_transform, 1, GL_FALSE, texel_view_matrix.colmajor().toFloat().data());

//        glDrawArrays(GL_LINES,  0, horizontal_lines_buf.m()*2);


//        glDisableVertexAttribArray(shared_window->std_2d_col_fragpos);

//        shared_window->std_2d_col_program->release();

//        endRawGLCalls(painter);

//        // Draw intensity numbers over each texel

//        // For each visible vertical line
//        for (int i = 0; i < vertical_lines_buf.m(); i++)
//        {
//            if ((vertical_lines_buf[i*4] >= -1) && (vertical_lines_buf[i*4] <= 1))
//            {
//                // For each visible horizontal line
//                for (int j = 0; j < horizontal_lines_buf.m(); j++)
//                {
//                    if ((horizontal_lines_buf[j*4+1] >= -1) && (horizontal_lines_buf[j*4+1] <= 1))
//                    {
//                        // Find the corresponding texel in the data buffer and display the intensity value at the current position
////                        translation_matrix[3]

//                    }
//                }
//            }
//        }




//    }
//}

void ImagePreviewWorker::setMode(int value)
{
    mode = value;
    calculus();
    refreshDisplay();
    Selection analysis_area = frame_image.selection();
    refreshSelection(&analysis_area);
    frame_image.setSelection(analysis_area);
}

void ImagePreviewWorker::setCorrection(bool value)
{
    isCorrected = (int) value;

    calculus();
    refreshDisplay();
    Selection analysis_area = frame_image.selection();
    refreshSelection(&analysis_area);
    frame_image.setSelection(analysis_area);
}

void ImagePreviewWorker::setAutoBackgroundCorrection(bool value)
{
    isAutoBackgroundCorrectionActive = (int) value;
    
    Selection analysis_area = frame_image.selection();
    Selection background_area = frame_image.background();
    
    refreshBackground(&background_area);
    calculus();
    refreshDisplay();
    
    refreshSelection(&analysis_area);
    frame_image.setSelection(analysis_area);
    frame_image.setBackground(background_area);
}

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

void ImagePreviewWorker::setSelectionAlphaActive(bool value)
{
    isSelectionAlphaActive = value;
    if (value) isSelectionBetaActive = false;
    emit selectionBetaChanged(isSelectionBetaActive);
}

void ImagePreviewWorker::setSelectionBetaActive(bool value)
{
    isSelectionBetaActive = value;
    if (value) isSelectionAlphaActive = false;
    emit selectionAlphaChanged(isSelectionAlphaActive);
}

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
    Q_UNUSED(shift_button);

    float move_scaling = 1.0;
    
    pos = QPoint(x,y);

    if (left_button)
    {
        if (!isSelectionAlphaActive && !isSelectionBetaActive)
        {
            if (shift_button)
            {
                Selection analysis_area = frame_image.selection();
                
                QPoint new_pos = analysis_area.topLeft() + (pos - prev_pos);
                
                analysis_area.moveTopLeft(new_pos);
                        
                frame_image.setSelection(analysis_area);
            }
            else
            {
                double dx = (pos.x() - prev_pos.x())*2.0/(render_surface->width()*zoom_matrix[0]);
                double dy = -(pos.y() - prev_pos.y())*2.0/(render_surface->height()*zoom_matrix[0]);
    
                translation_matrix[3] += dx*move_scaling;
                translation_matrix[7] += dy*move_scaling;
            }
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
    
    
}

void ImagePreviewWorker::metaMouseReleaseEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
    Q_UNUSED(mid_button);
    Q_UNUSED(right_button);
    Q_UNUSED(ctrl_button);
    Q_UNUSED(shift_button);

    pos = QPoint(x,y);
    
    Selection analysis_area = frame_image.selection();
    Selection background_area = frame_image.background();
    
    if (left_button)
    {
        if (isSelectionAlphaActive)
        {
            Matrix<int> pixel = getImagePixel(pos.x(), pos.y());
            
            analysis_area.setTopLeft(QPoint(pixel[0], pixel[1]));
            
            analysis_area = analysis_area.normalized();
        }
        else if (isSelectionBetaActive)
        {
            Matrix<int> pixel = getImagePixel(pos.x(), pos.y());
            
            background_area.setTopLeft(QPoint(pixel[0], pixel[1]));
            
            background_area = background_area.normalized();
        }
    }
    else if (right_button)
    {
        if (isSelectionAlphaActive)
        {
            Matrix<int> pixel = getImagePixel(pos.x(), pos.y());
            
            analysis_area.setBottomRight(QPoint(pixel[0], pixel[1]));
    
            analysis_area = analysis_area.normalized();
        }
        else if (isSelectionBetaActive)
        {
            Matrix<int> pixel = getImagePixel(pos.x(), pos.y());
    
            background_area.setBottomRight(QPoint(pixel[0], pixel[1]));
    
            background_area = background_area.normalized();
        }
    }
    
    refreshSelection(&analysis_area);
    
    frame_image.setSelection(analysis_area);
    
    refreshBackground(&background_area);
    
    frame_image.setBackground(background_area);
    
    emit imageChanged(frame_image);
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
ImagePreviewWorker * ImagePreviewWindow::getWorker()
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



