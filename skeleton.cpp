
#include <iostream>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModel.h"
#include <csignal>

using namespace std;
using glm::vec3;
using glm::ivec2;
using glm::vec2;
using glm::mat3;

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES

const int SCREEN_WIDTH =  300;
const int SCREEN_HEIGHT = 300;
SDL_Surface* screen;
int t;
vector<Triangle> triangles;
vec3 white(1,1,1);
vec3 red(1,0,0);
vec3 currentColor;
vec3 cameraPos( 0, 0, -3.001 );
mat3 R;
float yaw = 0; // Yaw angle controlling camera rotation around y-axis
float focal_length=SCREEN_WIDTH/2;
int cpt=1;
float depthBuffer[SCREEN_HEIGHT][SCREEN_WIDTH];

float tmpZinvLeft = 0.0,tmpZinvRight = 0.0;

struct Pixel
{
int x;
int y;
float zinv;
};

// ----------------------------------------------------------------------------
// FUNCTIONS

void Update();
void Draw();
void VertexShader( const vec3& v, Pixel& p );
void Interpolate( Pixel a, Pixel b, vector<Pixel>& result );
void DrawLineSDL( SDL_Surface* surface, ivec2 a, ivec2 b, vec3 color );
void DrawPolygonEdges( const vector<vec3>& vertices);
void ComputePolygonRows(
const vector<Pixel>& vertexPixels,
vector<Pixel>& leftPixels,
vector<Pixel>& rightPixels
);
void DrawPolygon( const vector<vec3>& vertices );
void DrawPolygonRows( vector<Pixel> leftPixels, vector<Pixel> rightPixels );

int main( int argc, char* argv[] )
{

	LoadTestModel( triangles );
	screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT );
	t = SDL_GetTicks();	// Set start value for timer.

	while( NoQuitMessageSDL() )
	{

		Update();
		Draw();
		cout << " out of a draw\n=====================\n";
	}

	SDL_SaveBMP( screen, "screenshot.bmp" );
	return 0;
}

void Update()
{
	// Compute frame time:
	int t2 = SDL_GetTicks();
	float dt = float(t2-t);
	t = t2;
	cout << "Render time: " <<dt <<"ms."<<endl;

	Uint8* keystate = SDL_GetKeyState(0);

	if( keystate[SDLK_UP] )
		cameraPos.z+=0.05;
		;

	if( keystate[SDLK_DOWN] )
		cameraPos.z-=0.05;
		;

	if( keystate[SDLK_RIGHT] )
		cameraPos.x+=0.05;
		;

	if( keystate[SDLK_LEFT] )
		cameraPos.x-=0.05;
		;

	if( keystate[SDLK_RSHIFT] )
		;

	if( keystate[SDLK_RCTRL] )
		;

	if( keystate[SDLK_w] )
		cameraPos.z+=0.05;
		;

	if( keystate[SDLK_s] )
		cameraPos.z-=0.05;		
		;

	if( keystate[SDLK_d] )
		yaw+=0.01;
		;

	if( keystate[SDLK_a] )
		yaw-=0.01;
		;

	if( keystate[SDLK_e] )
		;

	if( keystate[SDLK_q] )
		;
}

void Draw()
{
	//cout << "In the draw function\n";
	SDL_FillRect( screen, 0, 0 );
		for( int y=0; y<SCREEN_HEIGHT; ++y ){
			for( int x=0; x<SCREEN_WIDTH; ++x ){
				depthBuffer[x][y] = 0;	
			}
		}

	if( SDL_MUSTLOCK(screen) )
		SDL_LockSurface(screen);
	for( int i=0; i<triangles.size(); ++i )
	{
		currentColor = triangles[i].color;
		vector<vec3> vertices(3);
		vertices[0] = triangles[i].v0;
		vertices[1] = triangles[i].v1;
		vertices[2] = triangles[i].v2;
		//cout << "vertices["<<0<<"] :("<<vertices[0].x<<","<<vertices[0].y<<","<<vertices[0].z<<");\n";
		//cout << "vertices["<<1<<"] :("<<vertices[1].x<<","<<vertices[1].y<<","<<vertices[1].z<<");\n";
		//cout << "vertices["<<2<<"] :("<<vertices[2].x<<","<<vertices[2].y<<","<<vertices[2].z<<");\n";
		DrawPolygon( vertices );
	}
	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);
	SDL_UpdateRect( screen, 0, 0, 0, 0 );
}

// THE PROBLEM PROBABLY COMES FROM HERE
void VertexShader( const vec3& v, Pixel& p ){
	//It should take the 3D position of a vertex v and compute its 2D image position and store it in the given 2D
	//integer vector p. glm::ivec2 is a data type for 2D integer vectors, i.e. a pixel position will be represented by two
	//integers.

	cout << "in function VertexShader _ 1\n";

	vec3 newV = v-cameraPos;

	float cosYaw = cos(yaw);
	float sinYaw = sin(yaw);
	vec3 firstColumn = vec3(cosYaw, 0.0, -sinYaw);
	vec3 secondColumn = vec3(0.0, 1.0, 0.0);
	vec3 thirdColumn = vec3(sinYaw, 0.0, cosYaw);

	R = mat3(firstColumn,secondColumn,thirdColumn);

	// WE CHANGED d IN ORDER TO DO A ROTATION
	newV=R*newV;
	//cout << "newV:("<<newV.x<<","<<newV.y<<","<<newV.z<<")\n";
	p.x = focal_length * (newV.x/newV.z)  + SCREEN_WIDTH/2.f;
	p.y = focal_length * (newV.y/newV.z) +SCREEN_HEIGHT/2.f;
	// PROBLEM: DOUBT ABOUT THIS PART
	
	p.zinv = 1/newV.z;
	/*
	if (p.zinv>depthBuffer[p.x][p.y])
		depthBuffer[p.x][p.y]=p.zinv;
	*/
	//cout <<"pixel p:("<<p.x<<","<<p.y<<","<<p.zinv<<")\n";
	//cout <<"depthBuffer["<<p.x<<","<<p.y<<"]: "<<depthBuffer[p.x][p.y]<<"\n";
	
	}

// LOOKS OK
void Interpolate( Pixel a, Pixel b, vector<Pixel>& result )
{
	cout << "in function Interpolate _ 2\n";

	int N = result.size();
	vec3 step = vec3(b.x-a.x,b.y-a.y,b.zinv-a.zinv) / float(max(N-1,1));
	vec3 current;
	current.x=a.x;current.y=a.y;current.z=a.zinv;
	cout <<"N : "<<N<<"\n";

	for( int i=0; i<N; ++i )
	{
		//cout << "current : ("<<current.x<<","<<current.y<<","<<current.z<<")\n";
		result[i].x = current.x;
		result[i].y = current.y;
		result[i].zinv = current.z;
		current.x += step.x;
		current.y += step.y;
		current.z += step.z;
	}
}


// WE NEVER ENTER THIS FUNCTION NOW 
void DrawPolygonEdges( const vector<vec3>& vertices )
{
	cout << "in function DrawPolygonEdges _ 4\n";
	int V = vertices.size();
	// Transform each vertex from 3D world position to 2D image position:
	vector<Pixel> projectedVertices( V );
	for( int i=0; i<V; ++i )
	{
		VertexShader( vertices[i], projectedVertices[i] );
	}
	// Loop over all vertices and draw the edge from it to the next vertex:
	for( int i=0; i<V; ++i )
	{
		int j = (i+1)%V; // The next vertex
		vec3 color( 1, 1, 1 );
		ivec2 projectedVerticesVec2I
		(projectedVertices[i].x,projectedVertices[i].y);
		ivec2 projectedVerticesVec2J
		(projectedVertices[j].x,projectedVertices[j].y);
		
		DrawLineSDL( screen, projectedVerticesVec2I, projectedVerticesVec2J,
		color );
		//cout << " line drawned \n";

	}
}


// THE PROBLEM NOW COMES FROM HERE ?
void ComputePolygonRows(const vector<Pixel>& vertexPixels, 
	vector<Pixel>& leftPixels, vector<Pixel>& rightPixels )
{

	// vertexPixels contains the three points of a triangle 

	cout << "in function ComputePolygonRows _ 5\n";
	//for (int cpt=0;cpt<vertexPixels.size(); cpt++)
		//cout << "in the given vertexPixels["<<cpt<<"], values are :("<<vertexPixels[cpt].x<<","<<vertexPixels[cpt].y<<","<<vertexPixels[cpt].zinv<<")\n";

	int nbEdges = vertexPixels.size(); 

	int ROWS = 0;
	int distTMP1,distTMP2,distTMP3;
	int minY, maxY;
	int indexMin,indexMax, indexMiddle;
	int yMax,yMiddle,yMin;

	distTMP1 = abs(vertexPixels[0].y - vertexPixels[1].y);
	distTMP2 = abs(vertexPixels[0].y - vertexPixels[2].y);
	distTMP3 = abs(vertexPixels[1].y - vertexPixels[2].y);

	yMax = max(vertexPixels[0].y,
		max (vertexPixels[1].y,vertexPixels[2].y));
	if (yMax == vertexPixels[0].y){
		indexMax=0;
		if (vertexPixels[1].y > vertexPixels[2].y){
			yMiddle = vertexPixels[1].y; indexMiddle=1;
			yMin = vertexPixels[2].y; indexMin=2; 
			} 
			else {
			yMiddle = vertexPixels[2].y; indexMiddle=2;
			yMin = vertexPixels[1].y; indexMin=1; 
			}
	} else if ( yMax == vertexPixels[1].y){
		indexMax=1;
		if (vertexPixels[0].y > vertexPixels[2].y){
			yMiddle = vertexPixels[0].y; indexMiddle=0;
			yMin = vertexPixels[2].y; indexMin=2; 
			} 
			else {
			yMiddle = vertexPixels[2].y; indexMiddle=2;
			yMin = vertexPixels[0].y; indexMin=0; 
			}
	} else {
		indexMax=2;
		if (vertexPixels[0].y > vertexPixels[1].y){
			yMiddle = vertexPixels[0].y; indexMiddle=0;
			yMin = vertexPixels[1].y; indexMin=1; 
			} 
			else {
			yMiddle = vertexPixels[1].y; indexMiddle=1;
			yMin = vertexPixels[0].y; indexMin=0; 
			}
	}
	
	ROWS = yMax - yMin +1 ;


	cout <<"Step 1: yMax="<<yMax<<"_yMin="<<yMin<<"_yMiddle="<<yMiddle<<"\n";

// 2. Resize leftPixels and rightPixels
// so that they have an element for each row.
leftPixels.resize(ROWS);
rightPixels.resize(ROWS);

// 3. Initialize the x-coordinates in leftPixels
// to some really large value and the x-coordinates
// in rightPixels to some really small value.

for( int i=0; i<ROWS; ++i )
{
	leftPixels[i].x = + numeric_limits<int>::max();
	rightPixels[i].x = - numeric_limits<int>::max();
	leftPixels[i].zinv  = 1; 
	rightPixels[i].zinv = 1;
}

//4. Loop through all edges of the polygon and use
//linear interpolation to find the x-coordinate for
//each row it occupies. Update the corresponding
//values in rightPixels and leftPixels.

/*
	cout <<"Before interpolation for vecA,vecB,vecC:\n";
		cout <<"vertexPixels min : ("<<vertexPixels[indexMin].x<<","<<vertexPixels[indexMin].y<<","<<vertexPixels[indexMin].zinv<<")\n";
		cout <<"vertexPixels middle : ("<<vertexPixels[indexMiddle].x<<","<<vertexPixels[indexMiddle].y<<","<<vertexPixels[indexMiddle].zinv<<")\n";
		cout <<"vertexPixels max : ("<<vertexPixels[indexMax].x<<","<<vertexPixels[indexMax].y<<","<<vertexPixels[indexMax].zinv<<")\n";
	cout << "========================================";
*/

	vector <Pixel> vecA(ROWS);
	//cout <<"vecA.size:"<<vecA.size()<<"\n";
	 Interpolate(vertexPixels[indexMin],
		vertexPixels[indexMax], vecA);
	 vector <Pixel> vecB(
	 	abs(vertexPixels[indexMin].y - vertexPixels[indexMiddle].y) +1);
	//cout <<"vecB.size:"<<vecB.size()<<"\n";
	 Interpolate(vertexPixels[indexMin],
		vertexPixels[indexMiddle], vecB);
	 vector <Pixel> vecC(
	 	abs(vertexPixels[indexMiddle].y - vertexPixels[indexMax].y) +1);
	//cout <<"vecC.size:"<<vecC.size()<<"\n";
	 Interpolate(vertexPixels[indexMiddle],
		vertexPixels[indexMax], vecC);


	 for (int i =0; i<ROWS; i++){

	 	if (leftPixels[i].x > vecA[i].x && 
	 		leftPixels[i].zinv > vecA[i].zinv){
	 		leftPixels[i].x = vecA[i].x;
	 		leftPixels[i].y = vecA[i].y;
	 		leftPixels[i].zinv = vecA[i].zinv;
	 	}
	 	int varTest=0;
	 	// PROBLEM WITH THE TESTS ON THE ZINV
	 	if (i < vecB.size()){
	 		if (rightPixels[i].x < vecB[i].x
	 			//&& rightPixels[i].zinv > vecB[i].zinv
	 			){
	 			varTest=1;
	 			rightPixels[i].x = vecB[i].x;
	 			rightPixels[i].y = vecB[i].y;
	 			rightPixels[i].zinv = vecB[i].zinv;
	 		}
	 	} else {
	 		if (rightPixels[i].x < vecC[i-vecB.size()].x
	 			//&& rightPixels[i].zinv > vecC[i-vecB.size()].zinv
	 			){
	 			varTest=2;
	 			rightPixels[i].x = vecC[i-vecB.size()].x;
	 			rightPixels[i].y = vecC[i-vecB.size()].y;
	 			rightPixels[i].zinv = vecB[i-vecB.size()].zinv;
	 		}
	 	}
		 cout<<"Step 4 of ComputePolygonRows\n";

		 	/*
			 cout <<"vertexPixels min : ("<<vertexPixels[indexMin].x<<","<<vertexPixels[indexMin].y<<","<<vertexPixels[indexMin].zinv<<")\n";
			 cout <<"vertexPixels middle : ("<<vertexPixels[indexMiddle].x<<","<<vertexPixels[indexMiddle].y<<","<<vertexPixels[indexMiddle].zinv<<")\n";
			 cout <<"vertexPixels max : ("<<vertexPixels[indexMax].x<<","<<vertexPixels[indexMax].y<<","<<vertexPixels[indexMax].zinv<<")\n";

			cout << "depthBuffer[leftPixels[i] : "<<depthBuffer[leftPixels[i].x][leftPixels[i].y]<<"\n";
			cout << "depthBuffer[rightPixels[i] : "<<depthBuffer[rightPixels[i].x][rightPixels[i].y]<<"\n";
			*/

				if (leftPixels[i].zinv  > depthBuffer[leftPixels[i].x][leftPixels[i].y]
					&& rightPixels[i].zinv  > depthBuffer[rightPixels[i].x][rightPixels[i].y] )
				{
					depthBuffer[leftPixels[i].x][leftPixels[i].y] = leftPixels[i].zinv;
					depthBuffer[rightPixels[i].x][rightPixels[i].y] = rightPixels[i].zinv;
				}
			/*
			cout << "vecA["<<i<<"]:("<<vecA[i].x<<","<<vecA[i].y<<","<<vecA[i].zinv<<")\n";
			cout << "vecB["<<i<<"]:("<<vecB[i].x<<","<<vecB[i].y<<","<<vecB[i].zinv<<")\n";
			cout << "vecC["<<i<<"]:("<<vecC[i].x<<","<<vecC[i].y<<","<<vecC[i].zinv<<")\n";
			cout<<"leftPixels["<<i<<"]:("<<leftPixels[i].x<<","<<leftPixels[i].y
			 	<<","<<leftPixels[i].zinv<<")\n";
			cout<<"rightPixels["<<i<<"]:("<<rightPixels[i].x<<","<<rightPixels[i].y
			 	<<","<<rightPixels[i].zinv<<")\n";	
			*/
	 }

}

void DrawPolygon( const vector<vec3>& vertices )
{

	cout << " in function DrawPolygon _ 6 \n";
	int V = vertices.size();
	vector<Pixel> vertexPixels( V );
	for( int i=0; i<V; ++i ){
		VertexShader( vertices[i], vertexPixels[i] );
		//cout << "vertexPixels["<<i<<"]: ("<<vertexPixels[i].x<<","<<vertexPixels[i].y<<","<<vertexPixels[i].zinv<<")\n";
	}
	vector<Pixel> leftPixels;
	vector<Pixel> rightPixels;
	ComputePolygonRows( vertexPixels, leftPixels, rightPixels );
	DrawPolygonRows( leftPixels, rightPixels );
}

void DrawPolygonRows( vector<Pixel> leftPixels, vector<Pixel> rightPixels ){
	
	cout << "in function DrawPolygonRows _ 7\n";

	for (int i = 0; i <leftPixels.size(); i++){
		ivec2 left, right;
		left.x=leftPixels[i].x;left.y=leftPixels[i].y;
		right.x=rightPixels[i].x;right.y=rightPixels[i].y;
		//cout << "values before DrawLineSDL: \n";
		//cout <<"left("<<left.x<<','<<left.y<<")\n";
		//cout<<"right("<<right.x<<","<<right.y<<")\n";

		tmpZinvLeft = leftPixels[i].zinv;
		tmpZinvRight = rightPixels[i].zinv;
		//if (leftPixels[i].zinv >= depthBuffer[left.x][left.y] 
		//	&& rightPixels[i].zinv >= depthBuffer[right.x][right.y])
			DrawLineSDL(screen,left,right,currentColor);

	}

}

void DrawLineSDL( SDL_Surface* surface, ivec2 a, ivec2 b, vec3 color ){

	// The values of b are sometimes wrong !
	cout << "in function DrawLineSDL _ 3\n";
	//cout <<"Values of a : "<<a.x<<","<<a.y<<"\n";
	//cout <<"Values of b : "<<b.x<<","<<b.y<<"\n";
	
	ivec2 delta = glm::abs( a - b );
	int pixels = glm::max( delta.x, delta.y ) + 1;

	/*
	cout <<"delta ("<<delta.x<<","<<delta.y<<")\n";
	cout <<"pixels: "<<pixels<<"\n";
	*/
	/*Changing the ivec2 to Pixel ?*/
	/* where is that function called ?*/
	/* Apparently still called with ivec2 */

	vector<Pixel> line( pixels );
	Pixel pixelA,pixelB;
	// PROBLEM VALUES OF A AND B ARE WRONG, 
	// SO WE ARE TRYING TO REACH VALUES WHERE THE ARRAY IS NOT DEFINED
	//compute <<"Checking the depthBuffer:\n";
	cout <<"["<<a.x<<","<<a.y<<"]:"<<depthBuffer[a.x][a.y]<<"\n";
	cout <<"["<<b.x<<","<<b.y<<"]:"<<depthBuffer[b.x][b.y]<<"\n";

	pixelA.x=a.x;pixelA.y=a.y;
	pixelA.zinv=depthBuffer[a.x][a.y]; // focal_length;//
	pixelB.x=b.x;pixelB.y=b.y;
	pixelB.zinv=depthBuffer[b.x][b.y]; //focal_length;//
	cout <<"pixelA:("<<pixelA.x<<","<<pixelA.y<<","<<pixelA.zinv<<")\n";
	cout <<"pixelB:("<<pixelB.x<<","<<pixelB.y<<","<<pixelB.zinv<<")\n";
	Interpolate( pixelA, pixelB, line );

	for (int i=0; i<line.size();i++){
		// This seems like the good idea but something is weird
		if ( line[i].zinv >= depthBuffer[line[i].x][line[i].y] ){
			PutPixelSDL(surface, line[i].x,line[i].y,color);
			depthBuffer[line[i].x][line[i].y] = line[i].zinv;
		}

		cout << "line["<<i<<"].x:"<<line[i].x<<"\n"<<
		" line["<<i<<"].y:"<<line[i].y<<"\n";
	}

}