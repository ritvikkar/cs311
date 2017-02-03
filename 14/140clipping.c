/*
 * 140clipping.c
 * by Ritvik Kar
 * CS 331: Computer Graphics
*/

/*clipViewPort performs the viewport transformation and the homogeneous division
 *one a given vertex
 */
void clipViewPort(renRenderer *ren, double vertex[]) {
    double scale[renVARYDIMBOUND];

    vecScale(ren->attrDim, 1.0/vert[renVARYW], vertex, scale);
    mat441Multiply(ren->viewport, scale, vertex);
}

/*clipScale takes in two points a,b and finds a new point that*/
void clipScale(renRenderer *ren, double unif[], texTexture *tex[],
                double a[], double b[], double newPoint[])
{
    double bMinusA[renVARYDIMBOUND];
    vecSubtract(ren->varyDim,b,a,bMinusA);
    double t1 = (a[3]-a[2])/(a[3]-a[2]+b[2]-b[3]);
    vecScale(ren->attrDim, t1, bMinusA, newPoint);
}

/*clipOne, i.e. one side is clipped, will emit two triangles*/
void clipOne(renRenderer *ren, double unif[], texTexture *tex[], 
				double a[], double b[], double c[], int clip[]) {
    //new points = t(b-a) & t(c-a)
    //t = (a3 - a2) / (a3 - a2 + b2 - b3)
    printf("clipOne\n");

    double point1[renVARYDIMBOUND],point2[renVARYDIMBOUND];    
    clipScale(ren,unif,tex,a,b,point1);
    clipScale(ren,unif,tex,a,b,point2);        

    clipViewPort(ren,point1);
    clipViewPort(ren,point2);    
    clipViewPort(ren,b);
    clipViewPort(ren,c);
    triRender(ren, unif, tex, point1, b, c);
    triRender(ren, unif, tex, point2, b, point1);    
}

/*clipTwo, i.e. two sides are clipped, will emit one triangle*/
void clipTwo(renRenderer *ren, double unif[], texTexture *tex[], 
				double a[], double b[], double c[], int clip[]) {
    printf("clipTwo\n");
    
    double point1[renVARYDIMBOUND],point2[renVARYDIMBOUND];    
    clipScale(ren,unif,tex,a,b,point1);
    clipScale(ren,unif,tex,a,b,point2);        

    clipViewPort(ren,point1);
    clipViewPort(ren,point2);    
    clipViewPort(ren,b);
    triRender(ren, unif, tex, point2, b, point1);
}

/*chipChecker checks the type of clipping that has occured and assigns
 *which helper should be called
 */
void clipChecker(renRenderer *ren, double unif[], texTexture *tex[], 
						double a[], double b[], double c[]){
    //backface skulling
    double ba[2], ca[2], xa[2];
    vecSubtract(2,c,a,ca); //c-a
    vecSubtract(2,b,a,ba); //b-a

    double baca[2][2];//scratch space
    mat22Columns(ba,ca,baca);// [b-a|c-a]
    double bacaInv[2][2];
    mat22Invert(baca,bacaInv);//[baca]^-1

    if(mat22Invert(baca,bacaInv) <= 0)
        return ;
    
    int clip[3];//array to keep flags for clipping
    clip[0]=clip[1]=clip[2]=0;

    printf("%d %d %d\n",clip[0],clip[1],clip[2]);

	//a is clipped
    if (a[renVARYW] <= 0 || a[renVARYZ] > a[renVARYW])
    	clip[0] = 1;

	//b is clipped
    if (b[renVARYW] <= 0 || b[renVARYZ] > b[renVARYW])
    	clip[1] = 1;
    
    //c is clipped
    if (c[renVARYW] <= 0 || c[renVARYZ] > c[renVARYW])
    	clip[2] = 1;

    
    printf("%d %d %d\n",clip[0],clip[1],clip[2]);

    //all vertices are clipped,render nothing
    if (clip[0] == 1 && clip[1] == 1 && clip[2] == 1)
    	return	;
    
    //none are clipped, render triangle
    else if (clip[0] == 0 && clip[1] == 0 && clip[2] == 0)
    {
    	//put the vertices through the viewport transformation
	    clipViewPort(ren,a);
	    clipViewPort(ren,b);
	    clipViewPort(ren,c);
	    triRender(ren, unif, tex, a, b, c);
    }

    //six remaining cases of clipping
    else{
    	if(clip[0] == 1 && clip[1] == 0 && clip[2] == 0)
    		clipOne(ren,unif, tex, a, b, c, clip);
    	else if(clip[0] == 0 && clip[1] == 1 && clip[2] == 0)
    		clipOne(ren,unif, tex, a, b, c, clip);
    	else if(clip[0] == 0 && clip[1] == 0 && clip[2] == 1) 
    		clipOne(ren,unif, tex, a, b, c, clip);   		
    	else if(clip[0] == 1 && clip[1] == 1 && clip[2] == 0)
       		clipTwo(ren,unif, tex, a, b, c, clip);
    	else if(clip[0] == 0 && clip[1] == 1 && clip[2] == 1)
       		clipTwo(ren,unif, tex, a, b, c, clip);
    	else if(clip[0] == 1 && clip[1] == 0 && clip[2] == 1)
       		clipTwo(ren,unif, tex, a, b, c, clip);
    }
}

void clipRender(renRenderer *ren, double unif[], texTexture *tex[], 
					double a[], double b[], double c[]) {
    //check the orientation of the triangle
    if (((a[0]<=b[0]) && (a[0]<=c[0]))){
        clipChecker(ren,unif,tex,a,b,c); // a is left most
    }
    else if (((b[0]<=c[0]) && (b[0]<=a[0]))){
        clipChecker(ren,unif,tex,b,c,a); //b is left most
    }
    else if (((c[0]<=b[0]) && (c[0]<=a[0]))){
        clipChecker(ren,unif,tex,c,a,b); //c is left most
    }
}