// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Containers/Queue.h"
#include "SharedMemoryManager.h"
#include "CameraCaptureManager.generated.h"

class ASceneCapture2D;
class UMaterial;

USTRUCT()
struct FRenderRequestStruct {
	GENERATED_BODY()

	TArray<FColor> Image;
	FRenderCommandFence RenderFence;

	FRenderRequestStruct() {

	}
};

UCLASS()
class MYPROJECT_API ACameraCaptureManager : public AActor
{
	GENERATED_BODY()

private:
	SharedMemoryManager shared_memory;

public:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
		int FrameWidth = 600;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
		int FrameHeight = 300;
	// Color Capture Components
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	ASceneCapture2D* CaptureComponent;

	// PostProcessMaterial used for segmentation
	UPROPERTY(EditAnywhere, Category = "Capture")
	UMaterial* PostProcessMaterial = nullptr;

	// Sets default values for this actor's properties
	ACameraCaptureManager();

protected:
	// RenderRequest Queue
	TQueue<FRenderRequestStruct*> RenderRequestQueue;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void SetupCaptureComponent();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "ImageCapture")
	void CaptureNonBlocking();
};
