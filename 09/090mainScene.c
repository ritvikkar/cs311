/*
 * 090mainScene.c
 * by Ritvik Kar
 * CS 331: Computer Graphics
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include <GLFW/glfw3.h>
#include "000pixel.h"

#include "090matrix.c"
#include "071vector.c"
#include "040texture.c"
#include "090renderer.c"

#define renVARYDIMBOUND 16
#define renVERTNUMBOUND 1000

#define renATTRX 0
#define renATTRY 1
#define renATTRS 2
#define renATTRT 3
#define renATTRR 4
#define renATTRG 5
#define renATTRB 6

#define renVARYX 0
#define renVARYY 1
#define renVARYS 2
#define renVARYT 3
#define renVARYR 4
#define renVARYG 5
#define renVARYB 6

#define renUNIFR 0
#define renUNIFG 1
#define renUNIFB 2
#define renUNIFTHETA 0
#define renUNIFTRANSX 1
#define renUNIFTRANSY 2
#define renUNIFISOMETRY 3

#define renTEXR 0
#define renTEXG 1
#define renTEXB 2

renRenderer *ren;

texTexture *tex[1];
double unif[12] = {0.0,256.0,256.0};

/* Sets rgb, based on the other parameters, which are unaltered. attr is an 
interpolated attribute vector. */
void colorPixel(renRenderer *ren, double unif[], texTexture *tex[], 
        double vary[], double rgb[]) {
    texSample(tex[0], vary[renVARYS], vary[renVARYT]);
    rgb[0] = tex[0]->sample[renTEXR] * unif[renUNIFR] * vary[renVARYR];
    rgb[1] = tex[0]->sample[renTEXG] * unif[renUNIFG] * vary[renVARYG];
    rgb[2] = tex[0]->sample[renTEXB] * unif[renUNIFB] * vary[renVARYB];
}
/* Writes the vary vector, based on the other parameters. */
void transformVertex(renRenderer *ren, double unif[], double attr[], 
        double vary[]) {
    /* For now, just copy attr to varying. Baby steps. 
    vary[renVARYX] = attr[renATTRX];
    vary[renVARYY] = attr[renATTRY];
    vary[renVARYS] = attr[renATTRS];
    vary[renVARYT] = attr[renATTRT];*/
    double xy[2];
    double rTimesxy[2];
    double t[2];

    double r[2][2] = 
        {{ cos(unif[0]) , (-1)*sin(unif[0])}, 
         { sin(unif[0]) , cos(unif[0])}};

    xy[0] = attr[renATTRX];
    xy[1] = attr[renATTRY];

    mat221Multiply(r,xy,rTimesxy);

    t[0] = unif[1];
    t[1] = unif[2];
    vecAdd(2,t,rTimesxy,vary);

    vary[renVARYS] = attr[renATTRS];
    vary[renVARYT] = attr[renATTRT];

}

/* If unifParent is NULL, then sets the uniform matrix to the 
rotation-translation M described by the other uniforms. If unifParent is not 
NULL, but instead contains a rotation-translation P, then sets the uniform 
matrix to the matrix product P * M. */
void updateUniform(renRenderer *ren, double unif[], double unifParent[]) {
    if (unifParent == NULL)
        /* The nine uniforms for storing the matrix start at index 
        renUNIFISOMETRY. So &unif[renUNIFISOMETRY] is an array containing those 
        nine numbers. We use '(double(*)[3])' to cast it to a 3x3 matrix. */
        mat33Isometry(unif[renUNIFTHETA], unif[renUNIFTRANSX], 
            unif[renUNIFTRANSY], (double(*)[3])(&unif[renUNIFISOMETRY]));
    else {
        double m[3][3];
        mat33Isometry(unif[renUNIFTHETA], unif[renUNIFTRANSX], 
            unif[renUNIFTRANSY], m);
        mat333Multiply((double(*)[3])(&unifParent[renUNIFISOMETRY]), m, 
            (double(*)[3])(&unif[renUNIFISOMETRY]));
    }
}

#include "090triangle.c"
#include "090mesh.c"
meshMesh mesh;
void draw(void){
    pixClearRGB(0.0,0.0,0.0);
    //meshRender that that will call triRender
    meshRender(&mesh,ren,unif,tex);
  
    //triRender(ren, unif, tex, a,b,c);

}

void handleKeyDown(int key, int shiftIsDown, int controlIsDown,
        int altOptionIsDown, int superCommandIsDown){
    if(key == GLFW_KEY_ENTER)
    {
        if(tex[0]->filtering == texQUADRATIC)
            texSetFiltering(tex[0], texNEAREST);
            
        else
            texSetFiltering(tex[0], texQUADRATIC);
    }

    printf("key down %d, shift %d, control %d, altOpt %d, supComm %d\n",
        key, shiftIsDown, controlIsDown, altOptionIsDown, superCommandIsDown);
}

void handleTimeStep(double oldTime, double newTime) {
    if (floor(newTime) - floor(oldTime) >= 1.0)
        printf("handleTimeStep: %f frames/sec\n", 1.0 / (newTime - oldTime));
    unif[0] += (newTime - oldTime);
    draw();
}


int main(void) {
    /* Make a 512 x 512 window with the title 'Pixel Graphics'. This function 
    returns 0 if no error occurred. */ 
    ren = (renRenderer *)malloc(sizeof(renRenderer));
    tex[0] = (texTexture *)malloc(sizeof(texTexture));
    if (pixInitialize(512, 512, "Pixel Graphics") != 0)
        return 1;
    
    else {
        texInitializeFile(tex[0], "pic1.jpg");
        if (texInitializeFile(tex[0], "pic1.jpg") !=0)
        {
            return 1;
        }

        ren->attrDim = 4; ren->unifDim = 7; ren->texNum = 1; ren->varyDim = 4;
        ren->colorPixel = colorPixel;
        ren->transformVertex = transformVertex;
        ren->updateUniform = updateUniform;
        //initilize all the renderer values

        meshInitializeEllipse(&mesh,0,0,50,50,1000);
        //meshInitializeRectangle(&mesh,0,256,0,256);
        draw();

        pixSetTimeStepHandler(handleTimeStep);
        pixSetKeyDownHandler(handleKeyDown);
        texDestroy(tex[0]);
        pixRun();
        meshDestroy(&mesh);
        return 0;
    }
}
