#include "imagepreview.h"
#include <QPen>
#include <QBrush>
#include <QRect>
#include <QColor>

ImagePreviewWorker::ImagePreviewWorker(QObject *parent) :
    isInitialized(false),
    isImageTexInitialized(false),
    isTsfTexInitialized(false),
    isCLInitialized(false),
    isFrameValid(false)
{
    Q_UNUSED(parent);
    
    parameter.reserve(1,16);

    texture_view_matrix.setIdentity(4);
    translation_matrix.setIdentity(4);
    zoom_matrix.setIdentity(4);
    cursor_translation_matrix.setIdentity(4);

    texel_view_matrix.setIdentity(4);
    texel_offset_matrix.setIdentity(4);

    // Set initial zoom
    zoom_matrix[0] = 0.5;
    zoom_matrix[5] = 0.5;
    zoom_matrix[10] = 0.5;
}

ImagePreviewWorker::~ImagePreviewWorker()
{
    glDeleteBuffers(2, texel_line_vbo);
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
            isFrameValid = true;

            if (isImageTexInitialized){
                err = clReleaseMemObject(image_tex_cl);
                err |= clReleaseMemObject(source_cl);
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

            // Convert to CL texture
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
            
            err = clSetKernelArg(cl_image_preview, 1, sizeof(cl_mem), (void *) &source_cl);
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

    // Release shared CL/GL objects
    err = clEnqueueReleaseGLObjects(*context_cl->getCommandQueue(), 1, &image_tex_cl, 0, 0, 0);
    err |= clEnqueueReleaseGLObjects(*context_cl->getCommandQueue(), 1, &tsf_tex_cl, 0, 0, 0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clFinish(*context_cl->getCommandQueue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void ImagePreviewWorker::initResourcesCL()
{
    // Build program from OpenCL kernel source
    Matrix<const char *> paths(1,1);
    paths[0] = "cl/image_preview.cl";

    program = context_cl->createProgram(&paths, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    context_cl->buildProgram(&program, "-Werror");

    // Kernel handles
    cl_image_preview = clCreateKernel(program, "imagePreview", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

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

    isInitialized = true;

    setTsf(tsf);

//    setMode(0);
    setThresholdNoiseLow(-1e99);
    setThresholdNoiseHigh(1e99);
    setThresholdPostCorrectionLow(-1e99);
    setThresholdPostCorrectionHigh(1e99);
//    setDataMin(1);
//    setDataMax(1000);
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

    drawTexelOverlay(painter);

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

    shared_window->std_2d_tex_program->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, image_tex_gl);
    shared_window->std_2d_tex_program->setUniformValue(shared_window->std_2d_tex_texture, 0);


    QRectF image_rect(QPointF(render_surface->width()*0.5-frame.getFastDimension()*0.5,render_surface->height()*0.5-frame.getSlowDimension()*0.5),QSizeF(frame.getFastDimension(), frame.getSlowDimension()));
    Matrix<GLfloat> fragpos = glRect(image_rect);

    GLfloat texpos[] = {
        0.0, 0.0,
        1.0, 0.0,
        1.0, 1.0,
        0.0, 1.0
    };

    GLuint indices[] = {0,1,3,1,2,3};

    texture_view_matrix = zoom_matrix*cursor_translation_matrix*translation_matrix;

    glUniformMatrix4fv(shared_window->std_2d_tex_transform, 1, GL_FALSE, texture_view_matrix.getColMajor().toFloat().data());

    glVertexAttribPointer(shared_window->std_2d_tex_fragpos, 2, GL_FLOAT, GL_FALSE, 0, fragpos.data());
    glVertexAttribPointer(shared_window->std_2d_tex_pos, 2, GL_FLOAT, GL_FALSE, 0, texpos);

    glEnableVertexAttribArray(shared_window->std_2d_tex_fragpos);
    glEnableVertexAttribArray(shared_window->std_2d_tex_pos);

    glDrawElements(GL_TRIANGLES,  6,  GL_UNSIGNED_INT,  indices);

    glDisableVertexAttribArray(shared_window->std_2d_tex_pos);
    glDisableVertexAttribArray(shared_window->std_2d_tex_fragpos);
    glBindTexture(GL_TEXTURE_2D, 0);

    shared_window->std_2d_tex_program->release();

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
                vertical_lines_buf[i*4+0] = (0.5 + (float) (i - (int) vertical_lines_buf.getM()/2)) * 2.0 / (float) render_surface->width(); // The added 0.5 is due to image dimension being odd in test cases...
                vertical_lines_buf[i*4+1] = 1;
                vertical_lines_buf[i*4+2] = (0.5 + (float) (i - (int) vertical_lines_buf.getM()/2)) * 2.0 / (float) render_surface->width();
                vertical_lines_buf[i*4+3] = -1;
            }
            setVbo(texel_line_vbo[0], vertical_lines_buf.data(), vertical_lines_buf.size(), GL_DYNAMIC_DRAW);

            Matrix<float> horizontal_lines_buf(65,4);
            for (int i = 0; i < horizontal_lines_buf.getM(); i++)
            {
                horizontal_lines_buf[i*4+0] = 1.0;
                horizontal_lines_buf[i*4+1] = (0.5 + (float) (i - (int) horizontal_lines_buf.getM()/2)) * 2.0 / (float) render_surface->height();
                horizontal_lines_buf[i*4+2] = -1.0;
                horizontal_lines_buf[i*4+3] = (0.5 + (float) (i - (int) horizontal_lines_buf.getM()/2)) * 2.0 / (float) render_surface->height();
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
        texel_view_matrix = zoom_matrix*cursor_translation_matrix*texel_offset_matrix;

        glBindBuffer(GL_ARRAY_BUFFER, texel_line_vbo[0]);
        glVertexAttribPointer(shared_window->std_2d_col_fragpos, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glUniformMatrix4fv(shared_window->std_2d_col_transform, 1, GL_FALSE, texel_view_matrix.getColMajor().toFloat().data());

        glDrawArrays(GL_LINES,  0, vertical_lines_buf.getM()*2);

        // Horizontal
        texel_offset_matrix[3] = 0;
        texel_offset_matrix[7] = fmod(translation_matrix[7], 2.0 /(double) render_surface->height());
        texel_view_matrix = zoom_matrix*cursor_translation_matrix*texel_offset_matrix;

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

void ImagePreviewWorker::metaMouseMoveEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
    Q_UNUSED(mid_button);
    Q_UNUSED(right_button);
    Q_UNUSED(ctrl_button);
    Q_UNUSED(shift_button);

    float move_scaling = 1.0;
    if(shift_button) move_scaling = 5.0;
    else if(ctrl_button) move_scaling = 0.2;

    if (left_button && (isRendering == false))
    {
        double dx = (x - last_mouse_pos_x)*2.0/(render_surface->width()*zoom_matrix[0]);
        double dy = -(y - last_mouse_pos_y)*2.0/(render_surface->height()*zoom_matrix[0]);

        translation_matrix[3] += dx*move_scaling;
        translation_matrix[7] += dy*move_scaling;
    }

    last_mouse_pos_x = x;
    last_mouse_pos_y = y;
}
void ImagePreviewWorker::metaMousePressEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(left_button);
    Q_UNUSED(mid_button);
    Q_UNUSED(right_button);
    Q_UNUSED(ctrl_button);
    Q_UNUSED(shift_button);
}
void ImagePreviewWorker::metaMouseReleaseEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(left_button);
    Q_UNUSED(mid_button);
    Q_UNUSED(right_button);
    Q_UNUSED(ctrl_button);
    Q_UNUSED(shift_button);
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
