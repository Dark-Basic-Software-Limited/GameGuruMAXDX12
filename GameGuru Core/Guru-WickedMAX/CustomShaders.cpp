#include <string>
// Force update
#include "../GameGuru/Include/Utility/stb_image.h" // Fixed include path
#include "CFileC.h"
#include "CStr.h"
#include "WickedEngine.h"
#include "wiGraphicsDevice.h"
#include "wiScene.h"
#include "wiRenderer.h"
#include "wiProfiler.h"
#include "wiECS.h"
//#include "Utility/tinyddsloader.h" DX12

#include "master.h"

#include "..\GameGuru\Imgui\imgui.h"

// redefines MAX_PATH to 1050
#include "preprocessor-moreflags.h"

#ifdef OPTICK_ENABLE
#include "optick.h"
#endif

// Externl for debugging
extern void timestampactivity(int i, char* desc_s);

#include "GGThread.h"
using namespace GGThread;

#include "DirectXTex.h"
#include "..\..\..\..\GameGuru\Imgui\imgui_gg_dx11.h"
#include "M-UndoSys-Terrain.h"
#include <wiRenderer.h>


using namespace wiGraphics;
using namespace wiScene;
using namespace wiRenderer;

Shader shaderCustom1PS;


enum OBJECTRENDERING_DOUBLESIDED
{
	OBJECTRENDERING_DOUBLESIDED_DISABLED,
	OBJECTRENDERING_DOUBLESIDED_ENABLED,
	OBJECTRENDERING_DOUBLESIDED_BACKSIDE,
	OBJECTRENDERING_DOUBLESIDED_COUNT
};
enum OBJECTRENDERING_TESSELLATION
{
	OBJECTRENDERING_TESSELLATION_DISABLED,
	OBJECTRENDERING_TESSELLATION_ENABLED,
	OBJECTRENDERING_TESSELLATION_COUNT
};
enum OBJECTRENDERING_ALPHATEST
{
	OBJECTRENDERING_ALPHATEST_DISABLED,
	OBJECTRENDERING_ALPHATEST_ENABLED,
	OBJECTRENDERING_ALPHATEST_COUNT
};

PipelineState PSO_custom_object
[MaterialComponent::SHADERTYPE_COUNT]
[RENDERPASS_COUNT]
[BLENDMODE_COUNT]
[OBJECTRENDERING_DOUBLESIDED_COUNT]
[OBJECTRENDERING_TESSELLATION_COUNT]
[OBJECTRENDERING_ALPHATEST_COUNT];

Shader shaderMainTreeAnimateVS;
Shader shaderPrepassTreeAnimateVS;
Shader shaderShadowTreeAnimateVS;
Shader shaderWaterPS;
Shader shaderGlassPS;
Shader shaderGridPS;
Shader damageBloodPS;
Shader damageBloodVS;

void AddCustomShaders(void)
{

	//PE: Tree animated.
	CustomShader customShader;
	if(!LoadShader(ShaderStage::VS, shaderMainTreeAnimateVS, "objectVS_common_tree.cso"))
		; // customShader.bActive removed - bActive no longer exists in CustomShader
	if(!LoadShader(ShaderStage::VS, shaderPrepassTreeAnimateVS, "objectVS_prepass_trees.cso"))
		; // customShader.bActive removed - bActive no longer exists in CustomShader
	if(!LoadShader(ShaderStage::VS, shaderShadowTreeAnimateVS, "shadowVS_alphatest_tree.cso"))
		; // customShader.bActive removed - bActive no longer exists in CustomShader

	PipelineStateDesc desc[RENDERPASS_COUNT];
	PipelineState pso[RENDERPASS_COUNT];
	for (int i = 0; i < RENDERPASS_COUNT; i++)
	{
		//wiRenderer::AddPipelineDesc(desc[i], i, PSTYPE_OBJECT_PERMUTATION_BEGIN, MaterialComponent::SHADERTYPE::SHADERTYPE_PBR, BLENDMODE_OPAQUE, OBJECTRENDERING_DOUBLESIDED_ENABLED, false, true, false); // AddPipelineDesc removed from new API
		{
			if (i == RENDERPASS_MAIN)
			{
				desc[i].vs = &shaderMainTreeAnimateVS;
			}
			if (i == RENDERPASS_SHADOW)
			{
				desc[i].vs = &shaderShadowTreeAnimateVS;
			}
			if (i == RENDERPASS_PREPASS)
			{
				desc[i].vs = &shaderPrepassTreeAnimateVS;
			}
		}
		wiGraphics::GetDevice()->CreatePipelineState(&desc[i], &pso[i]);
	}
	customShader.name = "Tree Animate Doublesided";
	customShader.filterMask = wi::enums::FILTER_OPAQUE;
	for (int i = 0; i < RENDERPASS_COUNT; i++)
		customShader.pso[i] = pso[i];

	RegisterCustomShader(customShader);

	//PE: Water object.
	PipelineState psowater;
	PipelineStateDesc descwater;
	CustomShader customWaterShader;
	if (!LoadShader(ShaderStage::PS, shaderWaterPS, "objectPS_custom_water.cso"))
		; // customWaterShader.bActive removed - bActive no longer exists in CustomShader
	//wiRenderer::AddPipelineDesc(descwater, RENDERPASS_MAIN, PSTYPE_OBJECT_PERMUTATION_BEGIN, MaterialComponent::SHADERTYPE::SHADERTYPE_PBR, BLENDMODE_ALPHA, OBJECTRENDERING_DOUBLESIDED_DISABLED, false, false, true); // AddPipelineDesc removed from new API
	{
		descwater.ps = &shaderWaterPS;
	}
	wiGraphics::GetDevice()->CreatePipelineState(&descwater, &psowater);
	customWaterShader.name = "Water Object";
	customWaterShader.filterMask = wi::enums::FILTER_TRANSPARENT;
	customWaterShader.pso[RENDERPASS_MAIN] = psowater;
	RegisterCustomShader(customWaterShader);


	//PE: Glass object.
	PipelineState psoglass;
	PipelineStateDesc descglass;
	PipelineState psoglassshadow;
	PipelineStateDesc descglassshadow;
	CustomShader customglassShader;
	if(!LoadShader(ShaderStage::PS, shaderGlassPS, "objectPS_transparent_glass.cso"))
		; // customglassShader.bActive removed - bActive no longer exists in CustomShader
	//wiRenderer::AddPipelineDesc(descglass, RENDERPASS_MAIN, PSTYPE_OBJECT_PERMUTATION_BEGIN, MaterialComponent::SHADERTYPE::SHADERTYPE_PBR, BLENDMODE_ALPHA, OBJECTRENDERING_DOUBLESIDED_DISABLED, false, false, true); // AddPipelineDesc removed from new API
	//wiRenderer::AddPipelineDesc(descglassshadow, RENDERPASS_SHADOW, PSTYPE_OBJECT_PERMUTATION_BEGIN, MaterialComponent::SHADERTYPE::SHADERTYPE_PBR, BLENDMODE_ALPHA, OBJECTRENDERING_DOUBLESIDED_DISABLED, false, false, true); // AddPipelineDesc removed from new API

	{
		descglass.ps = &shaderGlassPS;
	}
	wiGraphics::GetDevice()->CreatePipelineState(&descglass, &psoglass);
	wiGraphics::GetDevice()->CreatePipelineState(&descglassshadow, &psoglassshadow);
	customglassShader.name = "Glass Object";
	customglassShader.filterMask = wi::enums::FILTER_TRANSPARENT;
	customglassShader.pso[RENDERPASS_MAIN] = psoglass;
	customglassShader.pso[RENDERPASS_SHADOW] = psoglassshadow;
	RegisterCustomShader(customglassShader);

	//PE: Grid object.
	PipelineState psogrid;
	PipelineStateDesc descgrid;
	CustomShader customgridShader;
	if (!LoadShader(ShaderStage::PS, shaderGridPS, "objectPS_grid.cso"))
		; // customgridShader.bActive removed - bActive no longer exists in CustomShader
	//wiRenderer::AddPipelineDesc(descgrid, RENDERPASS_MAIN, PSTYPE_OBJECT_PERMUTATION_BEGIN, MaterialComponent::SHADERTYPE::SHADERTYPE_PBR, BLENDMODE_ALPHA, OBJECTRENDERING_DOUBLESIDED_DISABLED, false, false, true); // AddPipelineDesc removed from new API

	{
		descgrid.ps = &shaderGridPS;
	}
	wiGraphics::GetDevice()->CreatePipelineState(&descgrid, &psogrid);
	customgridShader.name = "grid Object";
	customgridShader.filterMask = wi::enums::FILTER_TRANSPARENT;
	customgridShader.pso[RENDERPASS_MAIN] = psogrid;
	RegisterCustomShader(customgridShader);


	//PE: blood damage shader.
	PipelineState psoBloodDamage;
	PipelineStateDesc descBloodDamage;
	CustomShader customBloodDamageShader;
	if (!LoadShader(ShaderStage::PS, damageBloodPS, "damageBloodPS.cso"))
		; // customBloodDamageShader.bActive removed - bActive no longer exists in CustomShader
	if (!LoadShader(ShaderStage::VS, damageBloodVS, "damageBloodVS.cso"))
		; // customShader.bActive removed - bActive no longer exists in CustomShader


	PipelineStateDesc desc2[RENDERPASS_COUNT];
	PipelineState pso2[RENDERPASS_COUNT];
	for (int i = 0; i < RENDERPASS_COUNT; i++)
	{
		//wiRenderer::AddPipelineDesc(desc2[i], i, PSTYPE_OBJECT_PERMUTATION_BEGIN, MaterialComponent::SHADERTYPE::SHADERTYPE_PBR, BLENDMODE_OPAQUE, OBJECTRENDERING_DOUBLESIDED_ENABLED, false, true, false); // AddPipelineDesc removed from new API
		{
			if (i == RENDERPASS_MAIN)
			{
				desc2[i].ps = &damageBloodPS;
				desc2[i].vs = &damageBloodVS;
			}
		}
		wiGraphics::GetDevice()->CreatePipelineState(&desc2[i], &pso2[i]);
	}

	customBloodDamageShader.name = "Blood Damage";
	customBloodDamageShader.filterMask = wi::enums::FILTER_OPAQUE;
	for (int i = 0; i < RENDERPASS_COUNT; i++)
		customBloodDamageShader.pso[i] = pso2[i];

	RegisterCustomShader(customBloodDamageShader);


}