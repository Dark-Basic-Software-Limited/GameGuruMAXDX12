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

// Load a custom shader from GGTerrain\CustomShaders\ with WickedEngine shaders as include path
static bool LoadCustomShader(ShaderStage stage, Shader& shader, const std::string& filename)
{
	// Custom shader source lives alongside this .cpp file
	std::string customDir = wi::helper::GetDirectoryFromPath(std::string(__FILE__)) + "GGTerrain/CustomShaders/";
	wi::helper::MakePathAbsolute(customDir);

	// WickedEngine shaders directory (for #include "objectHF.hlsli" etc.)
	std::string engineShaderDir = wiRenderer::GetShaderSourcePath();
	wi::helper::MakePathAbsolute(engineShaderDir);

	std::string shaderbinaryfilename = wiRenderer::GetShaderPath() + filename;

	wi::shadercompiler::CompilerInput input;
	input.format = wiGraphics::GetDevice()->GetShaderFormat();
	input.stage = stage;
	input.include_directories.push_back(engineShaderDir);
	input.include_directories.push_back(customDir);
	input.shadersourcefilename = wi::helper::ReplaceExtension(customDir + filename, "hlsl");

	wi::shadercompiler::CompilerOutput output;
	wi::shadercompiler::Compile(input, output);

	if (output.IsValid())
	{
		wi::shadercompiler::SaveShaderAndMetadata(shaderbinaryfilename, output);
		wi::backlog::post("custom shader compiled: " + shaderbinaryfilename);
		return wiGraphics::GetDevice()->CreateShader(stage, output.shaderdata, output.shadersize, &shader);
	}
	else
	{
		wi::backlog::post("custom shader compile FAILED: " + filename + "\n" + output.error_message, wi::backlog::LogLevel::Error);
		return false;
	}
}

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
	bool bTreeShadersValid = true;
	if(!LoadCustomShader(ShaderStage::VS, shaderMainTreeAnimateVS, "objectVS_common_tree.cso"))
		bTreeShadersValid = false;
	if(!LoadCustomShader(ShaderStage::VS, shaderPrepassTreeAnimateVS, "objectVS_prepass_trees.cso"))
		bTreeShadersValid = false;
	if(!LoadCustomShader(ShaderStage::VS, shaderShadowTreeAnimateVS, "shadowVS_alphatest_tree.cso"))
		bTreeShadersValid = false;

	if (bTreeShadersValid)
	{
		// DX12: Only create PSOs for render passes that have shaders assigned.
		// Empty PipelineStateDescs crash in DX12 (no root signature to extract).
		PipelineStateDesc descMain, descShadow, descPrepass;
		PipelineState psoMain, psoShadow, psoPrepass;
		descMain.vs = &shaderMainTreeAnimateVS;
		wiGraphics::GetDevice()->CreatePipelineState(&descMain, &psoMain);
		descShadow.vs = &shaderShadowTreeAnimateVS;
		wiGraphics::GetDevice()->CreatePipelineState(&descShadow, &psoShadow);
		descPrepass.vs = &shaderPrepassTreeAnimateVS;
		wiGraphics::GetDevice()->CreatePipelineState(&descPrepass, &psoPrepass);

		customShader.name = "Tree Animate Doublesided";
		customShader.filterMask = wi::enums::FILTER_OPAQUE;
		customShader.pso[RENDERPASS_MAIN] = psoMain;
		customShader.pso[RENDERPASS_SHADOW] = psoShadow;
		customShader.pso[RENDERPASS_PREPASS] = psoPrepass;

		RegisterCustomShader(customShader);
	}

	//PE: Water object.
	if (LoadCustomShader(ShaderStage::PS, shaderWaterPS, "objectPS_custom_water.cso"))
	{
		PipelineState psowater;
		PipelineStateDesc descwater;
		CustomShader customWaterShader;
		//wiRenderer::AddPipelineDesc(descwater, RENDERPASS_MAIN, PSTYPE_OBJECT_PERMUTATION_BEGIN, MaterialComponent::SHADERTYPE::SHADERTYPE_PBR, BLENDMODE_ALPHA, OBJECTRENDERING_DOUBLESIDED_DISABLED, false, false, true); // AddPipelineDesc removed from new API
		{
			descwater.ps = &shaderWaterPS;
		}
		wiGraphics::GetDevice()->CreatePipelineState(&descwater, &psowater);
		customWaterShader.name = "Water Object";
		customWaterShader.filterMask = wi::enums::FILTER_TRANSPARENT;
		customWaterShader.pso[RENDERPASS_MAIN] = psowater;
		RegisterCustomShader(customWaterShader);
	}


	//PE: Glass object.
	if (LoadCustomShader(ShaderStage::PS, shaderGlassPS, "objectPS_transparent_glass.cso"))
	{
		PipelineState psoglass;
		PipelineStateDesc descglass;
		CustomShader customglassShader;
		// DX12: Only create PSO for MAIN — shadow PSO had no shaders assigned (would crash)
		descglass.ps = &shaderGlassPS;
		wiGraphics::GetDevice()->CreatePipelineState(&descglass, &psoglass);
		customglassShader.name = "Glass Object";
		customglassShader.filterMask = wi::enums::FILTER_TRANSPARENT;
		customglassShader.pso[RENDERPASS_MAIN] = psoglass;
		RegisterCustomShader(customglassShader);
	}

	//PE: Grid object.
	if (LoadCustomShader(ShaderStage::PS, shaderGridPS, "objectPS_grid.cso"))
	{
		PipelineState psogrid;
		PipelineStateDesc descgrid;
		CustomShader customgridShader;
		//wiRenderer::AddPipelineDesc(descgrid, RENDERPASS_MAIN, PSTYPE_OBJECT_PERMUTATION_BEGIN, MaterialComponent::SHADERTYPE::SHADERTYPE_PBR, BLENDMODE_ALPHA, OBJECTRENDERING_DOUBLESIDED_DISABLED, false, false, true); // AddPipelineDesc removed from new API

		{
			descgrid.ps = &shaderGridPS;
		}
		wiGraphics::GetDevice()->CreatePipelineState(&descgrid, &psogrid);
		customgridShader.name = "grid Object";
		customgridShader.filterMask = wi::enums::FILTER_TRANSPARENT;
		customgridShader.pso[RENDERPASS_MAIN] = psogrid;
		RegisterCustomShader(customgridShader);
	}


	//PE: blood damage shader.
	bool bBloodShadersValid = true;
	if (!LoadCustomShader(ShaderStage::PS, damageBloodPS, "damageBloodPS.cso"))
		bBloodShadersValid = false;
	if (!LoadCustomShader(ShaderStage::VS, damageBloodVS, "damageBloodVS.cso"))
		bBloodShadersValid = false;

	if (bBloodShadersValid)
	{
		CustomShader customBloodDamageShader;
		// DX12: Only create PSO for MAIN — other render passes have no shaders assigned
		PipelineStateDesc descBlood;
		PipelineState psoBlood;
		descBlood.ps = &damageBloodPS;
		descBlood.vs = &damageBloodVS;
		wiGraphics::GetDevice()->CreatePipelineState(&descBlood, &psoBlood);

		customBloodDamageShader.name = "Blood Damage";
		customBloodDamageShader.filterMask = wi::enums::FILTER_OPAQUE;
		customBloodDamageShader.pso[RENDERPASS_MAIN] = psoBlood;

		RegisterCustomShader(customBloodDamageShader);
	}


}