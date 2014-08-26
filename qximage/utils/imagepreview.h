#ifndef IMAGEPREVIEW_H
#define IMAGEPREVIEW_H

/*
 * This class can open a file from another thread and display its contents as a texture using QOpenGLPainter or raw GL calls
 * */

/* GL and CL*/
#include <CL/opencl.h>

#include "../../qxopengl/qxopengllib.h"
#include "../../qxfile/qxfilelib.h"
#include "../../qxmath/qxmathlib.h"


class Selection : public QObject, public QRect
{
    Q_OBJECT
public:
    explicit Selection(const QPoint & topLeft, const QPoint & bottomRight)
                : QRect(topLeft, bottomRight) {}
    explicit Selection(const QPoint & topLeft, const QSize & size)
                : QRect(topLeft, size) {}
    explicit Selection(int x, int y, int width, int height)
                : QRect(x, y, width, height) {}
    
    Selection();
    Selection(const Selection & other);
    ~Selection();
    
    
//    QRect area() const;
    double sum() const;
    double weighted_x() const;
    double weighted_y() const;
    
//    void setArea(QRect rect);
    void setSum(double value);
    void setWeightedX(double value);
    void setWeightedY(double value);
    
    Selection &operator =(QRect other);
    
private:
//    QRect p_area;
    double p_sum;
    double p_weighted_x;
    double p_weighted_y;
};


class ImagePreviewWorker : public OpenGLWorker
{
    Q_OBJECT
public:
    explicit ImagePreviewWorker(QObject *parent = 0);
    ~ImagePreviewWorker();
    void setSharedWindow(SharedContextWindow * window);

signals:
    void selectionChanged(QRect rect);
    void pathChanged(QString str);
    void resultFinished(QString str);
    void refresh();
    
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
    void setDataMin(double value);
    void setDataMax(double value);
    void displayImage(DetectorFile & file);

    void metaMouseMoveEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMousePressEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMouseReleaseEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
//    void keyPressEvent(QKeyEvent ev);
//    void keyReleaseEvent(QKeyEvent *ev);
    void wheelEvent(QWheelEvent* ev);
    void resizeEvent(QResizeEvent * ev);
    void setFrame(Image image);
    void setFrameNew(Image image);
    void setSelection(QRect rect);
    void setSelectionActive(bool value);
    void centerImage();
    void copyAndReduce(QRect selection_rect);
    void copyBufferRect(cl_mem cl_buffer, cl_mem cl_copy, Matrix<int> &buffer_size, Matrix<int> &buffer_origin, Matrix<int> &copy_size, Matrix<int> &copy_origin, size_t work_group_size);
    
    void integrateSingle(Image image);
    void integrateFolder(ImageFolder folder);
    void integrateSet(FolderSet set);
    
private:
    QString integrationFrameString(double value, Image &image);
    
    
    SharedContextWindow * shared_window;

    cl_int err;
    cl_program program;
    cl_kernel cl_image_preview;
    cl_kernel cl_image_display;
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

    DetectorFile frame;

    void initResourcesCL();
    void update(size_t w, size_t h);
    void setParameter(Matrix<float> &data);
    void setTsf(TransferFunction & tsf);
    Matrix<float> parameter;
    
    void beginRawGLCalls(QPainter * painter);
    void endRawGLCalls(QPainter * painter);

    // Draw
    void drawImage(QPainter *painter);
    void drawTexelOverlay(QPainter *painter);
    void drawSelection(QPainter *painter);
    void drawToolTip(QPainter * painter);

    // Vertice buffer objects
    GLuint texel_line_vbo[2];

    // Boolean checks
    bool isInitialized;
    bool isImageTexInitialized;
    bool isTsfTexInitialized;
    bool isCLInitialized;
    bool isFrameValid;

    Matrix<double> texture_view_matrix; // Used to draw main texture
    Matrix<double> translation_matrix;
    Matrix<double> zoom_matrix;
    
    Matrix<double> texel_view_matrix; // Used for texel overlay
    Matrix<double> texel_offset_matrix;

    // Mouse
    QPoint pos;
    QPoint prev_pos;
//    int last_mouse_pos_x;
//    int last_mouse_pos_y;

    // Display
    int isLog;
    int isCorrected;
    int mode;

    // Selection
    Selection selection;
    GLuint selection_lines_vbo;
    bool isSelectionActive;
    Matrix<int> getImagePixel(int x, int y);

    // Integration
    float sumGpuArray(cl_mem cl_data, unsigned int read_size, size_t work_group_size);

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
