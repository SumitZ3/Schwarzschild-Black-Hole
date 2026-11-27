# Schwarzschild Black Hole Ray Tracer

A real-time, shader-based black hole visualization built with **C++**,
**OpenGL 3.3**, **GLFW**, and **GLAD**.

The project renders a Schwarzschild black hole surrounded by a
relativistic thin accretion disk. Instead of drawing a black-hole-shaped
object, it numerically traces light rays through the Schwarzschild
spacetime and uses their paths to produce gravitational lensing, the
dark event-horizon region, and the distorted appearance of the disk and
background star field.

## Features

-   **Schwarzschild light-ray tracing** using the null-geodesic orbit
    equation \[ u'' + u = 3Mu\^2,`\qquad `{=tex}u=1/r \]
-   **Fourth-order Runge--Kutta (RK4)** integration of photon
    trajectories.
-   **Gravitational lensing** caused by the black hole's spacetime
    curvature.
-   **Event horizon and photon-sphere handling**, with the critical
    impact parameter used for a subtle photon-ring contribution.
-   **Relativistic accretion disk** with:
    -   A thin disk in the XZ plane.
    -   An inner edge at the innermost stable circular orbit (ISCO).
    -   A radial temperature and emissivity profile.
    -   Approximate blackbody colour.
    -   Turbulent detail generated with procedural noise and FBM.
    -   Relativistic Doppler shifting and gravitational redshift.
-   **Cinematic presentation** with a narrow field of view, vignette,
    exposure mapping, and gamma correction.
-   **Animated camera** with automatic orbital movement and
    mouse-controlled viewing inclination.

## Physics parameters

The shader uses geometrized units, setting (G=c=1), and normalizes the
black-hole mass to (M=1).

  ------------------------------------------------------------------------------
  Quantity                      :            Definition     :     Value when (M=1)
  ------------------- -------------------------------- -------------------------
  Event-horizon                   :            (r_s=2M)       :                  2
  radius                                               

  Photon-sphere                    :       (r\_{ph}=3M):                         3
  radius                                               

  ISCO radius                       :    (r\_{ISCO}=6M)  :                       6

  Critical impact    :   (b\_{crit}=3`\sqrt{3}`{=tex}M)     :  approximately 5.196
  parameter                                            

  Camera orbital          :              (r\_{cam}=80M)       :                 80
  radius                                               
  ------------------------------------------------------------------------------
## Requirements

-   C++ compiler with C++11 support or newer.
-   OpenGL 3.3 Core Profile support.
-   GLFW 3.
-   GLAD configured for OpenGL 3.3.
-   A graphics driver that supports the required OpenGL version.

The source includes the GLAD and GLFW headers:
``` cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
```

Make sure the GLAD source file and headers, GLFW library, and their
include/library paths are correctly configured in your build
environment.

## Build and run
In your IDE or compiler configuration:

1.  Create a C++ project and add the source file.
2.  Add the GLAD and GLFW include directories.
3.  Compile the GLAD implementation source file as part of the project,
    if your GLAD setup provides one.
4.  Link against GLFW and the platform's required OpenGL library.
5.  Build and run the executable.
If initialization fails, check that the GLFW library is linked correctly
and that your graphics driver supports OpenGL 3.3 Core Profile.

## Project structure

The current implementation is contained in a single C++ source file:

-   **Window and OpenGL setup:** GLFW initialization, OpenGL context
    creation, GLAD loading, fullscreen triangle, and render loop.
-   **Shader utilities:** Shader compilation and program linking.
-   **Vertex shader:** Generates fullscreen-triangle UV coordinates.
-   **Fragment shader:** Camera construction, ray tracing, RK4
    integration, disk intersection, star field, disk shading, and final
    colour processing.

## Performance notes

The fragment shader performs a numerical integration for each pixel,
with a loop that can run up to 4,500 iterations per ray. Rendering cost
can therefore rise quickly with resolution and with the number of rays
that approach the photon sphere.

## Credits

Created as an independent C++/OpenGL graphics and physics programming
project.
