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

class ImagePreviewWorker : public OpenGLWorker
{
    Q_OBJECT
public:
    explicit ImagePreviewWorker(QObject *parent = 0);
    ~ImagePreviewWorker();
    void setSharedWindow(SharedContextWindow * window);

signals:
    void selectionChanged(QRectF rect);
    void outputTextChanged(QString str);

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

    void metaMouseMoveEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMousePressEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void metaMouseReleaseEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
    void wheelEvent(QWheelEvent* ev);
    void resizeEvent(QResizeEvent * ev);
    void setImageFromPath(QString path);
    void setSelection(QRectF rect);
    void setSelectionActive(bool value);
    void centerImage();
    double integrate(QRectF rect, DetectorFile file);
    void integrateSingle();
    void integrateSelectedMode();
    void setIntegrationMode(int value);
    
private:
    SharedContextWindow * shared_window;

    cl_int err;
    cl_program program;
    cl_kernel cl_image_preview;
    cl_mem image_tex_cl;
    cl_mem source_cl;
    cl_mem tsf_tex_cl;
    cl_mem parameter_cl;
    cl_mem target_cl;
    
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

    // Vertice buffer objects
    GLuint texel_line_vbo[2];

    // Boolean checks
    bool isInitialized;
    bool isImageTexInitialized;
    bool isTsfTexInitialized;
    bool isCLInitialized;
    bool isFrameValid;
    bool isRendering;

    Matrix<double> texture_view_matrix;
    Matrix<double> texel_view_matrix;
    Matrix<double> translation_matrix;
    Matrix<double> zoom_matrix;
//    Matrix<double> cursor_translation_matrix;
    Matrix<double> texel_offset_matrix;

    // Mouse
    int last_mouse_pos_x;
    int last_mouse_pos_y;

    // Display
    int isLog;
    int isCorrected;
    int mode;

    // Selection
    QRectF selection;
    GLuint selection_lines_vbo;
    bool isSelectionActive;
    Matrix<int> getImagePixel(int x, int y);

    // Integration
    int integration_mode;

protected:
    void initialize();
    void render(QPainter *painter);
};

class ImagePreviewWindow : public OpenGLWindow
{
    Q_OBJECT

public:
    ImagePreviewWindow();
    ~ImagePreviewWindow();

    void setSharedWindow(SharedContextWindow * window);
    ImagePreviewWorker *getWorker();

    void initializeWorker();

public slots:
    void renderNow();

private:
    bool isInitialized;

    SharedContextWindow * shared_window;
    ImagePreviewWorker * gl_worker;
};

#endif // IMAGEPREVIEW_H
