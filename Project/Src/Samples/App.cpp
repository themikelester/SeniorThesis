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

BaseApp *app = new App();

bool App::onKey(const uint key, const bool pressed){
	if (D3D10App::onKey(key, pressed)) return true;

	if (pressed){
		if (key == KEY_PAUSE){
			pause = !pause;
		}
	}

	return false;
}

void App::moveCamera(const float3 &dir){
	float3 newPos = camPos + dir * (speed * frameTime);

	camPos = clamp(newPos, float3(20.0f - SIZE_X, 20.0f - SIZE_Y, 20.0f - SIZE_Z), float3(SIZE_X - 20.0f, SIZE_Y - 20.0f, SIZE_Z - 20.0f));
}

void App::resetCamera(){
	camPos = vec3(325, 100, -765);
	wx = 0.12f;
	wy = 0.33f;
}

void App::onSize(const int w, const int h){
	D3D10App::onSize(w, h);

	if (renderer){
		// Make sure render targets are the size of the window
		renderer->resizeRenderTarget(baseRT,   w, h, 1, 1);
		renderer->resizeRenderTarget(normalRT, w, h, 1, 1);
		renderer->resizeRenderTarget(depthRT,  w, h, 1, 1);
	}
}

bool App::init(){
	initNoise();
	pause = false;

	// Init GUI components
	int tab = configDialog->addTab("Particles");
	configDialog->addWidget(tab, new Label(0, 0, 192, 36, "Wiggle"));
	configDialog->addWidget(tab, particleWiggle = new Slider(0, 40, 200, 24, -1, 6, 3));
	configDialog->addWidget(tab, new Label(0, 80, 192, 36, "Speed (X,Y,Z)"));
	configDialog->addWidget(tab, particleSpeedX = new Slider(0, 120, 200, 24, 0, 2000, 1000));
	configDialog->addWidget(tab, particleSpeedY = new Slider(0, 160, 200, 24, 0, 2000, 1200));
	configDialog->addWidget(tab, particleSpeedZ = new Slider(0, 200, 200, 24, 0, 2000, 1000));

	return true;
}

void App::exit(){
}

bool App::initAPI(){
	// Override the user's MSAA settings
	return D3D10App::initAPI(DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_UNKNOWN, 1, NO_SETTING_CHANGE);
}

void App::exitAPI(){
	D3D10App::exitAPI();
}

bool App::load(){
	// Shaders
	if ((fillBuffers = renderer->addShader("fillBuffers.shd")) == SHADER_NONE) return false;
	if ((ambient     = renderer->addShader("ambient.shd"    )) == SHADER_NONE) return false;
	if ((lighting    = renderer->addShader("lighting.shd"   )) == SHADER_NONE) return false;
	if ((particles   = renderer->addShader("particles.shd"  )) == SHADER_NONE) return false;
	if ((initParticles   = renderer->addShader("initParticles.shd"  )) == SHADER_NONE) return false;
	if ((particlePhysics = renderer->addShader("particlePhysics.shd")) == SHADER_NONE) return false;

	// Samplerstates
	if ((trilinearAniso = renderer->addSamplerState(TRILINEAR_ANISO, WRAP, WRAP, WRAP)) == SS_NONE) return false;
	if ((trilinearClamp = renderer->addSamplerState(TRILINEAR, CLAMP, CLAMP, CLAMP)) == SS_NONE) return false;
	if ((pointClamp = renderer->addSamplerState(NEAREST, CLAMP, CLAMP, CLAMP)) == SS_NONE) return false;

	// Main render targets
	if ((baseRT   = renderer->addRenderTarget(width, height, 1, 1, FORMAT_RGB10A2, 1, SS_NONE)) == TEXTURE_NONE) return false;
	if ((normalRT = renderer->addRenderTarget(width, height, 1, 1, FORMAT_RGB10A2, 1, SS_NONE)) == TEXTURE_NONE) return false;
	if ((depthRT  = renderer->addRenderDepth (width, height, 1,    FORMAT_DEPTH16, 1, SS_NONE, SAMPLE_DEPTH)) == TEXTURE_NONE) return false;

	// Particle system render targets. Two of each for ping-ponging.
	if ((posRT[0] = renderer->addRenderTarget(PARTICLES_RT_SIZE, PARTICLES_RT_SIZE, FORMAT_RGBA32F, SS_NONE)) == TEXTURE_NONE) return false;
	if ((posRT[1] = renderer->addRenderTarget(PARTICLES_RT_SIZE, PARTICLES_RT_SIZE, FORMAT_RGBA32F, SS_NONE)) == TEXTURE_NONE) return false;
	if ((dirRT[0] = renderer->addRenderTarget(PARTICLES_RT_SIZE, PARTICLES_RT_SIZE, FORMAT_RGBA32F, SS_NONE)) == TEXTURE_NONE) return false;
	if ((dirRT[1] = renderer->addRenderTarget(PARTICLES_RT_SIZE, PARTICLES_RT_SIZE, FORMAT_RGBA32F, SS_NONE)) == TEXTURE_NONE) return false;
	currRT = 0;
	prevRT = 1;
	needsInit = true;


	// Textures
	if ((base[0] = renderer->addTexture  ("../Textures/FieldStone.dds",                     true, trilinearAniso)) == SHADER_NONE) return false;
	if ((bump[0] = renderer->addNormalMap("../Textures/FieldStoneBump.dds",   FORMAT_RGBA8, true, trilinearAniso)) == SHADER_NONE) return false;
	if ((base[1] = renderer->addTexture  ("../Textures/stone08.dds",                        true, trilinearAniso)) == SHADER_NONE) return false;
	if ((bump[1] = renderer->addNormalMap("../Textures/stone08Bump.dds",      FORMAT_RGBA8, true, trilinearAniso)) == SHADER_NONE) return false;
	if ((base[2] = renderer->addTexture  ("../Textures/floor_wood_3.dds",                   true, trilinearAniso)) == SHADER_NONE) return false;
	if ((bump[2] = renderer->addNormalMap("../Textures/floor_wood_3Bump.dds", FORMAT_RGBA8, true, trilinearAniso)) == SHADER_NONE) return false;

	if ((particle = renderer->addTexture("../Textures/Particle.tga", true, trilinearClamp)) == SHADER_NONE) return false;
	if ((particleColors = renderer->addTexture("../Textures/SparkColors.dds", false, linearClamp)) == SHADER_NONE) return false;




	// Blendstates
	if ((blendAdd = renderer->addBlendState(ONE, ONE)) == BS_NONE) return false;

	// Input layout
	FormatDesc format[] = {
		{ 0, TYPE_VERTEX,   FORMAT_FLOAT, 3 },
		{ 0, TYPE_TANGENT,  FORMAT_FLOAT, 3 },
		{ 0, TYPE_BINORMAL, FORMAT_FLOAT, 3 },
		{ 0, TYPE_NORMAL,   FORMAT_FLOAT, 3 },
	};
	if ((vertexFormat = renderer->addVertexFormat(format, elementsOf(format), fillBuffers)) == VF_NONE) return false;

	// Scene vertex buffer
	const Vertex vertices[] = {
		// Floor
		{ float3(-SIZE_X, -SIZE_Y,  SIZE_Z), float3( 1,  0,  0), float3( 0,  0,  1), float3( 0,  1,  0) },
		{ float3( SIZE_X, -SIZE_Y,  SIZE_Z), float3( 1,  0,  0), float3( 0,  0,  1), float3( 0,  1,  0) },
		{ float3(-SIZE_X, -SIZE_Y, -SIZE_Z), float3( 1,  0,  0), float3( 0,  0,  1), float3( 0,  1,  0) },
		{ float3( SIZE_X, -SIZE_Y, -SIZE_Z), float3( 1,  0,  0), float3( 0,  0,  1), float3( 0,  1,  0) },

		// Ceiling
		{ float3( SIZE_X,  SIZE_Y,  SIZE_Z), float3(-1,  0,  0), float3( 0,  0,  1), float3( 0, -1,  0) },
		{ float3(-SIZE_X,  SIZE_Y,  SIZE_Z), float3(-1,  0,  0), float3( 0,  0,  1), float3( 0, -1,  0) },
		{ float3( SIZE_X,  SIZE_Y, -SIZE_Z), float3(-1,  0,  0), float3( 0,  0,  1), float3( 0, -1,  0) },
		{ float3(-SIZE_X,  SIZE_Y, -SIZE_Z), float3(-1,  0,  0), float3( 0,  0,  1), float3( 0, -1,  0) },

		// Walls
		{ float3(-SIZE_X,  SIZE_Y,  SIZE_Z), float3( 1,  0,  0), float3( 0, -1,  0), float3( 0,  0, -1) },
		{ float3( SIZE_X,  SIZE_Y,  SIZE_Z), float3( 1,  0,  0), float3( 0, -1,  0), float3( 0,  0, -1) },
		{ float3(-SIZE_X, -SIZE_Y,  SIZE_Z), float3( 1,  0,  0), float3( 0, -1,  0), float3( 0,  0, -1) },
		{ float3( SIZE_X, -SIZE_Y,  SIZE_Z), float3( 1,  0,  0), float3( 0, -1,  0), float3( 0,  0, -1) },

		{ float3( SIZE_X,  SIZE_Y,  SIZE_Z), float3( 0,  0, -1), float3( 0, -1,  0), float3(-1,  0,  0) },
		{ float3( SIZE_X,  SIZE_Y, -SIZE_Z), float3( 0,  0, -1), float3( 0, -1,  0), float3(-1,  0,  0) },
		{ float3( SIZE_X, -SIZE_Y,  SIZE_Z), float3( 0,  0, -1), float3( 0, -1,  0), float3(-1,  0,  0) },
		{ float3( SIZE_X, -SIZE_Y, -SIZE_Z), float3( 0,  0, -1), float3( 0, -1,  0), float3(-1,  0,  0) },

		{ float3( SIZE_X,  SIZE_Y, -SIZE_Z), float3(-1,  0,  0), float3( 0, -1,  0), float3( 0,  0,  1) },
		{ float3(-SIZE_X,  SIZE_Y, -SIZE_Z), float3(-1,  0,  0), float3( 0, -1,  0), float3( 0,  0,  1) },
		{ float3( SIZE_X, -SIZE_Y, -SIZE_Z), float3(-1,  0,  0), float3( 0, -1,  0), float3( 0,  0,  1) },
		{ float3(-SIZE_X, -SIZE_Y, -SIZE_Z), float3(-1,  0,  0), float3( 0, -1,  0), float3( 0,  0,  1) },

		{ float3(-SIZE_X,  SIZE_Y, -SIZE_Z), float3( 0,  0,  1), float3( 0, -1,  0), float3( 1,  0,  0) },
		{ float3(-SIZE_X,  SIZE_Y,  SIZE_Z), float3( 0,  0,  1), float3( 0, -1,  0), float3( 1,  0,  0) },
		{ float3(-SIZE_X, -SIZE_Y, -SIZE_Z), float3( 0,  0,  1), float3( 0, -1,  0), float3( 1,  0,  0) },
		{ float3(-SIZE_X, -SIZE_Y,  SIZE_Z), float3( 0,  0,  1), float3( 0, -1,  0), float3( 1,  0,  0) },
	};
	if ((vertexBuffer = renderer->addVertexBuffer(sizeof(vertices), STATIC, vertices)) == VB_NONE) return false;

	// Scene index buffer
	const ushort indices[] = {
		0,  1,  2,  2,  1,  3,
		4,  5,  6,  6,  5,  7,
		8,  9,  10, 10, 9,  11,
		12, 13, 14, 14, 13, 15,
		16, 17, 18, 18, 17, 19,
		20, 21, 22, 22, 21, 23,
	};
	if ((indexBuffer = renderer->addIndexBuffer(elementsOf(indices), sizeof(ushort), STATIC, indices)) == IB_NONE) return false;

	return true;
}

void App::unload(){

}

void App::drawFrame(){
	const float particleSize = 8;
	const float fov = 1.5f;

	// Compute scene matrices
	float4x4 projection = toD3DProjection(perspectiveMatrixX(fov, width, height, 12, 2500));
	float4x4 modelview = rotateXY(-wx, -wy);
	modelview.translate(-camPos);
	float4x4 mvp = projection * modelview;
	// Pre-scale-bias the matrix so I can use the texCoord directly in [0..1] range instead of [-1..1] clip-space range
	float4x4 invMvp = (!mvp) * (translate(-1.0f, 1.0f, 0.0f) * scale(2.0f, -2.0f, 1.0f));


	if (!pause){
		TextureID particleRTs[] = { posRT[currRT], dirRT[currRT] };
		renderer->changeRenderTargets(particleRTs, elementsOf(particleRTs), TEXTURE_NONE);
		/*
			Particle physics pass
		*/
		renderer->reset();
		renderer->setRasterizerState(cullNone);
		if (needsInit){
			// Initialize particles
			renderer->setShader(initParticles);
			renderer->setShaderConstant2f("timeFactors", float2(1.0f, 1.0f / PARTICLES_RT_SIZE) * 4.0f);
			renderer->setShaderConstant1f("offset", 0.5f / PARTICLES_RT_SIZE);
			needsInit = false;
		} else {
			// Do particle physics using previous frame's render targets as input
			renderer->setShader(particlePhysics);
			renderer->setTexture("pos", posRT[prevRT]);
			renderer->setTexture("dir", dirRT[prevRT]);
			
			// Compute a direction to sprinkle particles
			static float t = 0;
			float ft = min(frameTime, 0.05f);
			t += ft * powf(2.0f, particleWiggle->getValue());
			float3 sprinkleDir = float3(
				particleSpeedX->getValue() *  noise1(1.3f * t), 
				particleSpeedY->getValue() * (noise1(t + 21.5823f) * 0.5f + 0.5f),
				particleSpeedZ->getValue() *  noise1(-t)
			);

			renderer->setShaderConstant3f("sprinkleDir", sprinkleDir);
			renderer->setShaderConstant1f("frameTime", ft);
			renderer->setShaderConstant3f("box", float3(SIZE_X, SIZE_Y, SIZE_Z) - particleSize);
		}
		renderer->setDepthState(noDepthTest);
		renderer->apply();

		device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		device->Draw(3, 0);
	}

	TextureID bufferRTs[] = { baseRT, normalRT };
	renderer->changeRenderTargets(bufferRTs, elementsOf(bufferRTs), depthRT);
		renderer->clear(false, true, NULL, 1.0f);

		/*
			Main scene pass.
			This is where the buffers are filled for the later deferred passes.
		*/
		renderer->reset();
		renderer->setRasterizerState(cullBack);
		renderer->setShader(fillBuffers);
		renderer->setShaderConstant4x4f("mvp", mvp);
		renderer->setShaderConstant3f("camPos", camPos);
		renderer->setSamplerState("baseFilter", trilinearAniso);
		renderer->setVertexFormat(vertexFormat);
		renderer->setVertexBuffer(0, vertexBuffer);
		renderer->setIndexBuffer(indexBuffer);
		renderer->apply();

		device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Floor
		renderer->setTexture("Base", base[0]);
		renderer->setTexture("Bump", bump[0]);
		renderer->applyTextures();

		device->DrawIndexed(6, 0, 0);

		// Ceiling
		renderer->setTexture("Base", base[1]);
		renderer->setTexture("Bump", bump[1]);
		renderer->applyTextures();

		device->DrawIndexed(6, 6, 0);

		// Walls
		renderer->setTexture("Base", base[2]);
		renderer->setTexture("Bump", bump[2]);
		renderer->applyTextures();

		device->DrawIndexed(24, 12, 0);

	renderer->changeToMainFramebuffer();


	/*
		Deferred ambient pass
	*/
	renderer->reset();
	renderer->setRasterizerState(cullNone);
	renderer->setShader(ambient);
	renderer->setShaderConstant4x4f("invMvp", invMvp);
	//renderer->setShaderConstant3f("camPos", camPos);
	renderer->setTexture("base", baseRT);
	renderer->setTexture("normal", normalRT);
	renderer->setTexture("depth", depthRT);
	renderer->setSamplerState("filter", pointClamp);
	renderer->setDepthState(noDepthTest);
	renderer->apply();

	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	device->Draw(3, 0);


	/*
		Deferred lighting pass
	*/
	renderer->reset();
	renderer->setRasterizerState(cullNone);
	renderer->setShader(lighting);
	renderer->setShaderConstant4x4f("modelview", modelview);
	renderer->setShaderConstant4x4f("invMvp", invMvp);
	//renderer->setShaderConstant3f("camPos", camPos);
	float ex = tanf(fov * 0.5f);
	float ey = ex * height / width;
	renderer->setShaderConstant1f("ex", 0.5f / ex);
	renderer->setShaderConstant1f("ey", 0.5f / ey);
	renderer->setShaderConstant2f("zw", projection.rows[2].zw());
	renderer->setShaderConstant1i("mask", PARTICLES_RT_SIZE - 1);
	renderer->setShaderConstant1i("shift", PARTICLES_RT_SIZE_SHIFT);
	renderer->setTexture("pos", posRT[currRT]);
	renderer->setTexture("base", baseRT);
	renderer->setTexture("normal", normalRT);
	renderer->setTexture("depth", depthRT);
	renderer->setSamplerState("filter", pointClamp);
	renderer->setTexture("colors", particleColors);
	renderer->setSamplerState("colorsFilter", linearClamp);
	renderer->setDepthState(noDepthTest);
	renderer->setBlendState(blendAdd);
	renderer->apply();

	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
	device->Draw(PARTICLES_RT_SIZE * PARTICLES_RT_SIZE, 0);


	/*
		Particles pass
	*/
	renderer->reset();
	renderer->setRasterizerState(cullNone);
	renderer->setShader(particles);
	renderer->setShaderConstant4x4f("mvp", mvp);
	renderer->setShaderConstant3f("dx", modelview.rows[0].xyz() * particleSize);
	renderer->setShaderConstant3f("dy", modelview.rows[1].xyz() * particleSize);
	renderer->setShaderConstant1i("mask", PARTICLES_RT_SIZE - 1);
	renderer->setShaderConstant1i("shift", PARTICLES_RT_SIZE_SHIFT);
	renderer->setTexture("pos", posRT[currRT]);
	renderer->setTexture("tex", particle);
	renderer->setTexture("colors", particleColors);
	renderer->setSamplerState("particleFilter", trilinearClamp);
	renderer->setSamplerState("colorsFilter", linearClamp);
	renderer->setDepthState(noDepthTest);
	renderer->setBlendState(blendAdd);
	renderer->apply();

	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
	device->Draw(PARTICLES_RT_SIZE * PARTICLES_RT_SIZE, 0);


	if (!pause){
		currRT = prevRT;
		prevRT = 1 - currRT;
	}
}
