#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cmath>
using namespace std;

#define PI 3.14159265358979323846f


constexpr int WIDTH = 1600;
constexpr int HEIGHT = 900;

double mouseX = WIDTH * 0.5;
double mouseY = HEIGHT * 0.5;

bool firstMouse = true;

void framebuffer_callback(
    GLFWwindow*,
    int width,
    int height)
{
    glViewport(
        0,
        0,
        width,
        height
    );
}

void mouse_callback(
    GLFWwindow*,
    double x,
    double y)
{
    mouseX = x;
    mouseY = y;
}

const char* vertexShader = R"GLSL(

#version 330 core

layout(location = 0)
in vec2 aPosition;

out vec2 uv;

void main()
{
    uv =
        aPosition *
        0.5 +
        0.5;

    gl_Position =
        vec4(
            aPosition,
            0.0,
            1.0
        );
}

)GLSL";

const char* fragmentShader = R"GLSL(

#version 330 core

out vec4 FragColor;

in vec2 uv;

uniform vec2  uResolution;
uniform vec2  uMouse;
uniform float uTime;

// CONSTANTS

const float M =     1.0;
const float RS =
    2.0 * M;

const float RPH =
    3.0 * M;

const float RISCO =
    6.0 * M;

const float BCRIT =
    3.0 * sqrt(3.0) * M;

const float CAMERA_R =
    80.0;

const float PI =
    3.14159265359;

const float TWO_PI =
    6.28318530718;


float hash21(vec2 p)
{
    p =
        fract(
            p *
            vec2(
                123.34,
                345.45
            )
        );

    p +=
        dot(
            p,
            p + 34.345
        );

    return fract(
        p.x *
        p.y
    );
}

// NOISE

float noise(vec2 p)
{
    vec2 i =
        floor(p);

    vec2 f =
        fract(p);

    f =
        f *
        f *
        (
            3.0 -
            2.0 * f
        );

    float a =
        hash21(i);

    float b =
        hash21(
            i +
            vec2(
                1.0,
                0.0
            )
        );

    float c =
        hash21(
            i +
            vec2(
                0.0,
                1.0
            )
        );

    float d =
        hash21(
            i +
            vec2(
                1.0,
                1.0
            )
        );

    return mix(
        mix(a, b, f.x),
        mix(c, d, f.x),
        f.y
    );
}

// FBM
float fbm(vec2 p)
{
    float result = 0.0;
    float amp = 0.5;

    for(int i = 0; i < 5; ++i)
    {
        result +=
            amp *
            noise(p);

        p *= 2.0;

        amp *= 0.5;
    }

    return result;
}


// BLACKBODY APPROXIMATION

vec3 blackbody(
    float temperature)
{
    temperature =
        clamp(
            temperature,
            0.0,
            1.0
        );

    vec3 red =
        vec3(
            0.35,
            0.008,
            0.001
        );

    vec3 orange =
        vec3(
            1.0,
            0.12,
            0.008
        );

    vec3 gold =
        vec3(
            1.0,
            0.48,
            0.025
        );

    vec3 white =
        vec3(
            1.0,
            0.93,
            0.72
        );

    vec3 c =
        mix(
            red,
            orange,
            smoothstep(
                0.0,
                0.35,
                temperature
            )
        );

    c =
        mix(
            c,
            gold,
            smoothstep(
                0.25,
                0.7,
                temperature
            )
        );

    c =
        mix(
            c,
            white,
            smoothstep(
                0.65,
                1.0,
                temperature
            )
        );

    return c;
}
// DISK TEMPERATURE
//
// T(r) ∝
// [ r^-3 (1 - sqrt(r_ISCO/r)) ]^(1/4)

float diskTemperature(
    float r)
{
    if(r <= RISCO)
        return 0.0;

    float x =
        RISCO / r;

    float profile =
        pow(
            x,
            3.0
        )
        *
        (
            1.0 -
            sqrt(x)
        );

    return clamp(
        pow(
            max(
                profile,
                0.0
            ),
            0.25
        ) * 1.55,
        0.0,
        1.0
    );
}


// DISK EMISSIVITY
float diskEmission(
    float r)
{
    if(
        r < RISCO ||
        r > 35.0
    )
        return 0.0;

    float x =
        RISCO / r;

    float profile =
        pow(
            x,
            3.0
        )
        *
        (
            1.0 -
            sqrt(x)
        );

    return
        clamp(
            profile * 32.0,
            0.0,
            1.0
        );
}


// SCHWARZSCHILD NULL GEODESIC
//
// u = 1/r
//
// u' = v
//
// v' = 3Mu²-u


vec2 geodesicDerivative(
    vec2 state)
{
    float u =
        state.x;

    float v =
        state.y;

    return vec2(
        v,
        3.0 * M * u * u - u
    );
}


// RK4

vec2 rk4(
    vec2 state,
    float h)
{
    vec2 k1 =
        geodesicDerivative(
            state
        );

    vec2 k2 =
        geodesicDerivative(
            state +
            0.5 * h * k1
        );

    vec2 k3 =
        geodesicDerivative(
            state +
            0.5 * h * k2
        );

    vec2 k4 =
        geodesicDerivative(
            state +
            h * k3
        );

    return
        state +
        (
            k1 +
            2.0 * k2 +
            2.0 * k3 +
            k4
        )
        *
        h /
        6.0;
}
// RAY RESULT

struct RayResult
{
    bool captured;

    bool diskHit;

    float diskRadius;

    vec3 diskPosition;

    vec3 diskPhotonDirection;

    vec3 escapeDirection;

    float phi;

    float impact;
};

// CAMERA

struct Camera
{
    vec3 position;

    vec3 forward;

    vec3 right;

    vec3 up;
};

// CAMERA CONSTRUCTION

Camera makeCamera()
{
    Camera cam;


    // Automatic slow orbital movement.
   

    float mouseYaw =
        (
            uMouse.x /
            uResolution.x -
            0.5
        )
        *
        0.7;

    float mousePitch =
        (
            uMouse.y /
            uResolution.y -
            0.5
        )
        *
        0.45;

    float yaw =
        uTime * 0.055 +
        mouseYaw;

    // About 22 degrees above the disk.
    float theta =
        radians(68.0)
        +
        mousePitch;

    theta =
        clamp(
            theta,
            radians(35.0),
            radians(82.0)
        );

    // Disk lies in XZ plane.
    //
    // Y is its normal.
    //

    vec3 radial =
        vec3(
            sin(theta) * cos(yaw),
            cos(theta),
            sin(theta) * sin(yaw)
        );

    cam.position =
        radial *
        CAMERA_R;

    cam.forward =
        normalize(
            -cam.position
        );

    vec3 worldUp =
        vec3(
            0.0,
            1.0,
            0.0
        );

    cam.right =
        normalize(
            cross(
                cam.forward,
                worldUp
            )
        );

    cam.up =
        normalize(
            cross(
                cam.right,
                cam.forward
            )
        );

    return cam;
}

// RAY TRACE

RayResult traceRay(
    vec3 cameraPosition,
    vec3 rayDirection)
{
    RayResult result;

    result.captured = false;
    result.diskHit = false;

    result.diskRadius = 0.0;

    result.diskPosition =
        vec3(0.0);

    result.diskPhotonDirection =
        vec3(0.0);

    result.escapeDirection =
        vec3(0.0);

    result.phi = 0.0;
    result.impact = 0.0;

    // The photon trajectory lies in the plane containing
    // the black hole, camera position and ray direction.

    vec3 eR =
        normalize(
            cameraPosition
        );

    vec3 tangent =
        rayDirection -
        dot(
            rayDirection,
            eR
        ) *
        eR;

    float tangentLength =
        length(tangent);

    if(tangentLength < 0.00001)
    {
        result.captured = true;
        return result;
    }

    vec3 ePhi =
        tangent /
        tangentLength;
    // Impact parameter
    // b = r sin(alpha) / sqrt(1 - 2M/r) 

    float fObserver =
        1.0 -
        RS /
        CAMERA_R;

    float b =
        CAMERA_R *
        tangentLength /
        sqrt(fObserver);

    result.impact =
        b;

  
    // u = 1/r
    float u =
        1.0 /
        CAMERA_R;

    // Null orbit equation
    //
    // (du/dphi)^2 =
    //
    // 1/b²-u²+2Mu³
    
    float radialTerm =
        1.0 /
        max(
            b * b,
            0.000001
        )
        -
        u * u
        +
        2.0 *
        M *
        u *
        u *
        u;

    radialTerm =
        max(
            radialTerm,
            0.00000001
        );

    // Ray travels toward smaller r.
    // Therefore du/dphi > 0.
    float du =
        sqrt(
            radialTerm
        );

    vec2 state =
        vec2(
            u,
            du
        );

    vec2 previousState =
        state;

    float previousPhi =
        0.0;

    // Initial position
    vec3 previousPosition =
        cameraPosition;
    // Integrate
    for(int i = 0;
        i < 4500;
        ++i)
    {
        float currentU =
            max(
                state.x,
                0.0000001
            );

        float r =
            1.0 /
            currentU;

        // Current orbital basis

        float c =
            cos(result.phi);

        float s =
            sin(result.phi);

        vec3 er =
            c * eR +
            s * ePhi;

        vec3 ephi =
            -s * eR +
            c * ePhi;

        vec3 position =
            r *
            er;

        // Event horizon
        
        if(
            r <=
            RS * 1.0002
        )
        {
            result.captured =
                true;

            return result;
        }

        
        // Check intersection with disk plane y = 0.
        

        float previousY =
            previousPosition.y;

        float currentY =
            position.y;

        if(
            previousY *
            currentY
            <= 0.0
            &&
            result.phi > 0.005
        )
        {
            float denominator =
                previousY -
                currentY;

            if(
                abs(denominator)
                >
                0.000001
            )
            {
                float t =
                    previousY /
                    denominator;

                t =
                    clamp(
                        t,
                        0.0,
                        1.0
                    );

                vec3 hit =
                    mix(
                        previousPosition,
                        position,
                        t
                    );

                float diskR =
                    length(
                        hit.xz
                    );
                // Disk exists from ISCO outward.
                
                if(
                    diskR >= RISCO &&
                    diskR <= 35.0
                )
                {
                    // Calculate derivative of position.
                    vec2 hitState =
                        mix(
                            previousState,
                            state,
                            t
                        );

                    float hitU =
                        max(
                            hitState.x,
                            0.000001
                        );

                    float hitR =
                        1.0 /
                        hitU;

                    float drdphi =
                        -hitState.y /
                        (
                            hitU *
                            hitU
                        );

                    float ph =
                        mix(
                            previousPhi,
                            result.phi,
                            t
                        );

                    float cc =
                        cos(ph);

                    float ss =
                        sin(ph);

                    vec3 hitER =
                        cc * eR +
                        ss * ePhi;

                    vec3 hitEPhi =
                        -ss * eR +
                        cc * ePhi;

                    vec3 photonDerivative =
                        drdphi *
                        hitER +
                        hitR *
                        hitEPhi;

                    vec3 photonDirection =
                        normalize(
                            photonDerivative
                        );

                    // This is the direction from emitter
                    // toward the observer.
                    photonDirection =
                        -photonDirection;

                    result.diskHit =
                        true;

                    result.diskRadius =
                        diskR;

                    result.diskPosition =
                        hit;

                    result.diskPhotonDirection =
                        photonDirection;

                    return result;
                }
            }
        }

        // Escape
        if(
            r >
            CAMERA_R * 1.01
            &&
            result.phi >
            0.5
        )
        {
            result.escapeDirection =
                normalize(
                    position
                );

            return result;
        }
        // Adaptive step.
        //
        // Much smaller near the photon sphere
        float proximity =
            abs(
                r -
                RPH
            );

        float h =
            mix(
                0.0018,
                0.012,
                smoothstep(
                    0.03,
                    2.0,
                    proximity
                )
            );

        previousState =
            state;

        previousPosition =
            position;

        previousPhi =
            result.phi;

        state =
            rk4(
                state,
                h
            );

        result.phi +=
            h;

        // Numerical safety
        if(
            state.x < 0.0
        )
        {
            result.escapeDirection =
                normalize(
                    position
                );

            return result;
        }

        if(
            result.phi >
            45.0
        )
        {
            result.captured =
                true;

            return result;
        }
    }

    result.captured =
        true;

    return result;
}
// STAR FIELD

vec3 stars(
    vec3 direction)
{
    // Convert direction to spherical coordinates.

    float longitude =
        atan(
            direction.z,
            direction.x
        );

    float latitude =
        asin(
            clamp(
                direction.y,
                -1.0,
                1.0
            )
        );

    vec2 sky =
        vec2(
            longitude / TWO_PI,
            latitude / PI
        );

    vec3 color =
        vec3(
            0.001,
            0.0015,
            0.003
        );

    // Very subtle galactic structure

    float milky =
        exp(
            -abs(
                direction.y
            )
            * 7.0
        );

    color +=
        vec3(
            0.007,
            0.009,
            0.018
        )
        *
        milky;
    // Stars
    for(int layer = 0;
        layer < 4;
        ++layer)
    {
        float scale =
            85.0 +
            float(layer) *
            70.0;

        vec2 grid =
            sky *
            scale;

        vec2 cell =
            floor(grid);

        vec2 local =
            fract(grid)
            -
            0.5;

        float h =
            hash21(
                cell +
                float(layer) *
                41.7
            );

        float star =
            smoothstep(
                0.025,
                0.0,
                length(local)
            );

        float probability =
            step(
                0.995 -
                float(layer) *
                0.0008,
                h
            );

        vec3 starColor =
            mix(
                vec3(
                    0.58,
                    0.72,
                    1.0
                ),
                vec3(
                    1.0,
                    0.78,
                    0.52
                ),
                h
            );

        color +=
            starColor *
            star *
            probability;
    }

    return color;
}

// DISK RENDERING
vec3 renderDisk(
    RayResult ray)
{
    if(!ray.diskHit)
        return vec3(0.0);

    float r =
        ray.diskRadius;

    // Temperature
    float temperature =
        diskTemperature(r);

    vec3 color =
        blackbody(
            temperature
        );

    // Disk coordinates
    vec3 p =
        ray.diskPosition;

    float azimuth =
        atan(
            p.z,
            p.x
        );

    // Differential rotation
    //
    // Omega ~ sqrt(M/r³)
    float omega =
        sqrt(
            M /
            (r * r * r)
        );

    float rotatingAngle =
        azimuth +
        omega *
        uTime *
        2.5;
    // Turbulent structure

    vec2 diskUV =
        vec2(
            r * 0.45,
            rotatingAngle * 5.0
        );

    float turbulence =
        fbm(
            diskUV
        );

    float fine =
        noise(
            diskUV *
            3.0
        );

    float filaments =
        mix(
            0.65,
            1.35,
            turbulence
        );

    filaments *=
        mix(
            0.8,
            1.15,
            fine
        );
    // Relativistic orbital velocity
    //
    // Physical speed relative to a static observer:
    //
    // v = sqrt(M/(r-2M))

    float beta =
        sqrt(
            M /
            max(
                r - 2.0 * M,
                0.0001
            )
        );

    beta =
        clamp(
            beta,
            0.0,
            0.95
        );

    float gamma =
        1.0 /
        sqrt(
            1.0 -
            beta *
            beta
        );
    // Tangential velocity direction
    // Disk lies in XZ plane.

    vec3 radial =
        normalize(
            vec3(
                p.x,
                0.0,
                p.z
            )
        );

    vec3 velocityDirection =
        normalize(
            cross(
                vec3(
                    0.0,
                    1.0,
                    0.0
                ),
                radial
            )
        );

    vec3 velocity =
        velocityDirection *
        beta;
    // Photon direction toward observer.

    float dopplerAngle =
        dot(
            velocity,
            ray.diskPhotonDirection
        );

    // Gravitational redshift

    float gravitational =
        sqrt(
            max(
                1.0 -
                2.0 * M / r,
                0.0001
            )
        );

    // Relativistic frequency shift
    //
    // g =
    // sqrt(1-2M/r) /
    // [gamma(1-v.n)]

    float g =
        gravitational /
        (
            gamma *
            (
                1.0 -
                dopplerAngle
            )
        );

    g =
        clamp(
            g,
            0.15,
            2.8
        );
   
    // Relativistic intensity transfer

    float intensity =
        diskEmission(r);

    intensity *=
        pow(
            g,
            3.0
        );

    intensity *=
        filaments;
    // Hot inner edge

    float innerGlow =
        exp(
            -pow(
                (
                    r -
                    RISCO
                )
                /
                1.5,
                2.0
            )
        );

    intensity +=
        innerGlow *
        0.35 *
        g;

    return
        color *
        intensity *
        2.2;
}

// PHOTON RING
//
// Only a very subtle enhancement.
// Most of the ring comes naturally from the traced disk.

vec3 photonRing(
    float b)
{
    float distance =
        abs(
            b -
            BCRIT
        );

    float ring =
        exp(
            -pow(
                distance /
                0.045,
                2.0
            )
        );

    return
        vec3(
            1.0,
            0.66,
            0.25
        )
        *
        ring
        *
        0.12;
}
// MAIN

void main()
{
  
    // Camera
   

    Camera camera =
        makeCamera();

    // Screen coordinate

    vec2 p =
        uv *
        2.0 -
        1.0;

    p.x *=
        uResolution.x /
        uResolution.y;

  
    // Cinematic 24° field of view

    float fov =
        radians(
            24.0
        );

    float focal =
        tan(
            fov *
            0.5
        );

    vec3 rayDirection =
        normalize(
            camera.forward +
            p.x *
            focal *
            camera.right +
            p.y *
            focal *
            camera.up
        );

   
    // Trace


    RayResult ray =
        traceRay(
            camera.position,
            rayDirection
        );

    vec3 color;

    // BLACK HOLE

    if(ray.captured)
    {
        color =
            vec3(
                0.0
            );

        // Very faint photon boundary.
        color +=
            photonRing(
                ray.impact
            );
    }
    // DISK

    else if(ray.diskHit)
    {
        color =
            stars(
                ray.escapeDirection
            );

        color +=
            renderDisk(
                ray
            );

        // Background light is heavily suppressed when
        // a disk ray intersects the emitter.
    }

    // BACKGROUND
    else
    {
        color =
            stars(
                ray.escapeDirection
            );
    }


    // Very subtle critical ring.

    color +=
        photonRing(
            ray.impact
        );

    // Soft central contrast
    

    float screenRadius =
        length(p);

    float vignette =
        smoothstep(
            1.45,
            0.40,
            screenRadius
        );

    color *=
        vignette;

   
    // Filmic exposure

    color *=
        1.15;

    color =
        color /
        (
            color +
            vec3(1.0)
        );

  
    // Gamma
    
    color =
        pow(
            color,
            vec3(
                1.0 / 2.2
            )
        );

    FragColor =
        vec4(
            color,
            1.0
        );
}

)GLSL";


GLuint compileShader(
    GLenum type,
    const char* source)
{
    GLuint shader =
        glCreateShader(
            type
        );

    glShaderSource(
        shader,
        1,
        &source,
        nullptr
    );

    glCompileShader(
        shader
    );

    GLint success;

    glGetShaderiv(
        shader,
        GL_COMPILE_STATUS,
        &success
    );

    if (!success)
    {
        char log[8192];

        glGetShaderInfoLog(
            shader,
            sizeof(log),
            nullptr,
            log
        );

        std::cerr
            << "\nSHADER ERROR:\n"
            << log
            << "\n";
    }

    return shader;
}

GLuint createProgram()
{
    GLuint vs =
        compileShader(
            GL_VERTEX_SHADER,
            vertexShader
        );

    GLuint fs =
        compileShader(
            GL_FRAGMENT_SHADER,
            fragmentShader
        );

    GLuint program =
        glCreateProgram();

    glAttachShader(
        program,
        vs
    );

    glAttachShader(
        program,
        fs
    );

    glLinkProgram(
        program
    );

    GLint success;

    glGetProgramiv(
        program,
        GL_LINK_STATUS,
        &success
    );

    if (!success)
    {
        char log[8192];

        glGetProgramInfoLog(
            program,
            sizeof(log),
            nullptr,
            log
        );

        cerr
            << "\nPROGRAM ERROR:\n"
            << log
            << "\n";
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

int main()
{


    if (!glfwInit())
    {
        cerr
            << "GLFW initialization failed.\n";

        return -1;
    }

    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MAJOR,
        3
    );

    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MINOR,
        3
    );

    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );

#ifdef __APPLE__
    glfwWindowHint(
        GLFW_OPENGL_FORWARD_COMPAT,
        GL_TRUE
    );
#endif

    GLFWwindow* window =
        glfwCreateWindow(
            WIDTH,
            HEIGHT,
            "DUKE",
            nullptr,
            nullptr
        );

    if (!window)
    {
        glfwTerminate();

        return -1;
    }

    glfwMakeContextCurrent(
        window
    );

    glfwSetFramebufferSizeCallback(
        window,
        framebuffer_callback
    );

    glfwSetCursorPosCallback(
        window,
        mouse_callback
    );

    glfwSwapInterval(1);

    if (!gladLoadGLLoader(
        (GLADloadproc)
        glfwGetProcAddress))
    {
        cerr
            << "GLAD initialization failed.\n";

        return -1;
    }

    cout
        << "\n"
        << " SCHWARZSCHILD BLACK HOLE RAY TRACER\n"

        << "G = c = 1\n"
        << "M = 1\n\n"

        << "Event horizon     : r = 2M\n"
        << "Photon sphere     : r = 3M\n"
        << "ISCO              : r = 6M\n"
        << "Critical b        : 3 sqrt(3) M\n\n"

        << "Ray equation:\n"
        << "    u'' + u = 3Mu^2\n\n"

        << "Integrator:\n"
        << "    RK4\n\n"

        << "Disk:\n"
        << "    Schwarzschild thin disk\n"
        << "    relativistic Doppler shift\n"
        << "    gravitational redshift\n"
        << "    turbulent emissivity\n\n"

        << "Camera:\n"
        << "    automatic orbital motion\n"
        << "    mouse-controlled inclination\n\n";

    float vertices[] =
    {
        -1.0f, -1.0f,
         3.0f, -1.0f,
        -1.0f,  3.0f
    };

    GLuint VAO;
    GLuint VBO;

    glGenVertexArrays(
        1,
        &VAO
    );

    glGenBuffers(
        1,
        &VBO
    );

    glBindVertexArray(
        VAO
    );

    glBindBuffer(
        GL_ARRAY_BUFFER,
        VBO
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        nullptr
    );

    glEnableVertexAttribArray(
        0
    );

    glBindVertexArray(0);
    GLuint program =
        createProgram();

    GLint resolutionLoc =
        glGetUniformLocation(
            program,
            "uResolution"
        );

    GLint mouseLoc =
        glGetUniformLocation(
            program,
            "uMouse"
        );

    GLint timeLoc =
        glGetUniformLocation(
            program,
            "uTime"
        );
    while (
        !glfwWindowShouldClose(
            window
        )
        )
    {
        int width;
        int height;

        glfwGetFramebufferSize(
            window,
            &width,
            &height
        );

        float time =
            static_cast<float>(
                glfwGetTime()
                );

        glViewport(
            0,
            0,
            width,
            height
        );

        glClearColor(
            0.0f,
            0.0f,
            0.0f,
            1.0f
        );

        glClear(
            GL_COLOR_BUFFER_BIT
        );

        glUseProgram(
            program
        );

        glUniform2f(
            resolutionLoc,
            static_cast<float>(
                width
                ),
            static_cast<float>(
                height
                )
        );

        glUniform2f(
            mouseLoc,
            static_cast<float>(
                mouseX
                ),
            static_cast<float>(
                mouseY
                )
        );

        glUniform1f(
            timeLoc,
            time
        );

        glBindVertexArray(
            VAO
        );

        glDrawArrays(
            GL_TRIANGLES,
            0,
            3
        );

        glfwSwapBuffers(
            window
        );

        glfwPollEvents();

        if (
            glfwGetKey(
                window,
                GLFW_KEY_ESCAPE
            )
            ==
            GLFW_PRESS
            )
        {
            glfwSetWindowShouldClose(
                window,
                true
            );
        }
    }
    glDeleteVertexArrays(
        1,
        &VAO
    );

    glDeleteBuffers(
        1,
        &VBO
    );

    glDeleteProgram(
        program
    );

    glfwDestroyWindow(
        window
    );

    glfwTerminate();

    return 0;
}