#ifndef CONTEXTCL_H
#define CONTEXTCL_H

/*
 * This class initializes an OpenCL context.
 * */

#include <QLibrary>
#include <CL/opencl.h>

#include "devicecl.h"
#include "../../qxmath/qxmathlib.h"

class OpenCLContext
{
public:
    OpenCLContext();
    void initDevices();
    void initSharedContext();
    void initCommandQueue();
    void initResources();
    const cl_command_queue *getCommandQueue();
    cl_context * getContext();
    QList<DeviceCL> * getDeviceList();
    DeviceCL * getMainDevice();

    cl_program createProgram(QStringList paths, cl_int * err);
    void buildProgram(cl_program * program, const char * options);
    
    cl_kernel cl_rect_copy_float; 
    cl_kernel cl_parallel_reduction;
    
private:
    Matrix<cl_platform_id> platforms;
    Matrix<cl_device_id> devices;

    DeviceCL *main_device;
    QList<DeviceCL> device_list;
    
    cl_program program;
    
    cl_command_queue queue;
    cl_context context;
    cl_int err;

    typedef cl_int (*PROTOTYPE_QOpenCLGetPlatformIDs)(  	cl_uint num_entries,
                                                            cl_platform_id *platforms,
                                                            cl_uint *num_platforms);

    typedef cl_int (*PROTOTYPE_QOpenCLGetDeviceIDs)(        cl_platform_id platform,
                                                            cl_device_type device_type,
                                                            cl_uint num_entries,
                                                            cl_device_id *devices,
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

    PROTOTYPE_QOpenCLGetPlatformIDs QOpenCLGetPlatformIDs;
    PROTOTYPE_QOpenCLGetDeviceIDs QOpenCLGetDeviceIDs;
    PROTOTYPE_QOpenCLGetPlatformInfo QOpenCLGetPlatformInfo;
    PROTOTYPE_QOpenCLGetDeviceInfo QOpenCLGetDeviceInfo;
};

#endif // CONTEXTCL_H
