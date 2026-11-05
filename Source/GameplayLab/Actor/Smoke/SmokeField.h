// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SmokeField.generated.h"

UCLASS()
class GAMEPLAYLAB_API ASmokeField : public AActor
{
	GENERATED_BODY()
	
public:
	ASmokeField();
	
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * 연막을 구형으로 제거
	 * @param Center 구형 제거 지점의 월드 좌표
	 * @param Radius 구 반지름
	 * @param Strength 강도, 0 ~ 1
	 * @param ExpandDuration Radius만큼 제거까지 도달하는 데 걸리는 시간
	 * @param HoldDuration 최대 제거 상태를 유지하는 시간
	 * @param RecoveryDuration 복구에 소요되는 시간, 0인 경우 복구하지 않음
	 */
	UFUNCTION(BlueprintCallable, Category = "Smoke")
	void ApplySphereCut(
		const FVector& Center,
		const float Radius,
		const float Strength = 1.f,
		const float ExpandDuration = 0.2f,
		const float HoldDuration = 1.f,
		const float RecoveryDuration = 0.f);

	/**
	 * 연막을 캡슐 형태로 제거
	 * @param Start 캡슐 시작 지점
	 * @param End 캡슐 끝 지점
	 * @param Radius 캡슐 반지름 
	 * @param Strength 강도, 0 ~ 1
	 * @param ExpandDuration Radius만큼 제거 상태까지 도달하는 데 걸리는 시간
	 * @param HoldDuration 최대 제거 상태를 유지하는 시간
	 * @param RecoveryDuration 복구에 소요되는 시간, 0인 경우 복구하지 않음
	 */
	UFUNCTION(BlueprintCallable, Category = "Smoke")
	void ApplySegmentCut(
		const FVector& Start,
		const FVector& End,
		const float Radius,
		const float Strength = 1.f,
		const float ExpandDuration = 0.2f,
		const float HoldDuration = 1.f,
		const float RecoveryDuration = 0.f);
	
	UFUNCTION(BlueprintCallable, Category = "Smoke")
	void ClearCuts();
	
protected:
	virtual void BeginPlay() override;
	
private:
	void InitializeMaterial();
	void UpdateVolumeBounds();
	void UpdateMaterialParameters() const;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smoke")
	TObjectPtr<UStaticMeshComponent> VolumeMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smoke")
	TObjectPtr<UMaterialInterface> SmokeMaterial;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smoke", meta = (ClampMin = "1.0", Units = "cm"))
	float SmokeRadius = 300.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smoke", meta = (ClampMin = "0.0"))
	float SmokeDensity = 1.f;
	
private:
	struct FCutState
	{
		FVector Start = FVector::ZeroVector;
		FVector End = FVector::ZeroVector;
		
		float Radius = 0.f;
		float InitialStrength = 0.f;
		float ExpandDuration = 0.f;
		float HoldDuration = 0.f;
		float RecoveryDuration = 0.f;
		float Elapsed = 0.f;
		
		void Advance(const float DeltaSeconds);
		float GetRadius() const;
		float GetStrength() const;
	};
	
	FCutState SphereCut;
	FCutState SegmentCut;
	
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SmokeMID;
};
