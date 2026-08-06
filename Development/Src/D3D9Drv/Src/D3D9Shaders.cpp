/*=============================================================================
	D3D9Shaders.cpp: D3D shader RHI implementation.
	Copyright 1998-2012 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "D3D9DrvPrivate.h"

//@zombie_ps4_begin
FVertexShaderRHIRef FD3D9DynamicRHI::CreateVertexShader(const FShaderType* /*InType*/,const TArray<BYTE>& Code)
//@zombie_ps4_end
{
	check(Code.Num());
	TRefCountPtr<FD3D9VertexShader> VertexShader;
	VERIFYD3D9RESULT(Direct3DDevice->CreateVertexShader((DWORD*)&Code(0),(IDirect3DVertexShader9**)VertexShader.GetInitReference()));
	return VertexShader.GetReference();
}

FPixelShaderRHIRef FD3D9DynamicRHI::CreatePixelShader(const TArray<BYTE>& Code)
{
	check(Code.Num());
	TRefCountPtr<FD3D9PixelShader> PixelShader = NULL;
	VERIFYD3D9RESULT(Direct3DDevice->CreatePixelShader((DWORD*)&Code(0),(IDirect3DPixelShader9**)PixelShader.GetInitReference()));
	return PixelShader.GetReference();
}

static HRESULT D3D9SafeGetShaderConstantTable(const DWORD* pCode, ID3DXConstantTable** ppTable)
{
	__try
	{
		return D3DXGetShaderConstantTable(pCode, ppTable);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
}

static INT GBuildMapDiagCount = 0;

void FD3D9DynamicRHI::BuildShaderParameterMap(const TArray<BYTE>& Code, FShaderParameterMap& OutParameterMap)
{
	if (Code.Num() < 4)
		return;

	// Validate D3D9 shader signature: ps_3_0 = 0xFFFF0300, vs_3_0 = 0xFFFE0300
	// DXBC (D3D10/11) bytecode starts with 0x43425844 — reject it.
	const DWORD FirstDword = *(const DWORD*)Code.GetData();
	const UBOOL bIsPixelShader = ((FirstDword & 0xFFFF0000) == 0xFFFF0000);
	const UBOOL bIsVertexShader = ((FirstDword & 0xFFFF0000) == 0xFFFE0000);
	if (!bIsPixelShader && !bIsVertexShader)
		return;

	TRefCountPtr<ID3DXConstantTable> ConstantTable;
	if (FAILED(D3D9SafeGetShaderConstantTable((const DWORD*)Code.GetData(), ConstantTable.GetInitReference())))
		return;

	D3DXCONSTANTTABLE_DESC TableDesc;
	if (FAILED(ConstantTable->GetDesc(&TableDesc)))
		return;

	// Collect samplers first to decide whether to log (only log PS with >=3 samplers).
	struct FSamplerEntry { UINT RegIndex; UINT RegCount; const char* Name; };
	TArray<FSamplerEntry> Samplers;
	for (UINT i = 0; i < TableDesc.Constants; i++)
	{
		D3DXHANDLE Handle = ConstantTable->GetConstant(NULL, i);
		D3DXCONSTANT_DESC ConstDesc;
		UINT NumConstants = 1;
		if (FAILED(D3D9SafeGetConstantDesc(ConstantTable, Handle, &ConstDesc, &NumConstants)))
			continue;

		if (ConstDesc.RegisterSet == D3DXRS_SAMPLER)
		{
			FSamplerEntry E;
			E.RegIndex = ConstDesc.RegisterIndex;
			E.RegCount = ConstDesc.RegisterCount;
			E.Name     = ConstDesc.Name;
			Samplers.AddItem(E);
			OutParameterMap.AddParameterAllocation(
				ANSI_TO_TCHAR(ConstDesc.Name), 0,
				ConstDesc.RegisterIndex, ConstDesc.RegisterCount, ConstDesc.RegisterIndex);
		}
		else
		{
			OutParameterMap.AddParameterAllocation(
				ANSI_TO_TCHAR(ConstDesc.Name), 0,
				ConstDesc.RegisterIndex * sizeof(FLOAT) * 4,
				ConstDesc.RegisterCount * sizeof(FLOAT) * 4, 0);
		}
	}

	(void)GBuildMapDiagCount;
}

void RHIBuildShaderParameterMap(const TArray<BYTE>& Code, FShaderParameterMap& OutParameterMap)
{
	check(GDynamicRHI);
	GDynamicRHI->BuildShaderParameterMap(Code, OutParameterMap);
}

FString RHIDisassembleShader(const TArray<BYTE>& Code)
{
	if (Code.Num() == 0)
		return TEXT("(empty)");
	LPD3DXBUFFER pDisasm = NULL;
	HRESULT hr = D3DXDisassembleShader((const DWORD*)&Code(0), FALSE, NULL, &pDisasm);
	if (FAILED(hr) || !pDisasm)
		return FString::Printf(TEXT("(D3DXDisassembleShader failed: 0x%08X)"), hr);
	FString Result = ANSI_TO_TCHAR((const char*)pDisasm->GetBufferPointer());
	pDisasm->Release();
	return Result;
}

FD3D9BoundShaderState::FD3D9BoundShaderState(
	FVertexDeclarationRHIParamRef InVertexDeclarationRHI,
	DWORD* InStreamStrides,
	FVertexShaderRHIParamRef InVertexShaderRHI,
	FPixelShaderRHIParamRef InPixelShaderRHI
	):
	CacheLink(InVertexDeclarationRHI,InStreamStrides,InVertexShaderRHI,InPixelShaderRHI,this)
{
	DYNAMIC_CAST_D3D9RESOURCE(VertexDeclaration,InVertexDeclaration);
	DYNAMIC_CAST_D3D9RESOURCE(VertexShader,InVertexShader);
	DYNAMIC_CAST_D3D9RESOURCE(PixelShader,InPixelShader);

	VertexDeclaration = InVertexDeclaration;
	check(IsValidRef(VertexDeclaration));
	VertexShader = InVertexShader;
	PixelShader = InPixelShader;
}

/**
 * Creates a bound shader state instance which encapsulates a decl, vertex shader, and pixel shader
 * @param VertexDeclaration - existing vertex decl
 * @param StreamStrides - optional stream strides
 * @param VertexShader - existing vertex shader
 * @param PixelShader - existing pixel shader
 * @param MobileGlobalShaderType - global shader type to use for mobile
 */
FBoundShaderStateRHIRef FD3D9DynamicRHI::CreateBoundShaderState(
	FVertexDeclarationRHIParamRef VertexDeclarationRHI, 
	DWORD* StreamStrides, 
	FVertexShaderRHIParamRef VertexShaderRHI, 
	FPixelShaderRHIParamRef PixelShaderRHI,
	EMobileGlobalShaderType MobileGlobalShaderType
	)
{
	// Check for an existing bound shader state which matches the parameters
	FCachedBoundShaderStateLink* CachedBoundShaderStateLink = GetCachedBoundShaderState(
		VertexDeclarationRHI,
		StreamStrides,
		VertexShaderRHI,
		PixelShaderRHI
		);
	if(CachedBoundShaderStateLink)
	{
		// If we've already created a bound shader state with these parameters, reuse it.
		return CachedBoundShaderStateLink->BoundShaderState;
	}
	else
	{
		return new FD3D9BoundShaderState(VertexDeclarationRHI,StreamStrides,VertexShaderRHI,PixelShaderRHI);
	}
}
