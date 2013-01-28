/***********      .---.         .-"-.      *******************\
* -------- *     /   ._.       / ´ ` \     * ---------------- *
* Author's *     \_  (__\      \_°v°_/     * humus@rogers.com *
*   note   *     //   \\       //   \\     * ICQ #47010716    *
* -------- *    ((     ))     ((     ))    * ---------------- *
*          ****--""---""-------""---""--****                  ********\
* This file is a part of the work done by Humus. You are free to use  *
* the code in any way you like, modified, unmodified or copy'n'pasted *
* into your own work. However, I expect you to respect these points:  *
*  @ If you use this file and its contents unmodified, or use a major *
*    part of this file, please credit the author and leave this note. *
*  @ For use in anything commercial, please request my approval.      *
*  @ Share your work and ideas too as much as you can.                *
\*********************************************************************/

#include "../Framework3/Direct3D10/D3D10App.h"
#include "../Framework3/Util/Model.h"

struct Vertex {
	float3 position;
};

class App : public D3D10App {
public:
	char *getTitle() const { return "Inferno"; }

	void resetCamera();

	bool init();
	void exit();

	bool load();
	void unload();

	void drawFrame();
	
	bool onMouseWheel(const int x, const int y, const int scroll);

	VertexBufferID generateInitialGraftals();
	void visualizeGraftalPositions();

protected:
	ShaderID graftalShader, initialShader, basicShader, graftalDbgShader, cullShader, 
				splineShader, renderShader, posVisShader, terrainShader;
	ShaderID *LODShaders;

	Model* model;
	
	uint nGraftals;
	uint nVerts;
	uint graftalSize;
	
	TextureID colorRT;
	TextureID decalRT;
	TextureID depthRT;
	TextureID stencilRT;

	TextureID terrain;

	VertexFormatID modelVF;
	VertexFormatID cullVF; 
	VertexFormatID renderVF; 

	VertexBufferID modelBuffer;
	VertexBufferID graftalBuffer;
	VertexBufferID shapeBuffer;
	VertexBufferID cullBuffer;
	VertexBufferID *LODBuffers;

	DepthStateID stencilSet;
	DepthStateID stencilRoute;
	RasterizerStateID noMSAA;
	SamplerStateID trilinearClamp, trilinearAniso;
	BlendStateID blendAdd;
	BlendStateID blendMin;

	ID3D10Buffer *SOBuffer;
};
