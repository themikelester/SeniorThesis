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

#include "App.h"
#include "../Framework3/Util/Model.h"

#define SHADER_DIR(filename) "./Shaders/"filename
#define NUM_GRAFTAL_BUFS (sizeof(shapeBuffers)/sizeof(shapeBuffers[0]))
#define INITIAL_GRAFTAL_SIZE 20

BaseApp *app = new App();
const uint nLODLevels = 2;

void App::resetCamera(){
	camPos = vec3(0.0, 0.0, 100.0f);
	wx = 0.0;
	wy = 3.14;
}

bool App::init(){
	speed = 100;
	graftalSize = INITIAL_GRAFTAL_SIZE;

	return true;
}

void App::exit(){
}

bool App::onMouseWheel(const int x, const int y, const int scroll){
	//Increase (or Decrease) Graftal size with scroll wheel
	graftalSize += scroll;
	return BaseApp::onMouseWheel(x, y, scroll);
}

bool App::load(){

	// Input layouts
	FormatDesc modelVertexFormat[] = {
		{ 0, TYPE_VERTEX,   FORMAT_FLOAT, CLASS_PER_VERTEX, 3 },	// Vertex position
		{ 0, TYPE_NORMAL,   FORMAT_FLOAT, CLASS_PER_VERTEX, 3 },	// Vertex normal
	};

	// Vertex format for graftals after creation and before rendering
	// This vertex format, the StreamOut format, and the Struct format in the shaders MUST MATCH
	FormatDesc graftalVertexFormat[] = {
		{ 0, TYPE_VERTEX,   FORMAT_FLOAT, CLASS_PER_VERTEX, 3 },	// Graftal position
		{ 0, TYPE_NORMAL,   FORMAT_FLOAT, CLASS_PER_VERTEX, 3 },	// Graftal normal
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, CLASS_PER_VERTEX, 1 },	// Graftal size
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, CLASS_PER_VERTEX, 1 },	// Graftal visibility
	};

	FormatDesc renderVertexFormat[] = {
		{ 0, TYPE_VERTEX,   FORMAT_FLOAT, CLASS_PER_INSTANCE, 3 },	// Graftal position
		{ 0, TYPE_NORMAL,   FORMAT_FLOAT, CLASS_PER_INSTANCE, 3 },	// Graftal normal
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, CLASS_PER_INSTANCE, 1 },	// Graftal size
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, CLASS_PER_INSTANCE, 1 },	// Graftal visiblity
		{ 1, TYPE_VERTEX,   FORMAT_FLOAT, CLASS_PER_VERTEX, 3 },	// Geometry position
	};

	// StreamOut format
	StreamDesc streamOut[] = {
		//type, typeIndex, startComponent, componentCount, stream
		{TYPE_VERTEX, 0, 0, 3, 0},		// Graftal position
		{TYPE_NORMAL, 0, 0, 3, 0},		// Graftal normal
		{TYPE_TEXCOORD, 0, 0, 1, 0},	// Graftal size
		{TYPE_TEXCOORD, 1, 0, 1, 0},	// Graftal visibility
	};

	// Shader struct format
	char* graftalDefStr = "struct GraftalFormat {\n\\
								float3 pos: Position0;\n\\
								float3 norm: Normal0;\n\\
								float size: Texcoord0;\\
								float P:	Texcoord1;\\
							};\n";


	char* extra = (char*)malloc(1024);
	memcpy(extra, graftalDefStr, strlen(graftalDefStr));

	const char* LODLevels[nLODLevels] = {"#define LOD1\n", "#define LOD2\n"};
	LODShaders = new ShaderID[nLODLevels];
	for (int i = 0; i < nLODLevels; i++) {
		memcpy(extra+strlen(graftalDefStr), LODLevels[i], strlen(LODLevels[i])+1);
		if ((LODShaders[i] = renderer->addShader(SHADER_DIR("LOD.shd"), extra, streamOut, elementsOf(streamOut), 0)) == SHADER_NONE) return false;
	}

	if ((cullShader = renderer->addShader(SHADER_DIR("cull.shd"), NULL, 0, graftalDefStr, streamOut, elementsOf(streamOut), 0)) == SHADER_NONE) return false;
	if ((initialShader = renderer->addShader(SHADER_DIR("initial.shd"), NULL, 0, graftalDefStr, streamOut, elementsOf(streamOut), 0)) == SHADER_NONE) return false;
	if ((posVisShader = renderer->addShader(SHADER_DIR("PosVis.shd"), graftalDefStr)) == SHADER_NONE) return false;
	if ((splineShader = renderer->addShader(SHADER_DIR("spline.shd"))) == SHADER_NONE) return false;
	if ((renderShader = renderer->addShader(SHADER_DIR("FSQuad.shd"))) == SHADER_NONE) return false;
	if ((basicShader = renderer->addShader(SHADER_DIR("basic.shd"))) == SHADER_NONE) return false;
	if ((terrainShader = renderer->addShader(SHADER_DIR("terrain.shd"))) == SHADER_NONE) return false;
	if ((graftalShader = renderer->addShader(SHADER_DIR("graftal.shd"), graftalDefStr)) == SHADER_NONE) return false;
	if ((graftalDbgShader = renderer->addShader(SHADER_DIR("graftalDebug.shd"), graftalDefStr)) == SHADER_NONE) return false;

	// Render targets
	if ((colorRT   = renderer->addRenderTarget(width, height, 1, 1, 1, FORMAT_RGB10A2, 1, SS_NONE)) == TEXTURE_NONE) return false;
	if ((decalRT   = renderer->addRenderTarget(width, height, 1, 1, 1, FORMAT_R8, 1, SS_NONE)) == TEXTURE_NONE) return false;
	if ((depthRT   = renderer->addRenderDepth (width, height, 1,       FORMAT_D16,     1, SS_NONE, SAMPLE_DEPTH)) == TEXTURE_NONE) return false;
	if ((stencilRT = renderer->addRenderDepth (width, height, 1,       FORMAT_D24S8,   1,           SS_NONE)) == TEXTURE_NONE) return false;

	// Samplerstates
	if ((trilinearClamp = renderer->addSamplerState(TRILINEAR, CLAMP, CLAMP, CLAMP)) == SS_NONE) return false;
	if ((trilinearAniso = renderer->addSamplerState(TRILINEAR_ANISO, WRAP, WRAP, WRAP)) == SS_NONE) return false;

	// Depthstates
	if ((stencilRoute = renderer->addDepthState(true, true, LEQUAL, true, 0xFF, NOTEQUAL, KEEP, KEEP, KEEP)) == BS_NONE) return false;
	if ((stencilSet   = renderer->addDepthState(true, true, LEQUAL, true, 0xFF, ALWAYS, REPLACE, REPLACE, REPLACE)) == BS_NONE) return false;

	// Textures
	if ((terrain = renderer->addTexture("Terrain.dds", false, linearClamp)) == TEXTURE_NONE) return false;
	
	// Model
	model = new Model();
	//model->createSphere(6);
	model->loadObj("../models/bunny3.obj");
	model->computeNormals();
	if (!model->makeDrawable(renderer, true, basicShader)) return false;
	nGraftals = model->getVertexCount();

	// Blendstates
	if ((blendAdd = renderer->addBlendState(ONE, ONE)) == BS_NONE) return false;
	if ((blendMin = renderer->addBlendState(SRC_COLOR, DST_COLOR, ONE, ONE, BM_MIN, BM_MIN)) == BS_NONE) return false;

	// Rasterizerstates
	if ((cullNone = renderer->addRasterizerState(CULL_NONE, SOLID, false, false)) == BS_NONE) return false;
	if ((cullFront = renderer->addRasterizerState(CULL_FRONT, SOLID, false, false)) == BS_NONE) return false;

	// Vertex Formats
	if ((modelVF = renderer->addVertexFormat(modelVertexFormat, elementsOf(modelVertexFormat), basicShader)) == VF_NONE) return false;
	if ((cullVF = renderer->addVertexFormat(graftalVertexFormat, elementsOf(graftalVertexFormat), cullShader)) == VF_NONE) return false;
	if ((renderVF = renderer->addVertexFormat(renderVertexFormat, elementsOf(renderVertexFormat), graftalShader)) == VF_NONE) return false;
	
	// Buffers
	/* First, add the buffer containing graftal geometry information (shape)
	   These vertices are declared as "PER_VERTEX", so they will always vary
	*/
	const Vertex graftalGeometry[] = {
		{float3(0, -0.001, 7)},
		{float3(0, 0, 7)},		
		{float3(3, 10, 5)},
		{float3(10, 18, 2)},
		{float3(15, 22, 0.0)},
		{float3(0)},
	};
	nVerts = sizeof(graftalGeometry)/sizeof(graftalGeometry[0]);
	if ((shapeBuffer = renderer->addVertexBuffer(sizeof(graftalGeometry), STATIC, graftalGeometry)) == VB_NONE) return false;
	
	uint graftalSize = renderer->getVertexFormatSize(cullVF);
	// Create a buffer to hold the graftals that pass the culling stage
	if ((cullBuffer = renderer->addVertexBuffer(model->getVertexCount()*graftalSize, DEFAULT, NULL, D3D10_BIND_STREAM_OUTPUT)) == VB_NONE) return false;

	// Create buffers to hold graftals for each LOD stage
	uint nLODs = 2;
	LODBuffers = new VertexBufferID[nLODs];
	for (int i = 0; i < nLODs; i++) {
		if ((LODBuffers[i] = renderer->addVertexBuffer(model->getVertexCount()*graftalSize, DEFAULT, NULL, D3D10_BIND_STREAM_OUTPUT)) == VB_NONE) return false;
	}

	/* Next, add the buffer containing the per-graftal data like position and normal
	   These vertices are declared as "PER_INSTANCE", so they will remain constant while each graftal is drawn
	   NOTE: For now, we'll just use the vertex positions and normals from the model
	*/
	FormatDesc* modelVF = model->getFormatDesc();
	modelBuffer = model->getVertexBuffer();
	graftalBuffer = generateInitialGraftals();

	return true;
}

VertexBufferID App::generateInitialGraftals() {
	VertexBufferID inputBuffer; 
	VertexBufferID outputBuffer; 
	
	// For now, let's let the initial graftal positions be the vertices of our model
	inputBuffer = model->getVertexBuffer();

	// Create the buffer that will hold our graftals
	uint nGraftals = model->getVertexCount();
	uint graftalSize = renderer->getVertexFormatSize(cullVF);
	if ((outputBuffer = renderer->addVertexBuffer(nGraftals * graftalSize, DEFAULT, NULL, D3D10_BIND_STREAM_OUTPUT)) == VB_NONE) return false;


	renderer->reset();
	renderer->setRasterizerState(cullNone);
	renderer->setDepthState(noDepthTest);
	renderer->setShader(initialShader);
	renderer->apply();
		
	uint offset[] = {0};

	renderer->changeVertexFormat(modelVF);
	renderer->changeVertexBuffers(0, 1, &inputBuffer, 0);
    device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_POINTLIST );
	
    // Point to the correct output buffer, and draw
	renderer->changeStreamTargets(1, &outputBuffer);
	device->Draw(nGraftals, 0);
	renderer->changeStreamTargets(0, NULL);

	return outputBuffer;
};

void App::unload(){
}

void App::visualizeGraftalPositions() {
	float4x4 projection = toD3DProjection(perspectiveMatrixX(1.5f, width, height, 0.1f, 500));
	float4x4 modelview = rotateXY(-wx, -wy);
	float4x4 invMvpEnv = !(projection * modelview);
	modelview.translate(-camPos);

	float4x4 mvp = projection * modelview;

	renderer->reset();
		renderer->setRasterizerState(cullNone);
		renderer->setShader(posVisShader);
		renderer->setShaderConstant4x4f("mvp", mvp);
		renderer->setShaderConstant1f("scale", 50);
	renderer->apply();
		
	uint offset[] = {0};

	renderer->changeVertexFormat(cullVF);
	renderer->changeVertexBuffers(0, 1, &graftalBuffer, 0);
    device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_POINTLIST );
	
	device->Draw(nGraftals, 0);
}

void App::drawFrame(){
	
	float clearColor[] = {0,0,0,0};
	VertexBufferID pBuffers[2];

	/*
		Terrain
	*/
	/*renderer->reset();
	renderer->setRasterizerState(cullBack);
	renderer->setShader(terrainShader);
	renderer->setShaderConstant4x4f("mvp", mvp);
	renderer->setTexture("terrain", terrain);
	renderer->setSamplerState("filter", trilinearAniso);
	renderer->apply();

	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);*/
	//device->DrawInstanced(256, 255, 0, 0);

	/*visualizeGraftalPositions();
	return;*/

	/*
		Cull Graftals
	*/
	renderer->reset();
	renderer->setShader(cullShader);
	renderer->setShaderConstant1f("scale", 50);
	renderer->setShaderConstant3f("eyePos", camPos);
	renderer->apply();
		
	uint offset[] = {0};

	renderer->changeVertexFormat(cullVF);
	renderer->changeVertexBuffers(0, 1, &graftalBuffer, 0);
    device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_POINTLIST );
	
    // Point to the correct output buffer, and draw
	renderer->changeStreamTargets(1, &cullBuffer);
	device->Draw(nGraftals, 0);
	renderer->changeStreamTargets(0, NULL);

	/*
		Sort Graftals into LOD buffers
	*/
	//Setup a Query to the GPU to find out how many graftals are in each LOD level
	ID3D10Query *LODQuery[nLODLevels];
	D3D10_QUERY_DESC d3dQueryDesc;
	d3dQueryDesc.Query = D3D10_QUERY_SO_STATISTICS;
	d3dQueryDesc.MiscFlags = 0;

	for (int i = 0; i < nLODLevels; i++) {
		device->CreateQuery(&d3dQueryDesc, &LODQuery[i]);

		renderer->reset();
			renderer->setShader(LODShaders[i]);
		renderer->apply();

		renderer->changeVertexFormat(cullVF);
		renderer->changeVertexBuffer(0, cullBuffer);
		device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_POINTLIST );
	
		// Point to the correct output buffer, and draw
		pBuffers[0] = LODBuffers[i];
		renderer->changeStreamTargets(1, pBuffers);

		LODQuery[i]->Begin();
			device->DrawAuto();
		LODQuery[i]->End();

		renderer->changeStreamTargets(0, NULL);
	}
	
	
	/* CPU STALL while waiting on the results of our query, so do some CPU stuff here
	*/
	float4x4 projection = toD3DProjection(perspectiveMatrixX(1.5f, width, height, 0.1f, 500));
	float4x4 modelview = rotateXY(-wx, -wy);
	float4x4 invMvpEnv = !(projection * modelview);
	modelview.translate(-camPos);

	float4x4 mvp = projection * modelview;
	float3 lightDir = vec3(1.0, 1.0, 1.0);
	
	TextureID bufferRTs[] = { colorRT, decalRT};
	renderer->changeRenderTargets(bufferRTs, elementsOf(bufferRTs), stencilRT);
	renderer->clear(true, true, true, clearColor, 1.0f);

	/* 
		Draw Models(s)
	*/
	renderer->reset();
		renderer->setRasterizerState(cullNone);
		renderer->setDepthState(stencilSet, 1);
		renderer->setShader(basicShader);
		renderer->setShaderConstant4x4f("mvp", mvp);
		renderer->setShaderConstant1f("scale", 50);
	renderer->apply();
	
	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	renderer->changeVertexFormat(modelVF);
	renderer->changeVertexBuffers(0, 1, &modelBuffer, 0);
	renderer->changeIndexBuffer(model->getIndexBuffer());
	device->DrawIndexed(model->getIndexCount(), 0, 0);
	
	// Get the data from our asynchronous query
	UINT64 nLODGraftals[nLODLevels];
	for (int i = 0; i < nLODLevels; i++) {
		D3D10_QUERY_DATA_SO_STATISTICS queryData;
		while(LODQuery[i]->GetData(&queryData, sizeof(D3D10_QUERY_DATA_SO_STATISTICS), 0) == S_FALSE);
		nLODGraftals[i] = queryData.NumPrimitivesWritten;
	}

	/*
		Draw Graftals
	*/
	for (int i = 0; i < nLODLevels; i++) {
		renderer->reset();
			renderer->setRasterizerState(cullFront);
			//if(i==0) renderer->setDepthState(stencilRoute, 1);
			if(i==1) renderer->setBlendState(blendSrcAlpha);
			renderer->setShader(i == 0 ? graftalShader : graftalDbgShader);
			renderer->setShaderConstant4x4f("mvp", mvp);
			renderer->setShaderConstant3f("eyePos", camPos);
			renderer->setShaderConstant1f("graftalScale", graftalSize * 0.01);
			renderer->setShaderConstant3f("lineColor", float3(0xFF/255.0, 0x78/255.0, 0));
			renderer->setShaderConstant3f("solidColor", float3(0xFF/255.0, 0x78/255.0, 0));
		renderer->apply();

		intptr offsets[] = {0, 0};
		
		pBuffers[0] = LODBuffers[i];
		pBuffers[1] = shapeBuffer;
	
		device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ);
		renderer->changeVertexFormat(renderVF);
		renderer->changeVertexBuffers(0, 2, pBuffers, offsets);
		device->DrawInstanced(nVerts, nLODGraftals[i], 0, 0);
	}
	
	
	renderer->changeToMainFramebuffer();
	
	//Test the FullScreen Quad shader
	renderer->reset();
		renderer->setRasterizerState(cullNone);
		renderer->setDepthState(noDepthTest);
		renderer->setShader(renderShader);
		renderer->setTexture("color", colorRT);
		renderer->setTexture("decal", decalRT);
		renderer->setSamplerState("filter", linearClamp);
		renderer->setShaderConstant3f("edgeColor", float3(0,0,0));
	renderer->apply();
    device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	device->Draw(4, 0);

	return;
}
