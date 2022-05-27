// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <sstream> 
#include <chrono>
#include <thread>    
#include <string>
#include <iomanip>

#define _CRT_SECURE_NO_WARNINGS

#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/cl2.hpp>
typedef char* string_t;
#define total 1600
#define rate 1088 
#define BUFFER_SIZE 400
#define rateBytes rate/8
#define rateBytesBuffer rateBytes*BUFFER_SIZE
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

void hostr(const char* const filename)
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

        std::string kernel_code = readFile("sha3DualBuffer2s1t.cl");

        sources.push_back({ kernel_code.c_str(),kernel_code.length() });

        cl::Program program(context, sources);
        if (program.build({ device }) != CL_SUCCESS) {
            std::cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << "\n";
            exit(1);
        }

        std::cout << "Hashed file: " << filename << "\n";

        unsigned char output[32] = { "" };
        std::streamsize size;// = message2.length();
        std::streamsize sizeFichier;
        char* contents = new char[rateBytesBuffer];
        std::ifstream istr(filename, std::ios::in | std::ios::binary);
        std::streambuf* pbuf = NULL;

        if (istr) {
            pbuf = istr.rdbuf();
            size = pbuf->pubseekoff(0, istr.end);
            sizeFichier = size;
            //std::cout << " File size is: " << size << "\n";
            pbuf->pubseekoff(0, istr.beg);       // rewind 
            /*while (> 0)
            {
                std::cout << " readbytes: " << contents << "\n";
            }*/


            /*contents = new char[size];
            pbuf->sgetn(contents, size);
            istr.close();
            std::cout.write(contents, size);*/
        }

        cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE);
        cl::CommandQueue queue1(context, device, CL_QUEUE_PROFILING_ENABLE);

        cl::Buffer buffer_Output(context, CL_MEM_READ_WRITE, sizeof(char) * 32);
        cl::Buffer buffer_Size(context, CL_MEM_READ_WRITE, sizeof(int));
        cl::Buffer buffer_Input1(context, CL_MEM_READ_WRITE, rateBytesBuffer);
        cl::Buffer buffer_Input2(context, CL_MEM_READ_WRITE, rateBytesBuffer);

        cl::Buffer buffer_keccakCounter(context, CL_MEM_READ_WRITE, sizeof(int));

        int keccakCounter = 0;

        auto start = std::chrono::steady_clock::now();

        int itter = 0;
        double hashed = pbuf->sgetn(contents, rateBytesBuffer);
        queue.enqueueWriteBuffer(itter ? buffer_Input2 : buffer_Input1, CL_TRUE, 0, size > rateBytesBuffer ? rateBytesBuffer : size, contents);
        size -= rateBytesBuffer;
        double avg_time = 0;
        cl_ulong time_start, time_end;
        for (int offset = 0; size > 0; itter = !itter) {

            double percentage = hashed / (double)sizeFichier * 100.0;
            //if (sizeFichier > 10485759)
            //    std::cout << " Percentage done: " << percentage << "%\r";
            int sizewBytes = size + rateBytesBuffer;
            queue.enqueueWriteBuffer(buffer_Size, CL_TRUE, 0, sizeof(int), &sizewBytes);
            queue.enqueueWriteBuffer(buffer_keccakCounter, CL_TRUE, 0, sizeof(int), &keccakCounter);

            cl::Kernel kernelKeccak1 = cl::Kernel(program, "Keccak");
            kernelKeccak1.setArg(0, itter ? buffer_Input2 : buffer_Input1);
            kernelKeccak1.setArg(1, buffer_Size);
            kernelKeccak1.setArg(2, buffer_Output);
            kernelKeccak1.setArg(3, buffer_keccakCounter);

            cl::Event prof_event;

            cl::Event write_event;
            cl::vector<cl::Event> m_lastaccesses;
            m_lastaccesses.push_back(write_event);

            queue1.enqueueNDRangeKernel(kernelKeccak1, cl::NullRange, cl::NDRange(1), cl::NullRange, NULL, &prof_event);
            size -= rateBytesBuffer;

            prof_event.wait();
            prof_event.getProfilingInfo(CL_PROFILING_COMMAND_QUEUED, &time_start);
            prof_event.getProfilingInfo(CL_PROFILING_COMMAND_END, &time_end);
            double runtime = (double)(time_end - time_start);

            double time = runtime / 1000000000;
            avg_time += time;
            hashed += pbuf->sgetn(contents, rateBytesBuffer);
            queue.enqueueWriteBuffer(itter ? buffer_Input1 : buffer_Input2, CL_TRUE, 0, rateBytesBuffer, contents, NULL, &m_lastaccesses[0]);
            keccakCounter++;

            queue1.finish();
            cl::WaitForEvents(m_lastaccesses);
            //std::cout << " keccakCounter: " << keccakCounter << "\r";
        }

        int sizewBytes = size + rateBytesBuffer;
        queue.enqueueWriteBuffer(buffer_Size, CL_TRUE, 0, sizeof(int), &sizewBytes);
        queue.enqueueWriteBuffer(buffer_keccakCounter, CL_TRUE, 0, sizeof(int), &keccakCounter);

        cl::Kernel kernelKeccak = cl::Kernel(program, "Keccak");
        kernelKeccak.setArg(0, itter ? buffer_Input2 : buffer_Input1);
        kernelKeccak.setArg(1, buffer_Size);
        kernelKeccak.setArg(2, buffer_Output);
        kernelKeccak.setArg(3, buffer_keccakCounter);

        cl::Event prof_event2;

        queue.enqueueNDRangeKernel(kernelKeccak, cl::NullRange, cl::NDRange(1), cl::NullRange, NULL, &prof_event2);

        prof_event2.wait();
        prof_event2.getProfilingInfo(CL_PROFILING_COMMAND_QUEUED, &time_start);
        prof_event2.getProfilingInfo(CL_PROFILING_COMMAND_END, &time_end);
        double runtime = (double)(time_end - time_start);

        double time = runtime / 1000000000;
        avg_time += time;

        queue.finish();
        queue1.finish();

        queue1.enqueueReadBuffer(buffer_Output, CL_TRUE, 0, sizeof(char) * 32, output);

        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - start;

        printf("\nHash Value : ");
        for (int i = 0; i < 32; i++)
            printf("%02x", output[i]);

        std::cout << "\nExecution time: " << avg_time << "s";
        std::cout << "\n" << "Hashrate: " << (sizeFichier / avg_time) << "\n\n\n";
        if (LOGG)myfile << filename << "; " << (sizeFichier / avg_time) << "\n";
        istr.close();
    }
}

int main()
{
    if (LOGG)myfile.open("loggerDualbuffer1Thread.csv");
    if (LOGG)myfile << "Dualbuffer1Thread" << "\n";

    hostr("other.bin");


    if (LOGG)myfile << "\n";
    if (LOGG)myfile.close();
}