#include "imagepreview.h"
#include <QPen>
#include <QBrush>
#include <QRect>
#include <QColor>
#include <QDateTime>

ImagePreviewWorker::ImagePreviewWorker(QObject *parent) :
    isInitialized(false),
    isImageTexInitialized(false),
    isTsfTexInitialized(false),
    isCLInitialized(false),
    isFrameValid(false),
    integration_mode(0)
{
    Q_UNUSED(parent);
    
    parameter.reserve(1,16);

    texture_view_matrix.setIdentity(4);
    translation_matrix.setIdentity(4);
    zoom_matrix.setIdentity(4);
//    cursor_translation_matrix.setIdentity(4);

    texel_view_matrix.setIdentity(4);
    texel_offset_matrix.setIdentity(4);

    // Set initial zoom
//    zoom_matrix[0] = 0.5;
//    zoom_matrix[5] = 0.5;
//    zoom_matrix[10] = 0.5;

}

ImagePreviewWorker::~ImagePreviewWorker()
{
    glDeleteBuffers(2, texel_line_vbo);
    glDeleteBuffers(1, &selection_lines_vbo);
}

void ImagePreviewWorker::setSharedWindow(SharedContextWindow * window)
{
    this->shared_window = window;
}

void ImagePreviewWorker::setImageFromPath(QString path)
{
    if (frame.set(path))
    {
        if(frame.readData())
        {
//            frame.setNaive();

//            frame.getData().print();

            isFrameValid = true;

            if (isImageTexInitialized){
                err = clReleaseMemObject(image_tex_cl);
                err |= clReleaseMemObject(source_cl);
                err |= clReleaseMemObject(frame_cl);
                if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
                glDeleteTextures(1, &image_tex_gl);
            }

            Matrix<size_t> image_tex_dim(1,2);
            image_tex_dim[0] = frame.getFastDimension();
            image_tex_dim[1] = frame.getSlowDimension();
            
            glGenTextures(1, &image_tex_gl);
            glBindTexture(GL_TEXTURE_2D, image_tex_gl);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA32F,
                image_tex_dim[0],
                image_tex_dim[1],
                0,
                GL_RGBA,
                GL_FLOAT,
                NULL);
            glBindTexture(GL_TEXTURE_2D, 0);
            
            isImageTexInitialized = true;

            // Share the texture with the OpenCL runtime
            image_tex_cl = clCreateFromGLTexture2D(*context_cl->getContext(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, image_tex_gl, &err);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
            
            // Pass texture to CL kernel
            err = clSetKernelArg(cl_image_preview, 0, sizeof(cl_mem), (void *) &image_tex_cl);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

            // Load data into a CL texture
            cl_image_format source_format;
            source_format.image_channel_order = CL_INTENSITY;
            source_format.image_channel_data_type = CL_FLOAT;

            source_cl = clCreateImage2D ( *context_cl->getContext(),
                CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                &source_format,
                frame.getFastDimension(),
                frame.getSlowDimension(),
                0,
                frame.data().data(),
                &err);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

            frame_cl = clCreateBuffer( *context_cl->getContext(),
                CL_MEM_ALLOC_HOST_PTR,
                frame.getFastDimension()*frame.getSlowDimension()*sizeof(cl_float),
                NULL,
                &err);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
            
            err = clSetKernelArg(cl_image_preview, 1, sizeof(cl_mem), (void *) &source_cl);
            err |= clSetKernelArg(cl_image_preview, 9, sizeof(cl_mem), (void *) &frame_cl);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

            // Thresholds and other parameters essential to the file
            parameter[4] = frame.getFlux();
            parameter[5] = frame.getExpTime();
            parameter[6] = frame.getWavelength();
            parameter[7] = frame.getDetectorDist();
            parameter[8] = frame.getBeamX();
            parameter[9] = frame.getBeamY();
            parameter[10] = frame.getPixSizeX();
            parameter[11] = frame.getPixSizeY();
            
            setParameter(parameter);
            err = clSetKernelArg(cl_image_preview, 6, sizeof(cl_int), &isCorrected);
            err |= clSetKernelArg(cl_image_preview, 7, sizeof(cl_int), &mode);
            err |= clSetKernelArg(cl_image_preview, 8, sizeof(cl_int), &isLog);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
            
            update(frame.getFastDimension(), frame.getSlowDimension());
        }
    }
}

double ImagePreviewWorker::integrate(QRectF rect, DetectorFile file)
{
    // Copy a chunk of GPU memory for further calculations. 
    rect = rect.normalized();
    
    // Prepare kernel parameters
    Matrix<size_t> local_ws(1,2);
    local_ws[0] = 8;
    local_ws[1] = 8;
    
//    local_ws.print(0,"local_ws");
    
    Matrix<size_t> global_ws(1,2);
    global_ws[0] = rect.width() + (local_ws[0] - ((size_t) rect.width())%local_ws[0]);
    global_ws[1] = rect.height() + (local_ws[1] - ((size_t) rect.height())%local_ws[1]);
    
//    global_ws.print(0,"global_ws");
    
    Matrix<int> file_size(1,2);
    file_size[0] = file.getFastDimension();
    file_size[1] = file.getSlowDimension();
    
//    file_size.print(0,"file_size");
    
    Matrix<int> file_origin(1,2);
    file_origin[0] = rect.left();
    file_origin[1] = rect.top();
    
//    file_origin.print(0,"file_origin");
    
    int file_row_pitch = file.getFastDimension();
    
    Matrix<int> selection_origin(1,2);
    selection_origin[0] = 0;
    selection_origin[1] = 0;

//    selection_origin.print(0,"selection_origin");
    
    int selection_row_pitch = rect.width();
    
    Matrix<int> selection_size(1,2);
    selection_size[0] = rect.width();
    selection_size[1] = rect.height();

//    selection_size.print(0,"selection_size");
    
    // Set the size of the cl buffer that will be used to store the data in the marked selection. The padded size is neccessary for the subsequen parallel reduction
    int selection_read_size = rect.width()*rect.height();
    int selection_local_size = 64;
    int selection_global_size = selection_read_size + (selection_read_size % selection_local_size ? selection_local_size - (selection_read_size % selection_local_size) : 0);
    int selection_padded_size = selection_global_size + selection_global_size/selection_local_size;
    
    if(0)
    {
        qDebug() << "selection_read_size   " << selection_read_size;
        qDebug() << "selection_local_size  " << selection_local_size;
        qDebug() << "selection_global_size " << selection_global_size;
        qDebug() << "selection_padded_size " << selection_padded_size;
    }
    
    cl_mem selection_cl = clCreateBuffer( *context_cl->getContext(),
        CL_MEM_ALLOC_HOST_PTR,
        selection_padded_size*sizeof(cl_float),
        NULL,
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    // Set kernel parameters
//    qDebug() << "got here";
    err = clSetKernelArg(context_cl->cl_rect_copy_float,  0, sizeof(cl_mem), (void *) &frame_cl);
    err |= clSetKernelArg(context_cl->cl_rect_copy_float, 1, sizeof(cl_int2), file_size.data());
    err |= clSetKernelArg(context_cl->cl_rect_copy_float, 2, sizeof(cl_int2), file_origin.data());
    err |= clSetKernelArg(context_cl->cl_rect_copy_float, 3, sizeof(int), &file_row_pitch);
    err |= clSetKernelArg(context_cl->cl_rect_copy_float, 4, sizeof(cl_mem), (void *) &selection_cl);
    err |= clSetKernelArg(context_cl->cl_rect_copy_float, 5, sizeof(cl_int2), selection_size.data());
    err |= clSetKernelArg(context_cl->cl_rect_copy_float, 6, sizeof(cl_int2), selection_origin.data());
    err |= clSetKernelArg(context_cl->cl_rect_copy_float, 7, sizeof(int), &selection_row_pitch);
    err |= clSetKernelArg(context_cl->cl_rect_copy_float, 8, sizeof(cl_int2), selection_size.data());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
//    qDebug() << "got here";
    
    
    // Launch the kernel
    err = clEnqueueNDRangeKernel(*context_cl->getCommandQueue(), context_cl->cl_rect_copy_float, 2, NULL, global_ws.data(), local_ws.data(), 0, NULL, NULL);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    err = clFinish(*context_cl->getCommandQueue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    Matrix<float> tmp(rect.height(),rect.width());

    err = clEnqueueReadBuffer (*context_cl->getCommandQueue(),
                                        selection_cl,
                                        CL_TRUE,
                                        0,
                                        (size_t) rect.width()*rect.height()*sizeof(cl_float),
                                        tmp.data(),
                                        0,NULL,NULL);

    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

//    Matrix<float> tmp2(frame.getSlowDimension(),frame.getFastDimension());

//    err = clEnqueueReadBuffer (*context_cl->getCommandQueue(),
//                                        frame_cl,
//                                        CL_TRUE,
//                                        0,
//                                        (size_t) frame.getFastDimension()*frame.getSlowDimension()*sizeof(cl_float),
//                                        tmp2.data(),
//                                        0,NULL,NULL);

//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clFinish(*context_cl->getCommandQueue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

//    tmp2.print(1,"Frame");

    tmp.print(1,"Selection");
    
    float sum = sumGpuArray(selection_cl, selection_read_size, 64);
    
//    qDebug() << sum;
    
    err = clReleaseMemObject(selection_cl);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    
    return sum;
}

float ImagePreviewWorker::sumGpuArray(cl_mem cl_data, unsigned int read_size, size_t work_group_size)
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
    Matrix<size_t> local_size(1,1,work_group_size);
    Matrix<size_t> global_size(1,1);
    unsigned int read_offset = 0;
    unsigned int write_offset;

    global_size[0] = read_size + (read_size % local_size[0] ? local_size[0] - (read_size % local_size[0]) : 0);
    write_offset = global_size[0];
    
    bool forth = true;
    float sum;

    /* Pass arguments to kernel */
    err = clSetKernelArg(context_cl->cl_parallel_reduction, 0, sizeof(cl_mem), (void *) &cl_data);
    err |= clSetKernelArg(context_cl->cl_parallel_reduction, 1, local_size[0]*sizeof(cl_float), NULL);
    err |= clSetKernelArg(context_cl->cl_parallel_reduction, 2, sizeof(cl_uint), &read_size);
    err |= clSetKernelArg(context_cl->cl_parallel_reduction, 3, sizeof(cl_uint), &read_offset);
    err |= clSetKernelArg(context_cl->cl_parallel_reduction, 4, sizeof(cl_uint), &write_offset);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    /* Launch kernel repeatedly until the summing is done */
    while (read_size > 1)
    {
        if(0)
        {
            qDebug() << "Forth        " << forth;
            qDebug() << "Read size    " << read_size;
            qDebug() << "Read offset  " << read_offset;
            qDebug() << "Write offset " << write_offset;
        }
        
        err = clEnqueueNDRangeKernel(*context_cl->getCommandQueue(), context_cl->cl_parallel_reduction, 1, 0, global_size.data(), local_size.data(), 0, NULL, NULL);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        err = clFinish(*context_cl->getCommandQueue());
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        /* Extract the sum */
        err = clEnqueueReadBuffer ( *context_cl->getCommandQueue(),
            cl_data,
            CL_TRUE,
            forth ? global_size[0]*sizeof(cl_float) : 0,
            sizeof(cl_float),
            &sum,
            0, NULL, NULL);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        /* Prepare the kernel parameters for the next iteration */
        forth = !forth;

        // Prepare to read memory in front of the separator and write to the memory behind it
        if (forth)
        {
            read_size = (global_size[0])/local_size[0];
            if (read_size % local_size[0]) global_size[0] = read_size + local_size[0] - (read_size % local_size[0]);
            else global_size[0] = read_size;

            read_offset = 0;
            write_offset = global_size[0];
        }
        // Prepare to read memory behind the separator and write to the memory in front of it
        else
        {
            read_offset = global_size[0];
            write_offset = 0;

            read_size = global_size[0]/local_size[0];
            if (read_size % local_size[0]) global_size[0] = read_size + local_size[0] - (read_size % local_size[0]);
            else global_size[0] = read_size;
        }

        err = clSetKernelArg(context_cl->cl_parallel_reduction, 2, sizeof(cl_uint), &read_size);
        err |= clSetKernelArg(context_cl->cl_parallel_reduction, 3, sizeof(cl_uint), &read_offset);
        err |= clSetKernelArg(context_cl->cl_parallel_reduction, 4, sizeof(cl_uint), &write_offset);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    }

    return sum;
}

void ImagePreviewWorker::integrateSelectedMode()
{


    switch (integration_mode)
    {
        case 0: // Single
            integrateSingle();
            break;

        case 1: // Folder
            break;

        case 2: // All
            break;

        default: // Should not occur
            break;
    }
}

void ImagePreviewWorker::setIntegrationMode(int value)
{
    integration_mode = value;
}

void ImagePreviewWorker::integrateSingle()
{
    if (isFrameValid && (selection.normalized().width() > 0) && (selection.normalized().height() > 0) && (selection.normalized().width() <= frame.getFastDimension()) && (selection.normalized().height() <= frame.getSlowDimension()))
    {
        double value = integrate(selection, frame);
    
        QString str;
                
        str += "# SINGLE FRAME INTEGRATION\n";
        str += "# "+QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm:ss t")+"\n";
        str += "#\n";
        str += frame.info();
        str += "#\n# AREA\n";
        str += QString("# Origin x y [pixels]: "+QString::number(selection.normalized().left())+" "+QString::number(selection.normalized().top())+"\n");
        str += QString("# Region w h [pixels]: "+QString::number(selection.normalized().width())+" "+QString::number(selection.normalized().height())+"\n");
        str += "#\n# Integrated intensity\n";
        str += QString::number(value,'E');
                
        
        emit outputTextChanged(str);
    }
}

void ImagePreviewWorker::update(size_t w, size_t h)
{
    // Aquire shared CL/GL objects
    glFinish();
    err = clEnqueueAcquireGLObjects(*context_cl->getCommandQueue(), 1, &image_tex_cl, 0, 0, 0);
    err |= clEnqueueAcquireGLObjects(*context_cl->getCommandQueue(), 1, &tsf_tex_cl, 0, 0, 0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    // Launch rendering kernel
    Matrix<size_t> local_ws(1,2);
    local_ws[0] = 16;
    local_ws[1] = 16;
    
    Matrix<size_t> global_ws(1,2);
    global_ws[0] = w + (local_ws[0] - w%local_ws[0]);
    global_ws[1] = h + (local_ws[1] - h%local_ws[1]);
    
    Matrix<size_t> area_per_call(1,2);
    area_per_call[0] = 128;
    area_per_call[1] = 128;

    Matrix<size_t> call_offset(1,2);
    call_offset[0] = 0;
    call_offset[1] = 0;

    // Launch the kernel
    for (size_t glb_x = 0; glb_x < global_ws[0]; glb_x += area_per_call[0])
    {
        for (size_t glb_y = 0; glb_y < global_ws[1]; glb_y += area_per_call[1])
        {
            call_offset[0] = glb_x;
            call_offset[1] = glb_y;

            err = clEnqueueNDRangeKernel(*context_cl->getCommandQueue(), cl_image_preview, 2, call_offset.data(), area_per_call.data(), local_ws.data(), 0, NULL, NULL);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        }
    }
    err = clFinish(*context_cl->getCommandQueue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
//    Matrix<size_t> origin(1,3);
//    origin[0] = 0;
//    origin[1] = 0;
//    origin[2] = 0;
    
//    Matrix<size_t> region(1,3);
//    region[0] = frame.getFastDimension();
//    region[1] = frame.getSlowDimension();
//    region[2] = 1;
    
//    region.print(0,"region");
    
//    Matrix<float> tmp(frame.getSlowDimension(), frame.getFastDimension()*4,-3);
    
//    err = clEnqueueReadImage(
//                *context_cl->getCommandQueue(),
//                image_tex_cl, 
//                CL_TRUE, 
//                origin.data(), 
//                region.data(), 
//                (size_t) frame.getFastDimension()*4*4, 
//                0, 
//                tmp.data(), 
//                0, NULL, NULL);
//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
//    err = clFinish(*context_cl->getCommandQueue());
//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
//    tmp.print(2,"texture_cl");
    
    
    // Release shared CL/GL objects
    err = clEnqueueReleaseGLObjects(*context_cl->getCommandQueue(), 1, &image_tex_cl, 0, 0, 0);
    err |= clEnqueueReleaseGLObjects(*context_cl->getCommandQueue(), 1, &tsf_tex_cl, 0, 0, 0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

//    err = clFinish(*context_cl->getCommandQueue());
//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void ImagePreviewWorker::initResourcesCL()
{
    // Build program from OpenCL kernel source
    QStringList paths;
    paths << "cl/image_preview.cl";
//    paths[1] = "cl/mem_functions.cl";
//    paths[2] = "cl/parallel_reduce.cl";

    program = context_cl->createProgram(paths, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    context_cl->buildProgram(&program, "-Werror");

    // Kernel handles
    cl_image_preview = clCreateKernel(program, "imagePreview", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
//    context_cl->cl_rect_copy_float = clCreateKernel(program, "rectCopyFloat", &err);
//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
//    context_cl->cl_parallel_reduction = clCreateKernel(program, "psum", &err);
//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Image sampler
    image_sampler = clCreateSampler(*context_cl->getContext(), false, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_NEAREST, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clSetKernelArg(cl_image_preview, 5, sizeof(cl_sampler), &image_sampler);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Tsf sampler
    tsf_sampler = clCreateSampler(*context_cl->getContext(), true, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR, &err);

    err = clSetKernelArg(cl_image_preview, 4, sizeof(cl_sampler), &tsf_sampler);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Parameters
    parameter_cl = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        parameter.bytes(),
        NULL, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    err = clSetKernelArg(cl_image_preview, 3, sizeof(cl_mem), &parameter_cl);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    isCLInitialized = true;
    
    setParameter(parameter);
}

void ImagePreviewWorker::setTsf(TransferFunction & tsf)
{
    if (isTsfTexInitialized)
    {
        err = clReleaseMemObject(tsf_tex_cl);
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
        tsf.getSplined()->getN(),
        1,
        0,
        GL_RGBA,
        GL_FLOAT,
        tsf.getSplined()->getColMajor().toFloat().data());
    glBindTexture(GL_TEXTURE_2D, 0);

    isTsfTexInitialized = true;

    tsf_tex_cl = clCreateFromGLTexture2D(*context_cl->getContext(), CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, tsf_tex_gl, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clSetKernelArg(cl_image_preview, 2, sizeof(cl_mem), (void *) &tsf_tex_cl);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void ImagePreviewWorker::initialize()
{
    initResourcesCL();

    glGenBuffers(2, texel_line_vbo);
    glGenBuffers(1, &selection_lines_vbo);

    isInitialized = true;

    setTsf(tsf);

    setThresholdNoiseLow(-1e99);
    setThresholdNoiseHigh(1e99);
    setThresholdPostCorrectionLow(-1e99);
    setThresholdPostCorrectionHigh(1e99);
    selection.setCoords(0,0,0,0);
}


void ImagePreviewWorker::setTsfTexture(int value)
{
    rgb_style = value;

    tsf.setColorScheme(rgb_style, alpha_style);
    tsf.setSpline(256);

    if (isInitialized) setTsf(tsf);
    if (isFrameValid) update(frame.getFastDimension(), frame.getSlowDimension());
}
void ImagePreviewWorker::setTsfAlpha(int value)
{
    alpha_style = value;

    tsf.setColorScheme(rgb_style, alpha_style);
    tsf.setSpline(256);

    if (isInitialized) setTsf(tsf);
    if (isFrameValid) update(frame.getFastDimension(), frame.getSlowDimension());
}
void ImagePreviewWorker::setLog(bool value)
{
    isLog = (int) value;

    if (isInitialized)
    {
        err = clSetKernelArg(cl_image_preview, 8, sizeof(cl_int), &isLog);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }

    if (isFrameValid) update(frame.getFastDimension(), frame.getSlowDimension());
}

void ImagePreviewWorker::setDataMin(double value)
{
    parameter[12] = value;
    setParameter(parameter);
    if (isFrameValid) update(frame.getFastDimension(), frame.getSlowDimension());
}
void ImagePreviewWorker::setDataMax(double value)
{
    parameter[13] = value;
    setParameter(parameter);
    if (isFrameValid) update(frame.getFastDimension(), frame.getSlowDimension());
}


void ImagePreviewWorker::setThresholdNoiseLow(double value)
{
    parameter[0] = value;
    setParameter(parameter);
}

void ImagePreviewWorker::setSelection(QRectF rect)
{
    selection = rect;
    
//    qDebug() << "IP selection" << selection << "left" << selection.left() << "top" << selection.top();
}

void ImagePreviewWorker::setThresholdNoiseHigh(double value)
{
    parameter[1] = value;
    setParameter(parameter);
}
void ImagePreviewWorker::setThresholdPostCorrectionLow(double value)
{
    parameter[2] = value;
    setParameter(parameter);
}
void ImagePreviewWorker::setThresholdPostCorrectionHigh(double value)
{
    parameter[3] = value;
    setParameter(parameter);
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
    isRendering = true;

    painter->setRenderHint(QPainter::Antialiasing);
    
    beginRawGLCalls(painter);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    const qreal retinaScale = render_surface->devicePixelRatio();
    glViewport(0, 0, render_surface->width() * retinaScale, render_surface->height() * retinaScale);

    endRawGLCalls(painter);

    drawImage(painter);

//    drawTexelOverlay(painter);

//    drawSelection(painter);

    isRendering = false;

//    QRect minicell_rect(50,50,200,200);

//    QPen pen;
//    pen.setColor(QColor(255,0,0,150));
//    pen.setWidthF(1.0);

//    QBrush brush;
//    brush.setColor(QColor(0,255,0,150));
//    brush.setStyle(Qt::SolidPattern);

//    painter->setPen(pen);
//    painter->setBrush(brush);
//    painter->drawRoundedRect(minicell_rect, 5, 5, Qt::AbsoluteSize);

//    QString resolution_string("ANDREAS L REITEN");
//    QRect resolution_string_rect = emph_fontmetric->boundingRect(resolution_string);
//    resolution_string_rect += QMargins(5,5,5,5);
//    resolution_string_rect.moveBottomLeft(QPoint(5, render_surface->height() - 5));

//    painter->drawRoundedRect(resolution_string_rect, 5, 5, Qt::AbsoluteSize);
//    painter->drawText(100,100,100,100, Qt::AlignCenter, resolution_string);
}

void ImagePreviewWorker::drawImage(QPainter * painter)
{
    beginRawGLCalls(painter);

    shared_window->rect_hl_2d_tex_program->bind();

//    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, image_tex_gl);
    shared_window->rect_hl_2d_tex_program->setUniformValue(shared_window->rect_hl_2d_tex_texture, 0);

//    Matrix<GLfloat> tmp(frame.getSlowDimension(),frame.getFastDimension()*4,-1);
    
//    glGetTexImage(GL_TEXTURE_2D,  0,  GL_RGBA,  GL_FLOAT,  tmp.data());
    
//    if ( glGetError() != GL_NO_ERROR) qFatal(gl_error_cstring(glGetError()));
    
//    glFinish();
    
//    tmp.print(1,"Texture");
    

    QRectF image_rect(QPointF(0,0),QSizeF(frame.getFastDimension(), frame.getSlowDimension()));
    image_rect.moveTopLeft(QPointF((qreal) render_surface->width()*0.5, (qreal) render_surface->height()*0.5));

    Matrix<GLfloat> fragpos = glRect(image_rect);

    GLfloat texpos[] = {
        0.0, 0.0,
        1.0, 0.0,
        1.0, 1.0,
        0.0, 1.0
    };
    
//    GLfloat texpos[] = { // The texpos is flipped along a horizontal mirror plane.
//        0.0, 1.0,
//        1.0, 1.0,
//        1.0, 0.0,
//        0.0, 0.0
//    };
    
//    fragpos.print(2,"fragpos");

    GLuint indices[] = {0,1,3,1,2,3};

    texture_view_matrix = zoom_matrix*translation_matrix;

    glUniformMatrix4fv(shared_window->rect_hl_2d_tex_transform, 1, GL_FALSE, texture_view_matrix.getColMajor().toFloat().data());
    if ( glGetError() != GL_NO_ERROR) qFatal(gl_error_cstring(glGetError()));
    
    // The bounds that enclose the highlighted area of the texture are passed to the shader
//    selection = selection.normalized();
    
    Matrix<double> bounds(1,4); // left, top, right, bottom
    
    bounds[0] = (double) selection.normalized().left() / (double) frame.getFastDimension();
    bounds[1] = 1.0 - (double) (selection.normalized().bottom()) / (double) frame.getSlowDimension();
    bounds[2] = (double) selection.normalized().right() / (double) frame.getFastDimension();
    bounds[3] = 1.0 - (double) (selection.normalized().top()) / (double) frame.getSlowDimension();
    
//    qDebug() << selection.normalized();
//    qDebug() << selection.normalized().left() << selection.normalized().top() << selection.normalized().right() << selection.normalized().bottom();
//    bounds.print(2);
    
    glUniform4fv(shared_window->rect_hl_2d_tex_bounds, 1, bounds.toFloat().data());
    
    
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


void ImagePreviewWorker::drawSelection(QPainter *painter)
{
    // Change to draw a faded polygon
    
//    ColorMatrix<float> selection_rect_color(1.0f,0.2f,0.0f,0.5f);
    
    ColorMatrix<float> selection_lines_color(0.0f,0.0f,0.0f,1.0f);
    
    glLineWidth(2.0);

//    xf = (x / (qreal) render_surface->width()) * 2.0 - 1.0;
//    yf = (1.0 - (y + h)/ (qreal) render_surface->height()) * 2.0 - 1.0;
//    wf = (w / (qreal) render_surface->width()) * 2.0;
//    hf = (h / (qreal) render_surface->height()) * 2.0;
    
    float x0 = (((qreal) selection.normalized().left() + 0.5*render_surface->width()) / (qreal) render_surface->width()) * 2.0 - 1.0;
    float y0 = (1.0 - (qreal) (selection.bottom())/ (qreal) render_surface->height()) * 2.0 - 1.0;
    float x1 = (((qreal) selection.normalized().right()  + 0.5*render_surface->width())/ (qreal) render_surface->width()) * 2.0 - 1.0;
    float y1 = (1.0 - (qreal) (selection.top())/ (qreal) render_surface->height()) * 2.0 - 1.0;
    
    Matrix<GLfloat> selection_lines(8,2);
    selection_lines[0] = x0;
    selection_lines[1] = 1.0;
    selection_lines[2] = x0;
    selection_lines[3] = 0.0;
    
    selection_lines[4] = x1;
    selection_lines[5] = 1.0;
    selection_lines[6] = x1;
    selection_lines[7] = 0.0;
    
    selection_lines[8] = 1.0;
    selection_lines[9] = y0;
    selection_lines[10] = 0.0;
    selection_lines[11] = y0;
    
    selection_lines[12] = 1.0;
    selection_lines[13] = y1;
    selection_lines[14] = 0.0;
    selection_lines[15] = y1;
    
    

    setVbo(selection_lines_vbo, selection_lines.data(), selection_lines.size(), GL_DYNAMIC_DRAW);

    beginRawGLCalls(painter);

    shared_window->std_2d_col_program->bind();
    glEnableVertexAttribArray(shared_window->std_2d_col_fragpos);

    glUniform4fv(shared_window->std_2d_col_color, 1, selection_lines_color.data());

    glBindBuffer(GL_ARRAY_BUFFER, selection_lines_vbo);
    glVertexAttribPointer(shared_window->std_2d_col_fragpos, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    texture_view_matrix = zoom_matrix*translation_matrix;

    glUniformMatrix4fv(shared_window->std_2d_col_transform, 1, GL_FALSE, texture_view_matrix.getColMajor().toFloat().data());

    glDrawArrays(GL_LINES,  0, 8);

    glDisableVertexAttribArray(shared_window->std_2d_col_fragpos);

    shared_window->std_2d_col_program->release();

    endRawGLCalls(painter);
}

void ImagePreviewWorker::drawTexelOverlay(QPainter *painter)
{
    /*
     * Draw lines between the texels to better distinguish them.
     * Only happens when the texels occupy a certain number of pixels.
     */

    // Find size of texels in GL coordinates
    double w = zoom_matrix[0] * 2.0 /(double) render_surface->width();
    double h = zoom_matrix[0] * 2.0 /(double) render_surface->height();

    // Find size of texels in pixels
    double wh_pix = zoom_matrix[0];

    // The number of texels that fit in
    double n_texel_x = 2.0 / w;
    double n_texel_y = 2.0 / h;

    if((wh_pix > 26.0) && (n_texel_x < 64) && (n_texel_y < 64))
    {
        beginRawGLCalls(painter);

        // Move to resize event
            Matrix<float> vertical_lines_buf(65,4);
            for (int i = 0; i < vertical_lines_buf.getM(); i++)
            {
                vertical_lines_buf[i*4+0] = ((float) (i - (int) vertical_lines_buf.getM()/2)) * 2.0 / (float) render_surface->width(); // The added 0.5 is due to image dimension being odd in test cases...
                vertical_lines_buf[i*4+1] = 1;
                vertical_lines_buf[i*4+2] = ((float) (i - (int) vertical_lines_buf.getM()/2)) * 2.0 / (float) render_surface->width();
                vertical_lines_buf[i*4+3] = -1;
            }
            setVbo(texel_line_vbo[0], vertical_lines_buf.data(), vertical_lines_buf.size(), GL_DYNAMIC_DRAW);

            Matrix<float> horizontal_lines_buf(65,4);
            for (int i = 0; i < horizontal_lines_buf.getM(); i++)
            {
                horizontal_lines_buf[i*4+0] = 1.0;
                horizontal_lines_buf[i*4+1] = ((float) (i - (int) horizontal_lines_buf.getM()/2)) * 2.0 / (float) render_surface->height();
                horizontal_lines_buf[i*4+2] = -1.0;
                horizontal_lines_buf[i*4+3] = ((float) (i - (int) horizontal_lines_buf.getM()/2)) * 2.0 / (float) render_surface->height();
            }
            setVbo(texel_line_vbo[1], horizontal_lines_buf.data(), horizontal_lines_buf.size(), GL_DYNAMIC_DRAW);

        // Draw lines
        ColorMatrix<float> texel_line_color(0.0f,0.0f,0.0f,0.7f);

        glLineWidth(2.0);

        shared_window->std_2d_col_program->bind();
        glEnableVertexAttribArray(shared_window->std_2d_col_fragpos);

        glUniform4fv(shared_window->std_2d_col_color, 1, texel_line_color.data());

        // Vertical
        texel_offset_matrix[3] = fmod(translation_matrix[3], 2.0 /(double) render_surface->width());
        texel_offset_matrix[7] = 0;
        texel_view_matrix = zoom_matrix*texel_offset_matrix;

        glBindBuffer(GL_ARRAY_BUFFER, texel_line_vbo[0]);
        glVertexAttribPointer(shared_window->std_2d_col_fragpos, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glUniformMatrix4fv(shared_window->std_2d_col_transform, 1, GL_FALSE, texel_view_matrix.getColMajor().toFloat().data());

        glDrawArrays(GL_LINES,  0, vertical_lines_buf.getM()*2);

        // Horizontal
        texel_offset_matrix[3] = 0;
        texel_offset_matrix[7] = fmod(translation_matrix[7], 2.0 /(double) render_surface->height());
        texel_view_matrix = zoom_matrix*texel_offset_matrix;

        glBindBuffer(GL_ARRAY_BUFFER, texel_line_vbo[1]);
        glVertexAttribPointer(shared_window->std_2d_col_fragpos, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glUniformMatrix4fv(shared_window->std_2d_col_transform, 1, GL_FALSE, texel_view_matrix.getColMajor().toFloat().data());

        glDrawArrays(GL_LINES,  0, horizontal_lines_buf.getM()*2);


        glDisableVertexAttribArray(shared_window->std_2d_col_fragpos);

        shared_window->std_2d_col_program->release();

        endRawGLCalls(painter);

        // Draw intensity numbers over each texel

        // For each visible vertical line
        for (int i = 0; i < vertical_lines_buf.getM(); i++)
        {
            if ((vertical_lines_buf[i*4] >= -1) && (vertical_lines_buf[i*4] <= 1))
            {
                // For each visible horizontal line
                for (int j = 0; j < horizontal_lines_buf.getM(); j++)
                {
                    if ((horizontal_lines_buf[j*4+1] >= -1) && (horizontal_lines_buf[j*4+1] <= 1))
                    {
                        // Find the corresponding texel in the data buffer and display the intensity value at the current position
//                        translation_matrix[3]

                    }
                }
            }
        }




    }
}

void ImagePreviewWorker::setMode(int value)
{
    mode = value;

    if (isInitialized)
    {
        err = clSetKernelArg(cl_image_preview, 7, sizeof(cl_int), &mode);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }

    if (isFrameValid) update(frame.getFastDimension(), frame.getSlowDimension());
}

void ImagePreviewWorker::setCorrection(bool value)
{
    isCorrected = (int) value;

    if (isInitialized)
    {
        err = clSetKernelArg(cl_image_preview, 6, sizeof(cl_int), &isCorrected);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }

    if (isFrameValid) update(frame.getFastDimension(), frame.getSlowDimension());
}

void ImagePreviewWorker::setParameter(Matrix<float> & data)
{
    if (0)
    {
        qDebug() << "th_a_low" << parameter[0];
        qDebug() << "th_a_high" << parameter[1];
        qDebug() << "th_b_low" << parameter[2];
        qDebug() << "th_b_high" << parameter[3];
        qDebug() << "flux" << parameter[4];
        qDebug() << "exp_time" << parameter[5];
        qDebug() << "wavelength" << parameter[6];
        qDebug() << "det_dist" << parameter[7];
        qDebug() << "beam_x" << parameter[8];
        qDebug() << "beam_y" << parameter[9];
        qDebug() << "pix_size_x" << parameter[10];
        qDebug() << "pix_size_y" << parameter[11];
        qDebug() << "intensity_min" << parameter[12];
        qDebug() << "intensity_max" << parameter[13];
    }

    if (isCLInitialized)
    {
        err = clEnqueueWriteBuffer (*context_cl->getCommandQueue(),
            parameter_cl,
            CL_TRUE,
            0,
            data.bytes(),
            data.data(),
            0,0,0);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
}

void ImagePreviewWorker::setSelectionActive(bool value)
{
    isSelectionActive = value;
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
    image_pos_gl = texture_view_matrix.getInverse4x4()*screen_pos_gl;
    
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
    if(shift_button) move_scaling = 5.0;
    else if(ctrl_button) move_scaling = 0.2;

    if (left_button && !isSelectionActive && (isRendering == false))
    {
        double dx = (x - last_mouse_pos_x)*2.0/(render_surface->width()*zoom_matrix[0]);
        double dy = -(y - last_mouse_pos_y)*2.0/(render_surface->height()*zoom_matrix[0]);

        translation_matrix[3] += dx*move_scaling;
        translation_matrix[7] += dy*move_scaling;
    }
    else if (left_button && isSelectionActive)
    {
        Matrix<int> pixel = getImagePixel(x, y);
        
        selection.setBottomRight(QPointF(pixel[0]+1, pixel[1]+1));
        // QPointF(pixel[0] + (qreal) render_surface->width()*0.5, pixel[1] + (qreal) render_surface->height()*0.5);
        
    }

    last_mouse_pos_x = x;
    last_mouse_pos_y = y;
}

void ImagePreviewWorker::metaMousePressEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(mid_button);
    Q_UNUSED(right_button);
    Q_UNUSED(ctrl_button);
    Q_UNUSED(shift_button);

    if (isSelectionActive && left_button)
    {
        Matrix<int> pixel = getImagePixel(x, y);
        
        selection.setTopLeft(QPointF(pixel[0], pixel[1]));
        selection.setBottomRight(QPointF(pixel[0], pixel[1]));
    }
}

void ImagePreviewWorker::metaMouseReleaseEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(mid_button);
    Q_UNUSED(right_button);
    Q_UNUSED(ctrl_button);
    Q_UNUSED(shift_button);

    if (isSelectionActive)
    {
        Matrix<int> pixel = getImagePixel(x, y);
        
        selection.setBottomRight(QPointF(pixel[0]+1, pixel[1]+1));
        
        emit selectionChanged(selection);
    }
    
//    qDebug() << "IP selection" << selection.normalized() << "left" << selection.normalized().left() << "top" << selection.normalized().top();
}
void ImagePreviewWorker::wheelEvent(QWheelEvent* ev)
{
    Q_UNUSED(ev);

    float move_scaling = 1.0;
    if(ev->modifiers() & Qt::ShiftModifier) move_scaling = 5.0;
    else if(ev->modifiers() & Qt::ControlModifier) move_scaling = 0.2;

    double delta = move_scaling*((double)ev->delta())*0.0008;

    if ((zoom_matrix[0] + zoom_matrix[0]*delta < 256) && (isRendering == false))
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
    gl_worker->setGLContext(context_gl);
    gl_worker->setOpenCLContext(context_cl);
    gl_worker->setSharedWindow(shared_window);
    gl_worker->setMultiThreading(isMultiThreaded);

    if (isMultiThreaded)
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
    if (isWorkerBusy)
    {
        if (isAnimating) renderLater();
        return;
    }
    else
    {
        if (!isInitialized) initializeWorker();

        if (gl_worker)
        {
            if (isMultiThreaded)
            {
                isWorkerBusy = true;
                worker_thread->start();
                emit render();
            }
            else
            {
                context_gl->makeCurrent(this);
//                gl_worker->process();
                emit render();
            }

        }
    }
    if (isAnimating) renderLater();
}
