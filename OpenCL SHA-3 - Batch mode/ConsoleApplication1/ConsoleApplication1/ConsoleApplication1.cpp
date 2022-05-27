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

#define totalBytes total/8 
#define message "The R points to resource, the id that specific part of the resources created. use of val because we don't want this object to be changed (constant value). Instead of using loops to see what everything is doing we make use of EventHandlers that will trigger when, what you defined, happens. This will then redirect them to a predefined function. This is written as bellow (e.g. button clicking)"

std::string readFile(const char* const szFilename)
{
    std::ifstream in(szFilename, std::ios::in | std::ios::binary);
    std::ostringstream contents;
    contents << in.rdbuf();
    return contents.str();
}

std::ofstream myfile;

void hostr(const char** const filenames, const int times)
{
    for (int j = 0; j < 1; j++) {
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
    for (int i = 0; i < all_platforms.size(); i++)
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

    std::string kernel_code = readFile("sha3sequential.cl");

    sources.push_back({ kernel_code.c_str(),kernel_code.length() });

    cl::Program program(context, sources);
    if (program.build({ device }) != CL_SUCCESS) {
        std::cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << "\n";
        exit(1);
    }

    std::string message2[4];
    int sze[4];
    unsigned char output[4][32] = { {""} };
    
    cl::CommandQueue queues[4];
    cl::Buffer buffer_Outputs[4];
    cl::Event prof_events[4];

    double runtime[4];
    auto start = std::chrono::steady_clock::now();

    for (int tm = 0; tm < times; tm++)
    {
        cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE);
        queues[tm] = queue;
        message2[tm] = readFile(filenames[tm]);
        sze[tm] = message2[tm].length();

        cl::Buffer buffer_Output(context, CL_MEM_READ_WRITE, sizeof(char) * 32);
        cl::Buffer buffer_Size(context, CL_MEM_READ_WRITE, sizeof(int));
        cl::Buffer buffer_Input(context, CL_MEM_READ_WRITE, sizeof(char) * sze[tm]);
        buffer_Outputs[tm] = buffer_Output;

        queues[tm].enqueueWriteBuffer(buffer_Input, CL_TRUE, 0, sizeof(char) * sze[tm], message2[tm].c_str());
        queues[tm].enqueueWriteBuffer(buffer_Size, CL_TRUE, 0, sizeof(int), &sze[tm]);

        cl::Kernel kernelKeccak = cl::Kernel(program, "Keccak");
        kernelKeccak.setArg(0, buffer_Input);
        kernelKeccak.setArg(1, buffer_Size);
        kernelKeccak.setArg(2, buffer_Outputs[tm]);

        cl::Event prof_event;
        prof_events[tm] = prof_event;
        queues[tm].enqueueNDRangeKernel(kernelKeccak, cl::NullRange, cl::NDRange(1), cl::NullRange, NULL, &prof_event);

    }

    for (int tm = 0; tm < times; tm++)
    {
        queues[tm].finish();
    }

    
    for (int tm = 0; tm < times; tm++)
    {
        queues[tm].enqueueReadBuffer(buffer_Outputs[tm], CL_TRUE, 0, sizeof(char) * 32, output[tm]);
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;

    printf("\n");

    for (int tm = 0; tm < times; tm++) {
        printf("Hashed file: %s \nHash value: ", filenames[tm]);

        for (int i = 0; i < 32; i++)
            printf("%02x", output[tm][i]);
        printf("\n");
        //std::cout << "Execution time: " << runtime[tm];
    }
    std::cout << "Hashrate " << message2[0].length() / elapsed_seconds.count() << "\n";
    if (LOGG)myfile << "10MB,1MB,100KB,10KB" << "; " << message2[0].length() / elapsed_seconds.count() << "\n";
    printf("\n\n");
    }
}

int main()
{
    if (LOGG)myfile.open("loggerBatch.csv");
    if (LOGG)myfile << "Batch" << "\n";

    hostr(new const char* [1]{ "other.bin" }, 1);
    hostr(new const char* [2]{ "other.bin", "other.bin" }, 2);
    hostr(new const char* [3]{ "other.bin", "other.bin", "other.bin" }, 3);
    hostr(new const char* [4]{ "other.bin", "other.bin", "other.bin", "other.bin" }, 4);

    if (LOGG)myfile << "\n";
    if (LOGG)myfile.close();
}