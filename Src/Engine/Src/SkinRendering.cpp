/*=============================================================================
	SubsurfaceScatteringRendering.cpp: Subsurface scattering rendering implementation.
	Copyright 1998-2012 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"

/** A vertex shader for subsurface scattering. */
class FSkinVertexShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSkinVertexShader,Global);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	FSkinVertexShader()	{}
	FSkinVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{}

	void SetParameters(const FViewInfo& View)
	{}

	virtual UBOOL Serialize(FArchive& Ar)
	{
		UBOOL bShaderHasOutdatedParameters = FShader::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}
};

/** A pixel shader for subsurface scattering. */
class FSkinStencilPixelShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSkinStencilPixelShader,Global)

public:

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TRUE;
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
	}

	FSkinStencilPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FGlobalShader(Initializer)
	{
		SubsurfaceInscatteringTextureParameter.Bind(Initializer.ParameterMap,TEXT("SubsurfaceInscatteringTexture"),TRUE);
	}

	FSkinStencilPixelShader()
	{
	}

	void SetParameters(const FSceneView& View)
	{
		SetTextureParameter(GetPixelShader(),SubsurfaceInscatteringTextureParameter,TStaticSamplerState<>::GetRHI(),GSceneRenderTargets.GetSubsurfaceInscatteringTexture());
	}

	virtual void RebindParameters(const FShaderParameterMap& ParameterMap)
	{
		SubsurfaceInscatteringTextureParameter.Bind(ParameterMap, TEXT("SubsurfaceInscatteringTexture"), TRUE);
	}

	virtual UBOOL Serialize(FArchive& Ar)
	{
		UBOOL bShaderHasOutdatedParameters = FShader::Serialize(Ar);
		Ar << SubsurfaceInscatteringTextureParameter;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderResourceParameter SubsurfaceInscatteringTextureParameter;
};

/** A pixel shader for subsurface scattering. */
class FSkinPixelShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSkinPixelShader,Global)

public:

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TRUE;
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
	}

	FSkinPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	:	FGlobalShader(Initializer)
	{
		SceneTextureParameters.Bind(Initializer.ParameterMap);
		KernelSizeParameter.Bind(Initializer.ParameterMap,TEXT("KernelSize"),TRUE);
		SampleDeltaUVsParameter.Bind(Initializer.ParameterMap,TEXT("SampleDeltaUVs"),TRUE);
		ClipToViewScaleXYParameter.Bind(Initializer.ParameterMap,TEXT("ClipToViewScaleXY"),TRUE);
		ViewToClipScaleXYParameter.Bind(Initializer.ParameterMap,TEXT("ViewToClipScaleXY"),TRUE);
		WorldFilterRadiusParameter.Bind(Initializer.ParameterMap,TEXT("WorldFilterRadius"),TRUE);
		SubsurfaceInscatteringTextureParameter.Bind(Initializer.ParameterMap,TEXT("SubsurfaceInscatteringTexture"),TRUE);
		RandomAngleTextureParameter.Bind(Initializer.ParameterMap,TEXT("RandomAngleTexture"),TRUE);
		NoiseScaleAndOffsetParameter.Bind(Initializer.ParameterMap,TEXT("NoiseScaleAndOffset"),TRUE);
		SubsurfaceDiffuseColorTextureParameter.Bind(Initializer.ParameterMap,TEXT("SubsurfaceDiffuseColorTexture"),TRUE);
	}

	FSkinPixelShader()
	{
	}

	void SetParameters(const FSceneView& View)
	{
		SceneTextureParameters.Set(&View,this,SF_Point);

		// Let's keep the kernel size super stable in every possible
		// condition, even in the editor where the view/buffer sizes
		// dont match at all.
		FLOAT BufferAspectRatio = 
			((FLOAT)GSceneRenderTargets.GetBufferSizeX() / 
			 (FLOAT)GSceneRenderTargets.GetBufferSizeY());
		FLOAT FovCompensation = View.ProjectionMatrix.M[0][0];

		FVector4 KernelSize;
		if (BufferAspectRatio > 1.0f)
		{
			KernelSize.X = GSystemSettings.SkinFilterSize;
			KernelSize.Y = GSystemSettings.SkinFilterSize * BufferAspectRatio;
			KernelSize.Z = GSystemSettings.MaxSkinFilterSize;
			KernelSize.W = GSystemSettings.MaxSkinFilterSize * BufferAspectRatio;
			KernelSize *= (View.SizeX / (FLOAT)GSceneRenderTargets.GetBufferSizeX());
		}
		else
		{
			KernelSize.X = GSystemSettings.SkinFilterSize / BufferAspectRatio;
			KernelSize.Y = GSystemSettings.SkinFilterSize;
			KernelSize.Z = GSystemSettings.MaxSkinFilterSize / BufferAspectRatio;
			KernelSize.W = GSystemSettings.MaxSkinFilterSize;
			KernelSize *= (View.SizeY / (FLOAT)GSceneRenderTargets.GetBufferSizeY());
		}

		KernelSize.X *= FovCompensation;
		KernelSize.Y *= FovCompensation;

		SetPixelShaderValue(GetPixelShader(), KernelSizeParameter, KernelSize);

		// Set the random normal texture.
		UTexture2D* const RandomAngleTexture = GEngine->RandomAngleTextureCosSin;
		SetTextureParameter(
			GetPixelShader(),
			RandomAngleTextureParameter,
			TStaticSamplerState<SF_Point,AM_Wrap,AM_Wrap,AM_Wrap>::GetRHI(),
			RandomAngleTexture->Resource->TextureRHI
			);

		FRandomStream Random;
		Random.Initialize(appCycles());
		const FVector4 NoiseScaleAndOffset = FVector4(
			GSceneRenderTargets.GetBufferSizeX() / (FLOAT)RandomAngleTexture->SizeX, 
			GSceneRenderTargets.GetBufferSizeY() / (FLOAT)RandomAngleTexture->SizeY,
			Random.GetFraction(),
			Random.GetFraction());
		SetPixelShaderValue(GetPixelShader(), NoiseScaleAndOffsetParameter, NoiseScaleAndOffset);

		const FVector2D ClipToViewScaleXY(
			1.0f / View.ProjectionMatrix.M[0][0],
			1.0f / View.ProjectionMatrix.M[1][1]
			);
		SetPixelShaderValue(GetPixelShader(),ClipToViewScaleXYParameter,ClipToViewScaleXY);

		const FVector2D ViewToClipScaleXY(
			View.ProjectionMatrix.M[0][0],
			View.ProjectionMatrix.M[1][1]
			);
		SetPixelShaderValue(GetPixelShader(),ViewToClipScaleXYParameter,ViewToClipScaleXY);

		SetTextureParameter(GetPixelShader(),SubsurfaceInscatteringTextureParameter,TStaticSamplerState<>::GetRHI(),GSceneRenderTargets.GetSubsurfaceInscatteringTexture());
		SetTextureParameter(GetPixelShader(),SubsurfaceDiffuseColorTextureParameter,TStaticSamplerState<>::GetRHI(),GSceneRenderTargets.GetSubsurfaceScatteringAttenuationTexture());
	}

	virtual void RebindParameters(const FShaderParameterMap& ParameterMap)
	{
		SceneTextureParameters.Bind(ParameterMap);
		KernelSizeParameter.Bind(ParameterMap, TEXT("KernelSize"), TRUE);
		SampleDeltaUVsParameter.Bind(ParameterMap, TEXT("SampleDeltaUVs"), TRUE);
		ClipToViewScaleXYParameter.Bind(ParameterMap, TEXT("ClipToViewScaleXY"), TRUE);
		ViewToClipScaleXYParameter.Bind(ParameterMap, TEXT("ViewToClipScaleXY"), TRUE);
		WorldFilterRadiusParameter.Bind(ParameterMap, TEXT("WorldFilterRadius"), TRUE);
		SubsurfaceInscatteringTextureParameter.Bind(ParameterMap, TEXT("SubsurfaceInscatteringTexture"), TRUE);
		RandomAngleTextureParameter.Bind(ParameterMap, TEXT("RandomAngleTexture"), TRUE);
		NoiseScaleAndOffsetParameter.Bind(ParameterMap, TEXT("NoiseScaleAndOffset"), TRUE);
		SubsurfaceDiffuseColorTextureParameter.Bind(ParameterMap, TEXT("SubsurfaceDiffuseColorTexture"), TRUE);
	}

	virtual UBOOL Serialize(FArchive& Ar)
	{
		UBOOL bShaderHasOutdatedParameters = FShader::Serialize(Ar);
		Ar << SceneTextureParameters;
		Ar << KernelSizeParameter;
		Ar << SampleDeltaUVsParameter;
		Ar << ClipToViewScaleXYParameter;
		Ar << ViewToClipScaleXYParameter;
		Ar << WorldFilterRadiusParameter;
		Ar << SubsurfaceInscatteringTextureParameter;
		Ar << RandomAngleTextureParameter;
		Ar << NoiseScaleAndOffsetParameter;
		Ar << SubsurfaceDiffuseColorTextureParameter;
		return bShaderHasOutdatedParameters;
	}

private:

	FSceneTextureShaderParameters SceneTextureParameters;
	FShaderParameter KernelSizeParameter;
	FShaderParameter SampleDeltaUVsParameter;
	FShaderParameter ClipToViewScaleXYParameter;
	FShaderParameter ViewToClipScaleXYParameter;
	FShaderParameter WorldFilterRadiusParameter;
	FShaderResourceParameter SubsurfaceInscatteringTextureParameter;
	FShaderResourceParameter SubsurfaceDiffuseColorTextureParameter;
	FShaderResourceParameter RandomAngleTextureParameter;
	FShaderParameter NoiseScaleAndOffsetParameter;
};

IMPLEMENT_SHADER_TYPE(,FSkinVertexShader,TEXT("SkinVertexShader"),TEXT("Main"),SF_Vertex,0,0); 
IMPLEMENT_SHADER_TYPE(,FSkinPixelShader,TEXT("SkinPixelShader"),TEXT("Main"),SF_Pixel,0,0); 
IMPLEMENT_SHADER_TYPE(,FSkinStencilPixelShader,TEXT("SkinPixelShader"),TEXT("MainStencil"),SF_Pixel,0,0); 

/** The subsurface scattering vertex declaration resource type. */
class FSkinVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	// Destructor
	virtual ~FSkinVertexDeclaration() {}

	virtual void InitRHI()
	{
		FVertexDeclarationElementList Elements;
		Elements.AddItem(FVertexElement(0,0,VET_Float4,VEU_Position,0));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI()
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

/** Vertex declaration for the light function fullscreen 2D quad. */
TGlobalResource<FSkinVertexDeclaration> GSkinVertexDeclaration;

/** The bound shader state for the per-sample subsurface scattering shaders. */
FGlobalBoundShaderState GSkinBoundShaderState;
FGlobalBoundShaderState GSkinStencilBoundShaderState;

UBOOL FSceneRenderer::RenderSkin(UINT DPGIndex)
{
	if (DPGIndex == SDPG_World && GSystemSettings.bAllowSkin)
	{
		GSceneRenderTargets.ResolveSubsurfaceScatteringSurfaces();

		static const FVector4 Vertices[4] =
		{
			FVector4(-1,-1,0,1),
			FVector4(-1,+1,0,1),
			FVector4(+1,+1,0,1),
			FVector4(+1,-1,0,1),
		};
		static const WORD Indices[6] =
		{
			0, 1, 2,
			0, 2, 3
		};

		GSceneRenderTargets.BeginRenderingSceneColor();

		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			const FViewInfo& View = Views(ViewIndex);

			if (!View.bHasOutlastNightVision && View.VisibleDynamicSkinPrimitives.Num())
			{
				SCOPED_CONDITIONAL_DRAW_EVENT(EventRenderSS,ViewIndex == 0)(DEC_SCENE_ITEMS,TEXT("Skin Shading"));

				// Set the device viewport for the view.
				RHISetViewport(View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);
				RHISetViewParameters(View);
				RHIClear(FALSE,FLinearColor::Black,FALSE,0.0f,TRUE,0);
				RHISetColorWriteEnable(FALSE);
				RHISetDepthState(TStaticDepthState<FALSE,CF_Always>::GetRHI());
				RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());

				RHIBeginHiStencilRecord(TRUE, 1);
				// Set to one if it passes.
				RHISetStencilState(TStaticStencilState<
					TRUE,CF_Always,SO_Keep,SO_Keep,SO_Replace,
					FALSE,CF_Never,SO_Keep,SO_Keep,SO_Replace,
					0xff,0xff,1
				>::GetRHI());			

				TShaderMapRef<FSkinVertexShader> VertexShader(GetGlobalShaderMap());
				TShaderMapRef<FSkinStencilPixelShader> StencilPixelShader(GetGlobalShaderMap());
				// Set the non-MSAA subsurface scattering shaders.
				SetGlobalBoundShaderState(
					GSkinStencilBoundShaderState,
					GSkinVertexDeclaration.VertexDeclarationRHI,
					*VertexShader,
					*StencilPixelShader,
					sizeof(FVector2D)
					);
				VertexShader->SetParameters(View);
				StencilPixelShader->SetParameters(View);

				// Draw a quad covering the view.
				RHIDrawIndexedPrimitiveUP(
					PT_TriangleList,
					0,
					ARRAY_COUNT(Vertices),
					2,
					Indices,
					sizeof(Indices[0]),
					Vertices,
					sizeof(Vertices[0])
					);

				// No depth or stencil tests, no backface culling.
				RHISetColorWriteEnable(TRUE);
				// Pass if 1.
				RHISetStencilState(TStaticStencilState<
					TRUE,CF_Equal,SO_Keep,SO_Keep,SO_Keep,
					FALSE,CF_Never,SO_Keep,SO_Keep,SO_Keep,
					0xff,0xff,1>::GetRHI());
				RHIBeginHiStencilPlayback(TRUE);

				// Use additive blending for color, and keep the destination alpha.
				RHISetBlendState(TStaticBlendState<BO_Add,BF_One,BF_One,BO_Add,BF_Zero,BF_One>::GetRHI());

				// Set the non-MSAA subsurface scattering shaders.
				TShaderMapRef<FSkinPixelShader> PixelShader(GetGlobalShaderMap());
				SetGlobalBoundShaderState(
					GSkinBoundShaderState,
					GSkinVertexDeclaration.VertexDeclarationRHI,
					*VertexShader,
					*PixelShader,
					sizeof(FVector2D)
					);
				PixelShader->SetParameters(View);

				// Draw a quad covering the view.
				RHIDrawIndexedPrimitiveUP(
					PT_TriangleList,
					0,
					ARRAY_COUNT(Vertices),
					2,
					Indices,
					sizeof(Indices[0]),
					Vertices,
					sizeof(Vertices[0])
					);

				RHISetStencilState(TStaticStencilState<>::GetRHI());
			}
		}
		return TRUE;
	}

	return FALSE;
}
