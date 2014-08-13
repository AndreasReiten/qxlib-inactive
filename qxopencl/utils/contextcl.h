#ifndef CONTEXTCL_H
#define CONTEXTCL_H

/*
 * This class initializes an OpenCL context.
 * */

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
};

#endif // CONTEXTCL_H
