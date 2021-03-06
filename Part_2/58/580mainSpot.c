/*
 * 580mainSpot.c
 * by Ritvik Kar & Martin Green
 * CS 311: Computer Graphics
*/

/* On macOS, compile with...
    clang -o spot32 580mainSpot.c /usr/local/gl3w/src/gl3w.o -lglfw -framework OpenGL -framework CoreFoundation
*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
/* Include the GL3W header before any other OpenGL-related headers. */
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <sys/time.h>

double getTime(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 0.000001;
}

#include "500shader.c"
#include "540texture.c"
#include "530vector.c"
#include "580mesh.c"
#include "520matrix.c"
#include "560light.c"
#include "520camera.c"
#include "580scene.c"

GLdouble alpha = 0.0;
GLuint program;
GLint attrLocs[3];
GLint viewingLoc, modelingLoc;
GLint texCoordsLoc, textureLoc[2];
GLint lightPosLoc, lightColLoc, lightAttLoc;
GLint cosLoc, dirLoc;
lightLight light;
GLint camPosLoc;
GLint unifLocs[1];
camCamera cam;
/* Allocate three meshes and three scene graph nodes. */
meshGLMesh rootMesh, childMesh, siblingMesh;
sceneNode rootNode, childNode, siblingNode;
texTexture texA, texB, texC;

GLuint vertBuffers[2];
GLuint vertVAO;

void handleError(int error, const char *description) {
    fprintf(stderr, "handleError: %d\n%s\n", error, description);
}

void handleResize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    camSetWidthHeight(&cam, width, height);
}

void handleKey(GLFWwindow *window, int key, int scancode, int action,
        int mods) {
    int shiftIsDown = mods & GLFW_MOD_SHIFT;
    int controlIsDown = mods & GLFW_MOD_CONTROL;
    int altOptionIsDown = mods & GLFW_MOD_ALT;
    int superCommandIsDown = mods & GLFW_MOD_SUPER;
    if (action == GLFW_PRESS && key == GLFW_KEY_L) {
        camSwitchProjectionType(&cam);
    } else if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_O)
            camAddTheta(&cam, -0.1);
        else if (key == GLFW_KEY_P)
            camAddTheta(&cam, 0.1);
        else if (key == GLFW_KEY_I)
            camAddPhi(&cam, -0.1);
        else if (key == GLFW_KEY_K)
            camAddPhi(&cam, 0.1);
        else if (key == GLFW_KEY_U)
            camAddDistance(&cam, -0.1);
        else if (key == GLFW_KEY_J)
            camAddDistance(&cam, 0.1);
    }
}

/* Returns 0 on success, non-zero on failure. Warning: If initialization fails 
midway through, then does not properly deallocate all resources. But that's 
okay, because the program terminates almost immediately after this function 
returns. */
int initializeScene(void) {
    /* Initialize meshes. */
    GLuint attrDims[3] = {3, 2, 3};
    meshMesh mesh;
    if (meshInitializeCapsule(&mesh, 0.5, 2.0, 16, 32) != 0)
        return 1;
    meshGLInitialize(&rootMesh, &mesh, 3, attrDims, 1);
    meshGLVAOInitialize(&rootMesh, 0, attrLocs);
    meshDestroy(&mesh);

    if (meshInitializeBox(&mesh, -0.5, 0.5, -0.5, 0.5, -0.5, 0.5) != 0)
        return 2;
    meshGLInitialize(&childMesh, &mesh, 3, attrDims, 1);
    meshGLVAOInitialize(&childMesh, 0, attrLocs);
    meshDestroy(&mesh);
    
    if (meshInitializeSphere(&mesh, 0.5, 16, 32) != 0)
        return 3;
    meshGLInitialize(&siblingMesh, &mesh, 3, attrDims, 1);
    meshGLVAOInitialize(&siblingMesh, 0, attrLocs);
    meshDestroy(&mesh);

    // int meshGLInitialize(meshGLMesh *meshGL, meshMesh *mesh, GLuint attrNum, 
    //     GLuint attrDims[], GLuint vaoNum)

    // void meshGLVAOInitialize(meshGLMesh *meshGL, GLuint index, GLint attrLocs[])

    /* Initialize scene graph nodes. */
    if (sceneInitialize(&siblingNode, 2, 2, &siblingMesh, NULL, NULL) != 0)
        return 4;
    if (sceneInitialize(&childNode, 2, 2, &childMesh, NULL, NULL) != 0)
        return 5;
    if (sceneInitialize(&rootNode, 2, 2, &rootMesh, &childNode, &siblingNode) != 0)
        return 6;
    
    /* Customize the uniforms. */
    GLdouble trans[3] = {1.0, 0.0, 0.0};
    sceneSetTranslation(&childNode, trans);
    vecSet(3, trans, 0.0, 1.0, 0.0);
    sceneSetTranslation(&siblingNode, trans);
    GLdouble unif[2] = {1.0, 1.0};
    
    sceneSetUniform(&siblingNode, unif);
    sceneSetUniform(&childNode, unif);
    sceneSetUniform(&rootNode, unif);

    GLdouble lightTranslation[3] = {1.0, 3.0, 0.0};
    GLdouble lightColor[3] = {1.0, 1.0, 1.0};
    GLdouble lightAttenuation[3] = {1.0, 0.0, 0.0};
    
    lightSetType(&light, lightSPOT);
    lightSetTranslation(&light, lightTranslation);
    lightSetColor(&light, lightColor);
    lightSetAttenuation(&light, lightAttenuation);
    lightSetSpotAngle(&light, 40.0);
    lightShineFrom(&light, lightTranslation, 0.0, 0.0);

    return 0;
}

void destroyScene(void) {
    meshGLDestroy(&siblingMesh);
    meshGLDestroy(&childMesh);
    meshGLDestroy(&rootMesh);
    sceneDestroyRecursively(&rootNode);
}

/* Returns 0 on success, non-zero on failure. */
int initializeShaderProgram(void) {
    GLchar vertexCode[] = "\
        #version 140\n\
        uniform mat4 viewing;\
        uniform mat4 modeling;\
        in vec3 position;\
        in vec2 texCoords;\
        in vec3 normal;\
        out vec3 fragPos;\
        out vec3 normalDir;\
        out vec2 st;\
        void main() {\
            vec4 worldPos = modeling * vec4(position, 1.0);\
            gl_Position = viewing * worldPos;\
            fragPos = vec3(worldPos);\
            normalDir = vec3(modeling * vec4(normal, 0.0));\
            st = texCoords;\
        }";
    GLchar fragmentCode[] = "\
        #version 140\n\
        uniform sampler2D textureA;\
        uniform sampler2D textureB;\
        uniform vec3 specular;\
        uniform vec3 camPos;\
        uniform vec3 lightPos;\
        uniform vec3 lightCol;\
        uniform vec3 lightAtt;\
        uniform float halfCos;\
        uniform vec3 aim;\
        in vec3 fragPos;\
        in vec3 normalDir;\
        in vec2 st;\
        out vec4 fragColor;\
        void main() {\
            vec3 surfCol = vec3((texture(textureA, st) + texture(textureB, st)) * 0.5);\
            vec3 norDir = normalize(normalDir);\
            vec3 litDir = normalize(lightPos - fragPos);\
            vec3 camDir = normalize(camPos - fragPos);\
            vec3 aimDir = normalize(aim);\
            vec3 refDir = 2.0 * dot(litDir, norDir) * norDir - litDir;\
            float d = distance(lightPos, fragPos);\
            float a = lightAtt[0] + lightAtt[1] * d + lightAtt[2] * d * d;\
            float diffInt = dot(norDir, litDir) / a;\
            float specInt = dot(refDir, camDir);\
            float cosGam = dot(aimDir, -1.0 * litDir);\
            if (diffInt <= 0.0 || specInt <= 0.0)\
                specInt = 0.0;\
            float ambInt = 0.5;\
            if (diffInt <= ambInt)\
                diffInt = ambInt;\
            vec3 diffLight = diffInt * lightCol * surfCol;\
            float shininess = 64.0;\
            vec3 specLight = pow(specInt / a, shininess) * lightCol * specular;\
            if (cosGam >= halfCos) {\
                fragColor = vec4(diffLight + specLight, 1.0);\
            } else {\
                fragColor = vec4(ambInt * diffLight, 1.0);\
            }\
        }";
    program = makeProgram(vertexCode, fragmentCode);
    if (program != 0) {
        glUseProgram(program);
        attrLocs[0] = glGetAttribLocation(program, "position");
        attrLocs[1] = glGetAttribLocation(program, "texCoords");
        attrLocs[2] = glGetAttribLocation(program, "normal");
        viewingLoc = glGetUniformLocation(program, "viewing");
        modelingLoc = glGetUniformLocation(program, "modeling");
        unifLocs[0] = glGetUniformLocation(program, "specular");
        camPosLoc = glGetUniformLocation(program, "camPos");
        textureLoc[0] = glGetUniformLocation(program, "textureA");
        textureLoc[1] = glGetUniformLocation(program, "textureB");
        lightPosLoc = glGetUniformLocation(program, "lightPos");
        lightColLoc = glGetUniformLocation(program, "lightCol");
        lightAttLoc = glGetUniformLocation(program, "lightAtt");
        cosLoc = glGetUniformLocation(program, "halfCos");
        dirLoc = glGetUniformLocation(program, "aim");    
    }
    return (program == 0);
}

int initializeTex() {
    /* A 'texture unit' is a piece of machinery inside the GPU that performs 
    texture mapping. A GPU might have many texture units, allowing you to map 
    many textures onto your meshes in complicated ways. The glActiveTexture 
    function selects which texture unit is affected by subsequent OpenGL calls. 
    In this tutorial, we use only texture unit 0, so we activate it here, once 
    and for all. */

    if (texInitializeFile(&texA, "capsule.jpg", GL_LINEAR, GL_LINEAR, 
            GL_REPEAT, GL_REPEAT) != 0)
        return 1;


    if (texInitializeFile(&texB, "box.jpg", GL_LINEAR, GL_LINEAR, 
            GL_REPEAT, GL_REPEAT) != 0)
        return 2;

    if (texInitializeFile(&texC, "earth.jpg", GL_LINEAR, GL_LINEAR, 
            GL_REPEAT, GL_REPEAT) != 0)
        return 3;


    sceneSetOneTexture(&rootNode, 0, &texA);
    sceneSetOneTexture(&rootNode, 1, &texB);

    sceneSetOneTexture(&siblingNode, 0, &texC);
    sceneSetOneTexture(&siblingNode, 1, &texC);

    sceneSetOneTexture(&childNode, 0, &texB);
    sceneSetOneTexture(&childNode, 1, &texB);

    return 0;
}

void render(void) {
    /* This part is the same as in 520mainCamera.c. */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);
    camRender(&cam, viewingLoc);

    /* calculating the camera position */
    GLdouble camPos[3];
    vecCopy(3, cam.translation, camPos);
    GLfloat camFloat[3];
    vecOpenGL(3, camPos, camFloat);
    glUniform3fv(camPosLoc, 1, camFloat);

    /* This animation code is different from that in 520mainCamera.c. */
    GLdouble rot[3][3], identity[4][4], axis[3] = {1.0, 1.0, 1.0};
    vecUnit(3, axis, axis);
    alpha += 0.01;
    mat33AngleAxisRotation(alpha, axis, rot);
    sceneSetRotation(&rootNode, rot);
    sceneSetOneUniform(&rootNode, 0, 0.5 + 0.5 * sin(alpha * 7.0));

    /* This rendering code is different from that in 520mainCamera.c. */
    mat44Identity(identity);
    GLuint unifDims[1] = {2};

    lightRender(&light, lightPosLoc, lightColLoc, lightAttLoc, dirLoc, cosLoc);    
    sceneRender(&rootNode, identity, modelingLoc, 1, unifDims, unifLocs, 0, textureLoc);
}

int main(void) {
    double oldTime;
    double newTime = getTime();

    glfwSetErrorCallback(handleError);
    if (glfwInit() == 0){
        fprintf(stderr, "main: glfwInit failed.\n");
        return 1;
    }

    /* Ask GLFW to supply an OpenGL 3.2 context. */
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    GLFWwindow *window;
    window = glfwCreateWindow(512, 512, "Spotlight 3.2", NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        return 2;
    }

    glfwSetWindowSizeCallback(window, handleResize);
    glfwSetKeyCallback(window, handleKey);
    glfwMakeContextCurrent(window);

    /* You might think that getting an OpenGL 3.2 context would make OpenGL 3.2 
    available to us. But you'd be wrong. The following call 'loads' a bunch of 
    OpenGL 3.2 functions, so that we can use them. This is why we use GL3W. */    
    if (gl3wInit() != 0) {
        fprintf(stderr, "main: gl3wInit failed.\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 3;
    }
    /* We rarely invoke any GL3W functions other than gl3wInit. But just for an 
    educational example, let's ask GL3W about OpenGL support. */    
    if (gl3wIsSupported(3, 2) == 0)
        fprintf(stderr, "main: OpenGL 3.2 is not supported.\n");
    else
        fprintf(stderr, "main: OpenGL 3.2 is supported.\n");

    fprintf(stderr, "main: OpenGL %s, GLSL %s.\n", 
        glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
    glEnable(GL_DEPTH_TEST);
    glDepthRange(1.0, 0.0);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    /* Initialize a whole scene, rather than just one mesh. */
    if (initializeShaderProgram() != 0)
        return 4;
    if (initializeScene() != 0)
        return 5;

    if (initializeTex() != 0) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 6;
    }

    GLdouble target[3] = {0.0, 0.0, 0.0};
    camSetControls(&cam, camPERSPECTIVE, M_PI / 6.0, 10.0, 512.0, 512.0, 10.0, 
        M_PI / 4.0, M_PI / 4.0, target);

    while (glfwWindowShouldClose(window) == 0) {
        oldTime = newTime;
        newTime = getTime();
        if (floor(newTime) - floor(oldTime) >= 1.0)
            fprintf(stderr, "main: %f frames/sec\n", 1.0 / (newTime - oldTime));
        render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(program);
    /* Don't forget to destroy the whole scene. */
    texDestroy(&texA);
    texDestroy(&texB);
    texDestroy(&texC);
    destroyScene();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}