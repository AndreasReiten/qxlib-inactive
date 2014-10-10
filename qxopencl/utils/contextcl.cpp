#include "contextcl.h"

#include <QtGlobal>
#include <QList>
//#include <QOpenGLContext>
#include <QString>
#include <QDebug>
#include <QFile>
#include <QByteArray>

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>

#include <CL/cl_gl.h>

#ifdef Q_OS_WIN
    #include <windows.h>
#elif defined Q_OS_LINUX
    #include <GL/glx.h>
#endif

OpenCLContext::OpenCLContext()
{
    QLibrary myLib("OpenCL");

    QOpenCLGetPlatformIDs = (PROTOTYPE_QOpenCLGetPlatformIDs) myLib.resolve("clGetPlatformIDs");
    if (!QOpenCLGetPlatformIDs) qDebug("Failed to resolve clGetPlatformIDs");

    QOpenCLGetDeviceIDs = (PROTOTYPE_QOpenCLGetDeviceIDs) myLib.resolve("clGetDeviceIDs");
    if (!QOpenCLGetDeviceIDs) qDebug("Failed to resolve clGetDeviceIDs");

    QOpenCLGetPlatformInfo = (PROTOTYPE_QOpenCLGetPlatformInfo) myLib.resolve("clGetPlatformInfo");
    if (!QOpenCLGetPlatformInfo) qDebug("Failed to resolve clGetPlatformInfo");

    QOpenCLGetDeviceInfo = (PROTOTYPE_QOpenCLGetDeviceInfo) myLib.resolve("clGetDeviceInfo");
    if (!QOpenCLGetDeviceInfo) qDebug("Failed to resolve clGetDeviceInfo");
}

const cl_command_queue * OpenCLContext::getCommandQueue()
{
    return &queue;
}

cl_context * OpenCLContext::getContext()
{
    return &context;
}

QList<DeviceCL> * OpenCLContext::getDeviceList()
{
    return &device_list;
}

DeviceCL * OpenCLContext::getMainDevice()
{
    return main_device;
}

cl_program OpenCLContext::createProgram(QStringList paths, cl_int * err)
{
    // Program
    Matrix<size_t> lengths(1, paths.size());
    Matrix<const char *> sources(1, paths.size());
//    Matrix<QByteArray> qsources(1, paths.size());
    
    Matrix<QByteArray> blobs(1,paths.size());
    
    for (size_t i = 0; i < paths.size(); i++)
    {
//        std::ifstream in(paths.at(i), std::ios::in | std::ios::binary);
//        std::string contents;
    
//        if (in)
//        {
//            in.seekg(0, std::ios::end);
//            contents.resize(in.tellg());
//            in.seekg(0, std::ios::beg);
//            in.read(&contents[0], contents.size());
//            in.close();
//        }
//        else
//        {
//            qDebug(QString("Could not open file: " + QString(paths.at(i))).toStdString().c_str());
//        }
    
//        qsources[i] = QString(contents.c_str()).toUtf8();
        
        QFile file(paths[i]);
        if (!file.open(QIODevice::ReadOnly)) qDebug(QString(QString("Could not open file: ")+paths[i]).toStdString().c_str());
        blobs[i] = file.readAll();
        
//        qDebug() << blobs[i];
        
        sources[i] = blobs[i].data();
        lengths[i] = blobs[i].length();
    }
    return clCreateProgramWithSource(context, paths.size(), sources.data(), lengths.data(), err);
}

void OpenCLContext::buildProgram(cl_program * program, const char * options)
{
    // Compile kernel
    cl_device_id tmp = main_device->getDeviceId();
    err = clBuildProgram(*program, 1, &tmp, options, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        // Compile log
        qDebug() << "Error compiling kernel: "+QString(cl_error_cstring(err));
        std::stringstream ss;

        char* build_log;
        size_t log_size;

        clGetProgramBuildInfo(*program, main_device->getDeviceId(), CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        build_log = new char[log_size+1];

        clGetProgramBuildInfo(*program, main_device->getDeviceId(), CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
        build_log[log_size] = '\0';

        ss << "___ START KERNEL COMPILE LOG ___" << std::endl;
        ss << build_log << std::endl;
        ss << "___  END KERNEL COMPILE LOG  ___" << std::endl;
        delete[] build_log;

        qDebug(ss.str().c_str());
    }
}


void OpenCLContext::initDevices()
{
    // Platforms and Devices
    cl_uint max_platforms = 10; // Max number of platforms
    cl_uint num_platforms = 0;
    platforms.reserve (1, max_platforms);

    // Get platforms
    err = QOpenCLGetPlatformIDs(max_platforms, platforms.data(), &num_platforms);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_uint max_devices = 10; // Max number of devices per platform
    cl_uint nupaint_device_gls = 0;
    cl_uint nupaint_device_gls_total = 0;
    devices.reserve (1, max_devices);

    // Get devices for platforms
    for (size_t i = 0; i < num_platforms; i++)
    {
        err = QOpenCLGetDeviceIDs( platforms[i], CL_DEVICE_TYPE_GPU, max_devices, devices.data(), &nupaint_device_gls);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        for (size_t j = 0; j < nupaint_device_gls; j++)
        {
            device_list.append (DeviceCL(platforms[i], devices[j]));
            qDebug() << device_list.last().getDeviceInfoString().c_str();
            nupaint_device_gls_total++;
        }
    }

    // Find suitable devices
    for (int i = 0; i < device_list.size(); i++)
    {
        if (device_list[i].getDeviceType() != CL_DEVICE_TYPE_GPU)
        {
            device_list.removeAt(i);
            i--;
        }
    }

    // Re-populate devices with only suitable devices
    devices.reserve(1, device_list.size());
    for (int i = 0; i < device_list.size(); i++)
    {
        devices[i] = device_list[i].getDeviceId();
    }

    // Pick a preferred device
    main_device = &device_list[0];

    // Get platforms
    cl_uint num_platform_entries = 64;
    cl_platform_id platform[64];
    cl_uint num_platforms;

    err = QOpenCLGetPlatformIDs(num_platform_entries, platform, &num_platforms);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Get devices for each platform
    for (size_t i = 0; i < num_platforms; i++)
    {
        char platform_name[128];

        err = QOpenCLGetPlatformInfo(platform[i], CL_PLATFORM_NAME, sizeof(char)*128, platform_name, NULL);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        qDebug(platform_name);


        cl_uint num_device_entries = 64;
        cl_device_id device[64];
        cl_uint num_devices;

        err = QOpenCLGetDeviceIDs( platform[i], CL_DEVICE_TYPE_GPU, num_device_entries, device, &num_devices);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        for (size_t j = 0; j < num_devices; j++)
        {
            device_list.append (DeviceCL(platform[i], device[j]));
        }
    }

    // For now just select the first GPU in the list
    main_device = &device_list[0];
}

void OpenCLContext::initSharedContext()
{
    // Context with GL interopability

    #ifdef Q_OS_LINUX
    cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties) glXGetCurrentContext(),
        CL_GLX_DISPLAY_KHR, (cl_context_properties) glXGetCurrentDisplay(),
        CL_CONTEXT_PLATFORM, (cl_context_properties) main_device->getPlatformId(),
        0};

    #elif defined Q_OS_WIN
    cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
        CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(),
        CL_CONTEXT_PLATFORM, (cl_context_properties) main_device->getPlatformId(),
        0};
    #endif

    context = clCreateContext(properties, devices.size(), devices.data(), NULL, NULL, &err);
    if (err != CL_SUCCESS)
    {
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
}

void OpenCLContext::initCommandQueue()
{
    // Command queue
    queue = clCreateCommandQueue(context, main_device->getDeviceId(), 0, &err);
    if (err != CL_SUCCESS)
    {
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
}

void OpenCLContext::initResources()
{
    // Build program from OpenCL kernel source
    QStringList paths;
    paths << "cl/mem_functions.cl";
    paths << "cl/parallel_reduce.cl";

    program = createProgram(paths, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    buildProgram(&program, "-Werror");

    // Kernel handles
    cl_rect_copy_float = clCreateKernel(program, "rectCopyFloat", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    
    cl_parallel_reduction = clCreateKernel(program, "psum", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}
