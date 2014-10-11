#ifndef IMAGEPREVIEW_H
#define IMAGEPREVIEW_H

/*
 * This class can open a file from another thread and display its contents as a texture using QOpenGLPainter or raw GL calls
 * */

#include <QLibrary>
#include <CL/opencl.h>

#include "../../qxopengl/qxopengllib.h"
#include "../../qxfile/qxfilelib.h"
#include "../../qxmath/qxmathlib.h"


class ImagePreviewWorker : public OpenGLWorker
{
    Q_OBJECT
public:
    explicit ImagePreviewWorker(QObject *parent = 0);
    ~ImagePreviewWorker();
    void setSharedWindow(SharedContextWindow * window);

signals:
    void resultFinished(QString str);
    void selectionAlphaChanged(bool value);
    void selectionBetaChanged(bool value);
    void noiseLowChanged(double value);
    void imageChanged(ImageInfo image);
    
public slots:
    void setMode(int value);
    void setThresholdNoiseLow(double value);
    void setThresholdNoiseHigh(double value);
    void setThresholdPostCorrectionLow(double value);
    void setThresholdPostCorrectionHigh(double value);
    void setTsfTexture(int value);
    void setTsfAlpha(int value);
    void setLog(bool value);
    void setCorrection(bool value);
    void setAutoBackgroundCorrection(bool value);
    void setDataMin(double value);
    void setDataMax(double value);
    void calculus();

    void metaMouseMoveEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMousePressEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMouseReleaseEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void wheelEvent(QWheelEvent* ev);
    void resizeEvent(QResizeEvent * ev);
    void setFrame(ImageInfo image);
    void setSelectionAlphaActive(bool value);
    void setSelectionBetaActive(bool value);
    void centerImage();
    void analyzeSingle(ImageInfo image);
    void analyzeFolder(ImageFolder folder);
    void analyzeSet(FolderSet set);
    
    void peakHuntSingle(ImageInfo image);
    void peakHuntFolder(ImageFolder folder);
    void peakHuntSet(FolderSet set);
    
    void showWeightCenter(bool value);
    
private:
    
    // GPU functions
    void imageCalcuclus(cl_mem data_buf_cl, cl_mem out_buf_cl, Matrix<float> &param, Matrix<size_t> &image_size, Matrix<size_t> &local_ws, int correction, float mean, float deviation, int task);
    
    void imageDisplay(cl_mem data_buf_cl, cl_mem frame_image_cl, cl_mem tsf_image_cl, Matrix<float> &data_limit, Matrix<size_t> &image_size, Matrix<size_t> & local_ws, cl_sampler tsf_sampler, int log);
    
    void copyBufferRect(cl_mem cl_buffer, cl_mem cl_copy, Matrix<size_t> &buffer_size, Matrix<size_t> &buffer_origin, Matrix<size_t> &copy_size, Matrix<size_t> &copy_origin, Matrix<size_t> &local_ws);
    
    float sumGpuArray(cl_mem cl_data, unsigned int read_size, Matrix<size_t> &local_ws);
    
    void selectionCalculus(Selection *area, cl_mem image_data_cl, cl_mem image_pos_weight_x_cl_new, cl_mem image_pos_weight_y_cl_new, Matrix<size_t> &image_size, Matrix<size_t> &local_ws);
    
    // Convenience 
    void refreshDisplay();
    void refreshSelection(Selection *area);
    void refreshBackground(Selection *area);
    
    // GPU buffer management
    void maintainImageTexture(Matrix<size_t> &image_size);
    void clMaintainImageBuffers(Matrix<size_t> &image_size);
    
    // GPU buffers
    cl_mem image_data_raw_cl;
    cl_mem image_data_corrected_cl;
    cl_mem image_data_variance_cl;
    cl_mem image_data_skewness_cl;
    cl_mem image_data_weight_x_cl;
    cl_mem image_data_weight_y_cl;
    cl_mem image_data_generic_cl;
    
    // Misc
    QString integrationFrameString(DetectorFile &f, ImageInfo &image);
    
    SharedContextWindow * shared_window;

    cl_int err;
    cl_program program;
    cl_kernel cl_display_image;
    cl_kernel cl_image_calculus;
    cl_mem image_tex_cl;
    cl_mem source_cl;
    cl_mem tsf_tex_cl;
    cl_mem parameter_cl;
    cl_mem image_intensity_cl;
    cl_mem image_pos_weight_x_cl;
    cl_mem image_pos_weight_y_cl;
    
    cl_sampler tsf_sampler;
    cl_sampler image_sampler;

    TransferFunction tsf;
    int rgb_style, alpha_style;

    GLuint image_tex_gl;
    GLuint tsf_tex_gl;
    Matrix<size_t> image_tex_size;
    Matrix<size_t> image_buffer_size;
    
    // Eventually merge the following two objects into a single class, or at least name them appropriately
    DetectorFile frame;
    ImageInfo frame_image;

    void initOpenCL();
    void setParameter(Matrix<float> &data);
    void setTsf(TransferFunction & tsf);
    Matrix<double> getScatteringVector(DetectorFile & f, double x, double y);
    double getScatteringAngle(DetectorFile & f, double x, double y);
    Matrix<float> parameter;
    
    void beginRawGLCalls(QPainter * painter);
    void endRawGLCalls(QPainter * painter);

    // Draw
    void drawImage(QPainter *painter);
    void drawSelection(Selection area, QPainter *painter, Matrix<float> &color);
    void drawWeightpoint(Selection area, QPainter *painter, Matrix<float> &color);
    void drawToolTip(QPainter * painter);

    // Boolean checks
    bool isImageTexInitialized;
    bool isTsfTexInitialized;
    bool isCLInitialized;
    bool isFrameValid;
    bool isWeightCenterActive;
    bool isAutoBackgroundCorrectionActive;

    Matrix<double> texture_view_matrix; // Used to draw main texture
    Matrix<double> translation_matrix;
    Matrix<double> zoom_matrix;
    
    Matrix<double> texel_view_matrix; // Used for texel overlay
    Matrix<double> texel_offset_matrix;

    // Mouse
    QPoint pos;
    QPoint prev_pos;
    Matrix<int> prev_pixel;

    // Display
    int isLog;
    int isCorrected;
    int mode;

    // Selection
    GLuint selections_vbo[5];
    GLuint weightpoints_vbo[5];
    bool isSelectionAlphaActive;
    bool isSelectionBetaActive;
    Matrix<int> getImagePixel(int x, int y);

    typedef cl_int (*PROTOTYPE_QOpenCLGetPlatformIDs)(  	cl_uint num_entries,
                                                            cl_platform_id *platforms,
                                                            cl_uint *num_platforms);

    typedef cl_int (*PROTOTYPE_QOpenCLGetDeviceIDs)(        cl_platform_id platform,
                                                            cl_device_type device_type,
                                                            cl_uint num_entries,
                                                            cl_device_id *device,
                                                            cl_uint *num_devices);

    typedef cl_int (*PROTOTYPE_QOpenCLGetPlatformInfo)( 	cl_platform_id platform,
                                                            cl_platform_info param_name,
                                                            size_t param_value_size,
                                                            void *param_value,
                                                            size_t *param_value_size_ret);

    typedef cl_int (*PROTOTYPE_QOpenCLGetDeviceInfo)(       cl_device_id device,
                                                            cl_device_info param_name,
                                                            size_t param_value_size,
                                                            void *param_value,
                                                            size_t *param_value_size_ret);

    typedef cl_program (*PROTOTYPE_QOpenCLCreateProgramWithSource)( 	cl_context context,
                                                                    cl_uint count,
                                                                    const char **strings,
                                                                    const size_t *lengths,
                                                                    cl_int *errcode_ret);
    typedef cl_int (*PROTOTYPE_QOpenCLGetProgramBuildInfo)( 	cl_program  program,
                                                                cl_device_id  device,
                                                                cl_program_build_info  param_name,
                                                                size_t  param_value_size,
                                                                void  *param_value,
                                                                size_t  *param_value_size_ret);
    typedef cl_context (*PROTOTYPE_QOpenCLCreateContext)( 	cl_context_properties *properties,
                                                        cl_uint num_devices,
                                                        const cl_device_id *devices,
                                                        void *pfn_notify (
                                                        const char *errinfo,
                                                        const void *private_info,
                                                        size_t cb,
                                                        void *user_data),
                                                        void *user_data,
                                                        cl_int *errcode_ret);

    typedef cl_command_queue (*PROTOTYPE_QOpenCLCreateCommandQueue)( 	cl_context context,
                                                    cl_device_id device,
                                                    cl_command_queue_properties properties,
                                                    cl_int *errcode_ret);

    typedef cl_int (*PROTOTYPE_QOpenCLSetKernelArg) ( 	cl_kernel kernel,
                                                        cl_uint arg_index,
                                                        size_t arg_size,
                                                        const void *arg_value);

    typedef cl_int (*PROTOTYPE_QOpenCLEnqueueNDRangeKernel)( 	cl_command_queue command_queue,
                                                                cl_kernel kernel,
                                                                cl_uint work_dim,
                                                                const size_t *global_work_offset,
                                                                const size_t *global_work_size,
                                                                const size_t *local_work_size,
                                                                cl_uint num_events_in_wait_list,
                                                                const cl_event *event_wait_list,
                                                                cl_event *event);

    typedef cl_int (*PROTOTYPE_QOpenCLFinish)( 	cl_command_queue command_queue);

    typedef cl_int (*PROTOTYPE_QOpenCLEnqueueAcquireGLObjects)( 	cl_command_queue command_queue,
                                                                    cl_uint num_objects,
                                                                    const cl_mem *mem_objects,
                                                                    cl_uint num_events_in_wait_list,
                                                                    const cl_event *event_wait_list,
                                                                    cl_event *event);

    typedef cl_int (*PROTOTYPE_QOpenCLEnqueueReleaseGLObjects)( 	cl_command_queue command_queue,
                                                                    cl_uint num_objects,
                                                                    const cl_mem *mem_objects,
                                                                    cl_uint num_events_in_wait_list,
                                                                    const cl_event *event_wait_list,
                                                                    cl_event *event);

    typedef cl_int (*PROTOTYPE_QOpenCLEnqueueReadBuffer)( 	cl_command_queue command_queue,
                                                            cl_mem buffer,
                                                            cl_bool blocking_read,
                                                            size_t offset,
                                                            size_t cb,
                                                            void *ptr,
                                                            cl_uint num_events_in_wait_list,
                                                            const cl_event *event_wait_list,
                                                            cl_event *event);

    typedef cl_mem (*PROTOTYPE_QOpenCLCreateBuffer) ( 	cl_context context,
                                                        cl_mem_flags flags,
                                                        size_t size,
                                                        void *host_ptr,
                                                        cl_int *errcode_ret);

    typedef cl_int (*PROTOTYPE_QOpenCLReleaseMemObject) ( 	cl_mem memobj);

    typedef cl_mem (*PROTOTYPE_QOpenCLCreateFromGLTexture2D) ( 	cl_context context,
                                                                cl_mem_flags flags,
                                                                GLenum texture_target,
                                                                GLint miplevel,
                                                                GLuint texture,
                                                                cl_int *errcode_ret);

    typedef cl_sampler (*PROTOTYPE_QOpenCLCreateSampler)( 	cl_context context,
                                                        cl_bool normalized_coords,
                                                        cl_addressing_mode addressing_mode,
                                                        cl_filter_mode filter_mode,
                                                        cl_int *errcode_ret);

    typedef cl_int (*PROTOTYPE_QOpenCLEnqueueWriteBuffer) ( 	cl_command_queue command_queue,
                                                                cl_mem buffer,
                                                                cl_bool blocking_write,
                                                                size_t offset,
                                                                size_t cb,
                                                                const void *ptr,
                                                                cl_uint num_events_in_wait_list,
                                                                const cl_event *event_wait_list,
                                                                cl_event *event);

    typedef cl_kernel (*PROTOTYPE_QOpenCLCreateKernel) ( 	cl_program  program,
                                                        const char *kernel_name,
                                                        cl_int *errcode_ret);


    PROTOTYPE_QOpenCLSetKernelArg QOpenCLSetKernelArg; // OK
    PROTOTYPE_QOpenCLEnqueueNDRangeKernel QOpenCLEnqueueNDRangeKernel; // OK
    PROTOTYPE_QOpenCLFinish QOpenCLFinish; // OK
    PROTOTYPE_QOpenCLEnqueueAcquireGLObjects QOpenCLEnqueueAcquireGLObjects;
    PROTOTYPE_QOpenCLEnqueueReleaseGLObjects QOpenCLEnqueueReleaseGLObjects;
    PROTOTYPE_QOpenCLEnqueueReadBuffer QOpenCLEnqueueReadBuffer; // OK
    PROTOTYPE_QOpenCLCreateBuffer QOpenCLCreateBuffer;
    PROTOTYPE_QOpenCLReleaseMemObject QOpenCLReleaseMemObject;
    PROTOTYPE_QOpenCLCreateFromGLTexture2D QOpenCLCreateFromGLTexture2D;
    PROTOTYPE_QOpenCLCreateSampler QOpenCLCreateSampler;
    PROTOTYPE_QOpenCLEnqueueWriteBuffer QOpenCLEnqueueWriteBuffer;
    PROTOTYPE_QOpenCLCreateKernel QOpenCLCreateKernel;
    PROTOTYPE_QOpenCLGetProgramBuildInfo QOpenCLGetProgramBuildInfo;
    PROTOTYPE_QOpenCLCreateContext QOpenCLCreateContext;
    PROTOTYPE_QOpenCLCreateCommandQueue QOpenCLCreateCommandQueue;
    PROTOTYPE_QOpenCLCreateProgramWithSource QOpenCLCreateProgramWithSource;
    PROTOTYPE_QOpenCLGetPlatformIDs QOpenCLGetPlatformIDs;
    PROTOTYPE_QOpenCLGetDeviceIDs QOpenCLGetDeviceIDs;
    PROTOTYPE_QOpenCLGetPlatformInfo QOpenCLGetPlatformInfo;
    PROTOTYPE_QOpenCLGetDeviceInfo QOpenCLGetDeviceInfo;


protected:
    void initialize();
    void render(QPainter *painter);
};

class ImagePreviewWindow : public OpenGLWindow
{
    Q_OBJECT

signals:
    void selectionActiveChanged(bool value);
    
public:
    ImagePreviewWindow();
    ~ImagePreviewWindow();

    void setSharedWindow(SharedContextWindow * window);
    ImagePreviewWorker *getWorker();

    void initializeWorker();

public slots:
    void renderNow();
    void keyPressEvent(QKeyEvent *ev);
    void keyReleaseEvent(QKeyEvent *ev);

private:
    bool isInitialized;

    SharedContextWindow * shared_window;
    ImagePreviewWorker * gl_worker;
};

#endif // IMAGEPREVIEW_H
