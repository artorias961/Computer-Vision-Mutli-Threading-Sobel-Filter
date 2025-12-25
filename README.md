# Computer Vision Multi Threading Sobel Filter

This project is to validate end-to-end operation of a image and a video, built and run with CMake and OpenCV, including device recognition, frame capture, and the ability to apply basic computer-vision processing (grayscale conversion and 2D [Sobel](https://en.wikipedia.org/wiki/Sobel_operator) edge detection) to captured frames. 





## Purpose of each Versions
Each version will contain the CMake file with the src main cpp file.

*NOTE: FOR MULTITHREADING ONLY or THREADING IN GENERAL, I am using Windows 11, so I will use [Windows threading](https://learn.microsoft.com/en-us/windows/win32/procthread/processes-and-threads) library but linux threading library is the same. Just using windows since, most students in my school uses Windows:/. You can use [P-thread](https://man7.org/linux/man-pages/man7/pthreads.7.html) just follow the guide!!! Good luck;)*


- **Version 1**: To test if C++, CMake, and OpenCV works together
- **Version 2**: To do a simple OpenCV pipeline image conversion (bgr -> grayscale -> threshold)
- **Version 3**: To do a 2D [Sobel Filter](https://en.wikipedia.org/wiki/Sobel_operator) on a image
- **Version 4**: To do a 2D [Sobel Filter](https://en.wikipedia.org/wiki/Sobel_operator) on a video/gif
- **Version 5**: To implement Multithreading Sobel Filter on a image 
- **Version 6**: To implement Multithreading Sobel Filter on a video 



## Treelist of the directory

Computer Vision Multi Threading Sobel Filter:
|
|
|   CMakeList.txt
|   README.md
|   treelist.txt
|   
+---archive
|   +---Version_1
|   \---Version_2
+---pictures
|       test.jpg
|       
+---picture_reference_readme
|       image-1.png
|       image-10.png
|       image-11.png
|       image-12.png
|       image-13.png
|       image-14.png
|       image-15.png
|       image-16.png
|       image-17.png
|       image-18.png
|       image-19.png
|       image-2.png
|       image-20.png
|       image-21.png
|       image-22.png
|       image-23.png
|       image-24.png
|       image-25.png
|       image-3.png
|       image-4.png
|       image-5.png
|       image-6.png
|       image-7.png
|       image-8.png
|       image-9.png
|       image.png
|       
\---src
        main.cpp
        



# Setting up the environment for OpenCV, GCC Compiler (C++), CMakeList, and Edit environment variables for your account

## Visual Studio Code

I will be using [Visual Studio Code](https://code.visualstudio.com). You are welcome to use any other Code Editor!

### Extensions to install in Visual Studio Code (Recommend)
I recommend going to the extension tab within Visual Studio Code and install the following:

![alt text](picture_reference_readme/image.png)
![alt text](picture_reference_readme/image-1.png)




## Installing and Setting GCC Compiler
To install windows 10/11 you need to go to [MSYS2](https://www.msys2.org). Just follow the isntallation processes if not. You can follow mine but I will skip some steps.

![alt text](picture_reference_readme/image-2.png)

Navigate to:
![alt text](picture_reference_readme/image-3.png)

Download the file and run it. A windows protection will pop up. Once you are running it, i recommend leaving the default path as since we will reference the directory for CMake:

![alt text](picture_reference_readme/image-4.png)

Once installation is done, run MSYS2. 

![alt text](picture_reference_readme/image-5.png)

Lets jsut update the entire compiler, just to give you less headache later on:
```bash
pacman -Syu
```

Then type in:
```bash
pacman -S mingw-w64-x86_64-gcc
```

To verify we have the GCC Compiler, do the following command:
```bash
gcc --version
```
![alt text](picture_reference_readme/image-6.png)




Now that we have GCC Compiler installed. We need to **Edit environment variables for your account** since we need to call out the directory for windows to find the compiler and use it in Visual Studio Code. So, in windows, open Start, type in **Edit environment variables for your account** and open:

![alt text](picture_reference_readme/image-7.png)

Yours may look a bit different but should be the same essentially.
![alt text](picture_reference_readme/image-8.png)

Double click variable **Path**

![alt text](picture_reference_readme/image-9.png)

Select a empty line (assuming you used the default path for MSYS2):
![alt text](picture_reference_readme/image-10.png)
![alt text](picture_reference_readme/image-11.png)

Click Ok, twice. The second image shows you what the pacman command does. From here, we can use GCC Compiler!



## Installing and Setting CMake
Go to [CMake](https://cmake.org/download/), select for you system, if you are windows user i recommend selecting **Windows x64 Installer**.
![alt text](picture_reference_readme/image-12.png)

Double click the *.msi* file and again use the default path. It will make your life easier:
![alt text](picture_reference_readme/image-13.png)

Now we need to open **Edit environment variables for your account**, go to **Path**, double click, go to an empty cell and type in the following (if you are using the default path):

![alt text](picture_reference_readme/image-14.png)
![alt text](picture_reference_readme/image-15.png)

Click Ok, twice. Now windows can use CMake commands in Visual Studio Code!

## Installing and Setting up OpenCV

Go to [OpenCV](https://opencv.org/releases/), select Windows and run the file. 
![alt text](picture_reference_readme/image-16.png)

This will be the only time I recommend changing the directory from (if you are planning to use OpenCV a lot else put it in a local folder):
![alt text](picture_reference_readme/image-17.png)

To:
![alt text](picture_reference_readme/image-18.png)
This is just easier and so far what worked with most systems I had to installed from scratch. Once extraction is done we need to do a couple of things in order OpenCV can be recgonize by windows and CMake. So first, lets open **Edit environment variables for your account**

![alt text](picture_reference_readme/image-19.png)

Click **New**, for *Variable Name* put *OpenCV_DIR* and for *Variable Value* put *C:\opencv\build*. Yes, spelling does matter!

![alt text](picture_reference_readme/image-20.png)

Select Ok, once. waht the previous image doing is making CMake know how to build the OpenCV library!

![alt text](picture_reference_readme/image-21.png)
![alt text](picture_reference_readme/image-22.png)


In **Edit environment variables for your account**, double click **Path**, add to an empty cell *C:\opencv\build\x64\vc16\bin*, add to another empty cell *C:\opencv\build\x64\vc16\lib*:

![alt text](picture_reference_readme/image-23.png)
![alt text](picture_reference_readme/image-24.png)
![alt text](picture_reference_readme/image-25.png)

Click Ok, twice. Now Visual Studio Code can find the OpenCV libraries!!!



# The basics environment build requirements
Just know, if you encounter any issues when building. Verify the paths for the **Edit environment variables for your account**. Recheck the steps since CMake and OpenCv is a headache to setup. But down below should be the bare minimum to do basic OpenCV. As long, you can compile then you are fine. 

If you get no errors from CMake or OpenCV is not recgonize. Then congrats! This took me weeks on the job to solve lol. 


## CMakeList.txt file


```cmake
cmake_minimum_required(VERSION 3.5)

# Project name
project(OpenCVExample)

# Find OpenCV
find_package(OpenCV REQUIRED)

# Include directories from OpenCV
include_directories(${OpenCV_INCLUDE_DIRS})

# Add your source file(s)
add_executable(OpenCVExample src/main.cpp)

# Link OpenCV libraries
target_link_libraries(OpenCVExample ${OpenCV_LIBS})
```

## Main file
```c++
#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // Load an image (replace with your own file if needed)
    cv::Mat image = cv::imread("test.jpg");

    if (image.empty()) {
        std::cout << "Could not read the image!" << std::endl;
        return -1;
    }

    cv::imshow("Test Window", image);

    cv::waitKey(0);  // Wait for any key
    return 0;
}
```

# How to build and run an enviroment using CMake

## Building an Environment
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
.\Release\OpenCVExample.exe
```

## Recommended to rebuild an enviornment for any changes in the code/cmake

```bash
cd ..
rmdir build 
```

then

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
.\Release\OpenCVExample.exe
```
