# CPU Based Ray Tracing

## Demo Reel Video

Work-in-progress... the video will be made public before the end of the CSC8599 module (22nd Agugust 2023).

## Getting Started

### Things to note

When developing this project, the code is running on a Windows laptop with <ins>11980HK</ins> and <ins>RTX3080 (laptop version)</ins>.

The code is written in Visual Studio 2022 with C++20. Note that if you want to run from the source code, you do need a C++20 compiler since we are indeed using C++20 features.

Please <ins>run the code in Release mode</ins>. The ray tracing is currently done entirely on the CPU side, it will be extremely slow if we are in Debug mode.

### For everything except the offline prototype

To run from the source code, we need:

- [Visual Studio 2022](https://visualstudio.com)
- A recent version of [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows)
  
Then, execute `scripts/Setup.bat`, this will generate the Visual Studio 2022 solution for us.

### For the offline prototype

The offline prototype of the non-physical path tracer is design to produce the data for one single image each time we run the executable file in the Command Prompt.

When running the executable file in the Command Prompt, please redirect the outputs to a `.ppm` file. For example:

```
8599RayTracer.exe > output_image.ppm
```

A `.ppm` file can be rendered into an image using:

- **Portable Anymap Viewer** (on Windows)
- **GIMP**

## Main Features

- Whitted Style Ray Tracing

  - Blinn-Phong Model on diffuse material
  - Glass with light transport calculated using Fresnel equations, mirror reflection and Snell's law

- Moller-Trumbore algorithm for testing ray-triangle intersection

- BVH as acceleration structure
 
- Monte Carlo Path Tracing

  - Energy-conserved Russian Roulette for terminating the rays
  - Light sources sampling for faster convergence

- Spatial Denoising using Joint Bilateral Filtering

- Temporal Denoising using motion vectors, object ID and clamping techniques

- "Hacked BRDF" for:

  - Mirror
  - Glass with air bubbles (reflectance calculated using Schlick's approximation)
  - Rough Metal
  - Diffuse Material

- MSAA

- Gamma Correction

- A thin lens camera for Defocus Blur / depth-of-field (currently only implemented in the offline prototype)

- Procedually-Generated Textures including:

  - the chessboard
  - the white-to-blue skybox

- To be continued...

## Gallery

Whitted-Style Ray Tracing:

<img src="https://github.com/IQ404/cpu-based-ray-tracer/blob/main/Sample%20Images/whitted%20style%20ray%20tracer/WhittedStyle.gif"></a>

<img src="https://github.com/IQ404/cpu-based-ray-tracer/blob/main/Sample%20Images/whitted%20style%20ray%20tracer/WhittedStyle.jpg"></a>

Ray-traced Meshes with BVH:

<img src="https://github.com/IQ404/cpu-based-ray-tracer/blob/main/Sample%20Images/bvh%20ray%20tracer/bvh%20ray%20tracer.jpg"></a>

The "Hacked" Path Tracing:

<img src="https://github.com/IQ404/cpu-based-ray-tracer/blob/main/Sample%20Images/non%20physical%20path%20tracer/non-physical%20path%20tracing.png"></a>

Monte Carlo Path Tracing (10000 SPP):

<img src="https://github.com/IQ404/cpu-based-ray-tracer/blob/main/Sample%20Images/monte%20carlo%20path%20tracer/RR0.8%2010000spp.jpg"></a>

1 SPP without denoising:

<img src="https://github.com/IQ404/cpu-based-ray-tracer/blob/main/Sample%20Images/denoiser/without%20denoising.jpg"></a>

1 SPP with denoising:

<img src="https://github.com/IQ404/cpu-based-ray-tracer/blob/main/Sample%20Images/denoiser/with%20denoising.jpg"></a>

To be continued...

## Potential Future Works

### Move to GPU

In the near future, this project is aimed to be implemented using:

- Taichi (a compiler that translates Python into highly optimized GPU code)
- Vulkan
- NVIDIA Optix

and to give a performance comparison.

### More Materials For the Path Tracer

- Perfect specular reflection
- Microfacet models

### Better division rule for BVH

- SAH (Surface Area Heuristic)

### More Denoising algorithms

<ins>Note</ins>: the current implementation for the spatial filter is too brutal to have an acceptable render time!!!

- With Machine Learning:

  - [Temporally Stable Real-Time Joint Neural Denoising and Supersampling](https://www.intel.com/content/www/us/en/developer/articles/technical/temporally-stable-denoising-and-supersampling.html)
 
  - [RAE](https://research.nvidia.com/publication/2017-07_interactive-reconstruction-monte-carlo-image-sequences-using-recurrent)
 
  - [SVGF](https://research.nvidia.com/publication/2017-07_spatiotemporal-variance-guided-filtering-real-time-reconstruction-path-traced)

- Without Machine Learning:

  - [Weighted À-Trous Linear Regression (WALR) for Real-Time Diffuse Indirect Lighting Denoising](https://gpuopen.com/download/publications/GPUOpen2022_WALR.pdf)
 
  - Temporally Reliable Motion Vectors for Better Use of Temporal Information ([Ray Tracing Gems II: Chapter 25](https://www.realtimerendering.com/raytracinggems/rtg2/index.html))
 

## References
