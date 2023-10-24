// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraCaptureManager.h"
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageWrapper/Public/IImageWrapper.h"
#include "ImageWrapper/Public/IImageWrapperModule.h"
#include "ShowFlags.h"

// Sets default values
ACameraCaptureManager::ACameraCaptureManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACameraCaptureManager::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("BEGIN PLAY"));
	SetupCaptureComponent();
}

void ACameraCaptureManager::SetupCaptureComponent()
{
	if (!IsValid(CaptureComponent)) {
		UE_LOG(LogTemp, Error, TEXT("SetupCaptureComponent: CaptureComponent is not valid!"));
		return;
	}

	// Create RenderTargets
	UTextureRenderTarget2D* renderTarget2D = NewObject<UTextureRenderTarget2D>();
	renderTarget2D->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8; //8-bit color format
	renderTarget2D->InitCustomFormat(FrameWidth, FrameHeight, PF_B8G8R8A8, true); // PF... disables HDR, which is most important since HDR gives gigantic overhead, and is not needed!
	UE_LOG(LogTemp, Warning, TEXT("Set Render Format for Color-Like-Captures"));

	renderTarget2D->bGPUSharedFlag = true; // demand buffer on GPU

	// Assign RenderTarget
	CaptureComponent->GetCaptureComponent2D()->TextureTarget = renderTarget2D;
	// Set Camera Properties
	CaptureComponent->GetCaptureComponent2D()->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	CaptureComponent->GetCaptureComponent2D()->TextureTarget->TargetGamma = GEngine->GetDisplayGamma();
	CaptureComponent->GetCaptureComponent2D()->ShowFlags.SetTemporalAA(true);
	// lookup more showflags in the UE4 documentation..
	// Assign PostProcess Material if assigned
	if (PostProcessMaterial) { // check nullptr
		CaptureComponent->GetCaptureComponent2D()->AddOrUpdateBlendable(PostProcessMaterial);
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("No PostProcessMaterial is assigend"));
	}
	UE_LOG(LogTemp, Warning, TEXT("Initialized RenderTarget!"));
}

// Called every frame
void ACameraCaptureManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!RenderRequestQueue.IsEmpty()) {
		// Peek the next RenderRequest from queue
		FRenderRequestStruct* nextRenderRequest = nullptr;
		RenderRequestQueue.Peek(nextRenderRequest);
		if (nextRenderRequest) { //nullptr check
			if (nextRenderRequest->RenderFence.IsFenceComplete()) { // Check if rendering is done, indicated by RenderFence

				int frame_size = 640 * 480 * 3;
				char* unpacked_frame = new char[frame_size];
				int i = 0;
				for (auto pixel : nextRenderRequest->Image) {
					unpacked_frame[i] = pixel.B;
					unpacked_frame[i + 1] = pixel.G;
					unpacked_frame[i + 2] = pixel.R;
					i+=3;
				}
				shared_memory.push_frame(unpacked_frame, frame_size);

				UE_LOG(LogTemp, Log, TEXT("Frame pushed"));
				delete unpacked_frame;

				// Delete the first element from RenderQueue
				RenderRequestQueue.Pop();
				delete nextRenderRequest;
			}
		}
	}
	else {
		CaptureNonBlocking();
	}
}

void ACameraCaptureManager::CaptureNonBlocking()
{
	if (!IsValid(CaptureComponent)) {
		UE_LOG(LogTemp, Error, TEXT("CaptureColorNonBlocking: CaptureComponent was not valid!"));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("Entering: CaptureNonBlocking"));
	CaptureComponent->GetCaptureComponent2D()->TextureTarget->TargetGamma = 1.2f;//GEngine->GetDisplayGamma();

	// Get RenderConterxt
	FTextureRenderTargetResource* renderTargetResource = CaptureComponent->GetCaptureComponent2D()->TextureTarget->GameThread_GetRenderTargetResource();
	UE_LOG(LogTemp, Warning, TEXT("Got display gamma"));
	struct FReadSurfaceContext {
		FRenderTarget* SrcRenderTarget;
		TArray<FColor>* OutData;
		FIntRect Rect;
		FReadSurfaceDataFlags Flags;
	};

	UE_LOG(LogTemp, Warning, TEXT("Inited ReadSurfaceContext"));
	// Init new RenderRequest
	FRenderRequestStruct* renderRequest = new FRenderRequestStruct();
	UE_LOG(LogTemp, Warning, TEXT("inited renderrequest"));

	// Setup GPU command
	FReadSurfaceContext readSurfaceContext = {
		renderTargetResource,
		&(renderRequest->Image),
		FIntRect(0,0,renderTargetResource->GetSizeXY().X, renderTargetResource->GetSizeXY().Y),
		FReadSurfaceDataFlags(RCM_UNorm, CubeFace_MAX)
	};
	UE_LOG(LogTemp, Warning, TEXT("GPU Command complete"));

	ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
		[readSurfaceContext](FRHICommandListImmediate& RHICmdList) {
			RHICmdList.ReadSurfaceData(
				readSurfaceContext.SrcRenderTarget->GetRenderTargetTexture(),
				readSurfaceContext.Rect,
				*readSurfaceContext.OutData,
				readSurfaceContext.Flags
			);
		});

	// Notifiy new task in RenderQueue
	RenderRequestQueue.Enqueue(renderRequest);

	// Set RenderCommandFence
	renderRequest->RenderFence.BeginFence();
}

