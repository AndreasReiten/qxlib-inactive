#ifndef FILE_FORMATS_H
#define FILE_FORMATS_H

/*
 * The DetectorFile class.
 * */

/* GL and CL*/
// It might be cleaner to exclude opencl from this lib and rather use opencl as a method in a parent
#include <CL/opencl.h>

/* QT */
//class QString;

/* User libs */
#include "../../qxopencl/qxopencllib.h"
#include "../../qxmath/qxmathlib.h"




/* This is the file to edit/append if you would like to add your own file formats */

class DetectorFile
{
    /* Refer to PILATUS CBF header specs for details */
    public:
        ~DetectorFile();
        DetectorFile();
        DetectorFile(QString path, OpenCLContext * context);

        QString getPath() const;

        int set(QString path, OpenCLContext * context);
        int readData();
        int filterData(size_t * n, Matrix<float> *outBuf, float threshold_reduce_low, float threshold_reduce_high, float threshold_project_low, float threshold_project_high, bool isProjectionActive = true);

        Matrix<float> & data();
        int getWidth() const;
        int getHeight() const;
        size_t getBytes() const;
        float getSearchRadiusLowSuggestion();
        float getSearchRadiusHighSuggestion();
        float getQSuggestion();
        float getMaxCount();
        void clearData();
        float getFlux();
        float getExpTime();
        float getWavelength();
        float getDetectorDist();
        float getBeamX();
        float getBeamY();
        float getPixSizeX();
        float getPixSizeY();
        void setProjectionKernel(cl_kernel * kernel);
        void print();
        QString getHeaderText();
        void setActiveAngle(int value);
        void setOffsetOmega(double value);
        void setOffsetKappa(double value);
        void setOffsetPhi(double value);
        
        
    private:
        // Misc
        int active_angle;
        OpenCLContext * context_cl;
        cl_kernel * project_kernel;
        cl_int err;

        size_t loc_ws[2];
        size_t glb_ws[2];

        Matrix<float> data_buf;
        float background_flux;
        float backgroundExpTime;

        QString path;
        size_t fast_dimension, slow_dimension;
        float max_counts;
        int STATUS_OK;

        int threshold_reduce_low, threshold_reduce_high;
        int threshold_project_low, threshold_project_high;
        float srchrad_sugg_low, srchrad_sugg_high;

        void suggestSearchRadius();
        int readHeader();
        QString regExp(QString * regular_expression, QString * source, size_t offset, size_t i);

        // Angle offsets
        double offset_omega;
        double offset_kappa;
        double offset_phi;
        
        /* Non-optional keywords */
        QString detector;
        float pixel_size_x, pixel_size_y;
        float silicon_sensor_thickness;
        float exposure_time;
        float exposure_period;
        float tau;
        int count_cutoff;
        int threshold_setting;
        QString gain_setting;
        int n_excluded_pixels;
        QString excluded_pixels;
        QString flat_field;
        QString time_file;
        QString image_path;

        /* Optional keywords */
        float wavelength;
        int energy_range_low, energy_range_high;
        float detector_distance;
        float detector_voffset;
        float beam_x, beam_y;
        float flux;
        float filter_transmission;
        float start_angle;
        float angle_increment;
        float detector_2theta;
        float polarization;
        float alpha;
        float kappa;
        float phi;
        float phi_increment;
        float chi;
        float chi_increment;
        float omega;
        float omega_increment;
        QString oscillation_axis;
        int n_oscillations;
        float start_position;
        float position_increment;
        float shutter_time;

        float beta;
};


#endif
