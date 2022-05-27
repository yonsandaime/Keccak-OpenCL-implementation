// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <sstream> 
#include <chrono>
#include <thread>    
#include <string>

#define _CRT_SECURE_NO_WARNINGS

#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/cl2.hpp>
typedef char* string_t;
#define total 1600
#define rate 1088 
#define LOGG true

std::string readFile(const char* const szFilename)
{
    std::ifstream in(szFilename, std::ios::in | std::ios::binary);
    std::ostringstream contents;
    contents << in.rdbuf();
    return contents.str();  
}

std::ofstream myfile;

void hostr(const char* const filename, int times = 1)
{
    for (int tm = 0; tm < times; tm++) {
    using namespace std::chrono; // nanoseconds, system_clock, seconds
    //std::cout << "Hello World!\n";
    //get all platforms (drivers)
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if (all_platforms.size() == 0) {
        std::cout << " No platforms found. Check OpenCL installation!\n";
        exit(1);
    }
    //std::cout << "platforms size: " << all_platforms.size() << "\n";
    for(int i = 0; i< all_platforms.size(); i++)
    {
        cl::Platform default_platform = all_platforms[i];
        std::cout << "Available platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << "\n";
    }
    auto platform = all_platforms.at(0);
    std::cout << "Using platform: " << platform.getInfo<CL_PLATFORM_NAME>() << "\n";
    std::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

    auto device = devices.front();
    cl::Context context({ device });

    cl::Program::Sources sources;

    // kernel calculates for each element C=A+B
    std::string kernel_code  = readFile("sha3sequential.cl"); 
    //      "   void kernel simple_add(global const int* A, global const int* B, global int* C){       "
    //    "       C[get_global_id(0)]=A[get_global_id(0)]+B[get_global_id(0)];                 "
    //    "   }                                                                           ";
    std::string message2 = readFile(filename);
    //std::cout << (kernel_code);
    sources.push_back({ kernel_code.c_str(),kernel_code.length() });

    cl::Program program(context, sources);
    if (program.build({ device }) != CL_SUCCESS) {
        std::cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << "\n";
        exit(1);
    }

    cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE);
    unsigned char output[32] = { "" };

    std::cout << "Hashed file: " << filename <<"\n";

    int size = message2.length();


    cl::Buffer buffer_Output(context, CL_MEM_READ_WRITE, sizeof(char) * 32);
    cl::Buffer buffer_Size(context, CL_MEM_READ_WRITE, sizeof(int));
    cl::Buffer buffer_Input(context, CL_MEM_READ_WRITE, sizeof(char) * size);

    auto start = high_resolution_clock::now();

    queue.enqueueWriteBuffer(buffer_Input, CL_TRUE, 0, sizeof(char) * size, message2.c_str());
    queue.enqueueWriteBuffer(buffer_Size, CL_TRUE, 0, sizeof(int) , &size); 


    cl::Kernel kernelKeccak = cl::Kernel(program, "Keccak"); 
    kernelKeccak.setArg(0, buffer_Input);
    kernelKeccak.setArg(1, buffer_Size);
    kernelKeccak.setArg(2, buffer_Output);

    cl::Event prof_event;
        
    queue.enqueueNDRangeKernel(kernelKeccak, cl::NullRange, cl::NDRange(1), cl::NullRange, NULL, &prof_event);

    queue.finish();
    
    cl_ulong time_start, time_end;
    prof_event.wait();
    prof_event.getProfilingInfo(CL_PROFILING_COMMAND_QUEUED, &time_start);
    prof_event.getProfilingInfo(CL_PROFILING_COMMAND_END, &time_end);
    double runtime = (double)(time_end - time_start);

    queue.enqueueReadBuffer(buffer_Output, CL_TRUE, 0, sizeof(char) * 32, output);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;

    printf("Hash Value: ");
    for (int i = 0; i < 32; i++)
        printf("%02x", output[i]);

    double eventUnit = 1000000000;

    printf("\n");

    //WITHOUT data copy
    std::cout << "Runtime: " << runtime / eventUnit << "s\n";
    std::cout << "Hashrate: " << size* eventUnit / runtime << "\n\n";
    if (LOGG)myfile << filename << "; " << size * eventUnit / runtime << "\n";

    //WITH DATA COPY
    //std::cout << "RuntimeWithFileCopy: " << elapsed_seconds.count() << "\n";
    //std::cout << "Hashrate: " << message2.length() /elapsed_seconds.count() << "\n\n";
    //if (LOGG)myfile << filename << "; " << message2.length() / elapsed_seconds.count() << "\n";
    }
    }

int main()
{
    if (LOGG)myfile.open("loggerNormalNoMalloc.csv");
    if (LOGG)myfile << "normalNoMalloc" << "\n";
    hostr("other.bin");

    if (LOGG)myfile << "\n";
    if (LOGG)myfile.close();
}