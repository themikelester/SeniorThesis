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
#include "../Framework3/Math/Noise.h"

#define SIZE_X 512
#define SIZE_Y 256
#define SIZE_Z 1024

#define PARTICLES_RT_SIZE_SHIFT 5
#define PARTICLES_RT_SIZE (1 << PARTICLES_RT_SIZE_SHIFT)

struct Vertex {
	float3 position;
	float3 tangent;
	float3 binormal;
	float3 normal;
};

class App : public D3D10App {
public:
	char *getTitle() const { return "Deferred shading"; }

	bool onKey(const uint key, const bool pressed);

	void moveCamera(const float3 &dir);
	void resetCamera();

	void onSize(const int w, const int h);

	bool init();
	void exit();

	bool initAPI();
	void exitAPI();

	bool load();
	void unload();

	void drawFrame();

protected:
	ShaderID fillBuffers, ambient, lighting, particles, initParticles, particlePhysics;
	TextureID base[3], bump[3];

	TextureID baseRT, normalRT, depthRT;
	TextureID posRT[2], dirRT[2];
	int currRT, prevRT;

	TextureID particle, particleColors;

	SamplerStateID trilinearAniso, trilinearClamp, pointClamp;
	BlendStateID blendAdd;

	VertexFormatID vertexFormat;
	VertexBufferID vertexBuffer;
	IndexBufferID indexBuffer;

	Slider *particleWiggle;
	Slider *particleSpeedX;
	Slider *particleSpeedY;
	Slider *particleSpeedZ;
	bool needsInit, pause;
};
