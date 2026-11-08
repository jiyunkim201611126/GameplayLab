// Fill out your copyright notice in the Description page of Project Settings.

#include "SmokeField.h"

namespace GLSmokeParameters
{
	const FName SmokeCenter(TEXT("SmokeCenter"));
	const FName SmokeRadius(TEXT("SmokeRadius"));
	const FName SmokeDensity(TEXT("SmokeDensity"));
	
	const FName SphereCutCenter(TEXT("SphereCutCenter"));
	const FName SphereCutRadius(TEXT("SphereCutRadius"));
	const FName SphereCutStrength(TEXT("SphereCutStrength"));
	
	const FName SegmentCutStart(TEXT("SegmentCutStart"));
	const FName SegmentCutEnd(TEXT("SegmentCutEnd"));
	const FName SegmentCutRadius(TEXT("SegmentCutRadius"));
	const FName SegmentCutStrength(TEXT("SegmentCutStrength"));

	static FLinearColor ToVectorParameter(const FVector& Value)
	{
		return FLinearColor(Value.X, Value.Y, Value.Z, 0.f);
	}
}

ASmokeField::ASmokeField()
{
	PrimaryActorTick.bCanEverTick = true;
	
	VolumeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VolumeMesh"));
	SetRootComponent(VolumeMesh);
	
	VolumeMesh->SetMobility(EComponentMobility::Movable);
	VolumeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VolumeMesh->SetGenerateOverlapEvents(false);
	VolumeMesh->SetCastShadow(false);
	VolumeMesh->SetVisibility(false);
}

void ASmokeField::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	UpdateVolumeBounds();
	InitializeMaterial();
	UpdateMaterialParameters();
}

void ASmokeField::BeginPlay()
{
	Super::BeginPlay();
	
	ClearCuts();
	UpdateVolumeBounds();
	InitializeMaterial();
	UpdateMaterialParameters();
}

void ASmokeField::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	SphereCut.Advance(DeltaSeconds);
	SegmentCut.Advance(DeltaSeconds);
	
	UpdateMaterialParameters();
}

void ASmokeField::ApplySphereCut(const FVector& Center, const float Radius, const float Strength,
	const float ExpandDuration, const float HoldDuration, const float RecoveryDuration)
{
	SphereCut = FCutState();
	
	SphereCut.Start = Center;
	SphereCut.End = Center;
	
	SphereCut.Radius = FMath::Max(Radius, 0.f);
	SphereCut.InitialStrength = FMath::Clamp(Strength, 0.f, 1.f);
	
	SphereCut.ExpandDuration = FMath::Max(ExpandDuration, 0.f);
	SphereCut.HoldDuration = FMath::Max(HoldDuration, 0.f);
	SphereCut.RecoveryDuration = FMath::Max(RecoveryDuration, 0.f);
	
	UpdateMaterialParameters();
}

void ASmokeField::ApplySegmentCut(const FVector& Start, const FVector& End, const float Radius, const float Strength,
	const float ExpandDuration, const float HoldDuration, const float RecoveryDuration)
{
	const FVector Segment = End - Start;
	const double SegmentLength = Segment.Length();
	if (SegmentLength <= UE_SMALL_NUMBER)
	{
		return;
	}
	
	const FVector Direction = Segment / SegmentLength;
	const FVector StartToCenter = GetActorLocation() - Start;
	
	// Start End 방향으로 무한 직선을 가정하고, 그 직선 위 점 중 가장 연막 중심과 가까운 지점부터 Start까지의 거리
	const double CenterDistance = FVector::DotProduct(StartToCenter, Direction);
	
	// 직선과 연막 중심 사이의 최단 거리 제곱(피타고라스)
	const double ClosetDistanceSquared = StartToCenter.SizeSquared() - FMath::Square(CenterDistance);
	
	// 무한 직선 자체가 연막 구를 지나지 않음
	if (ClosetDistanceSquared > FMath::Square(static_cast<double>(SmokeRadius)))
	{
		return;
	}
	
	SegmentCut = FCutState();
	
	SegmentCut.Start = Start;
	
	// End 지점이 반드시 구 바깥까지 통과하도록 보장
	SegmentCut.End = End + Direction * (SmokeRadius * 2.f + Radius);
	
	SegmentCut.Radius = FMath::Max(Radius, 0.0f);
	SegmentCut.InitialStrength = FMath::Clamp(Strength, 0.0f, 1.0f);
	
	SegmentCut.ExpandDuration = FMath::Max(ExpandDuration, 0.f);
	SegmentCut.HoldDuration = FMath::Max(HoldDuration, 0.f);
	SegmentCut.RecoveryDuration = FMath::Max(RecoveryDuration, 0.0f);
	
	UpdateMaterialParameters();
}

void ASmokeField::ClearCuts()
{
	SphereCut = FCutState();
	SegmentCut = FCutState();
	
	UpdateMaterialParameters();
}

void ASmokeField::InitializeMaterial()
{
	SmokeMID = nullptr;
	
	if (SmokeMaterial)
	{
		SmokeMID = UMaterialInstanceDynamic::Create(SmokeMaterial, this);
	}
	
	VolumeMesh->SetMaterial(0, SmokeMID);
	VolumeMesh->SetVisibility(SmokeMID != nullptr);
}

void ASmokeField::UpdateVolumeBounds()
{
	SmokeRadius = FMath::Max(SmokeRadius, 1.f);
	
	// 100cm Cube를 연막 지름에 맞춤
	// 크기는 액터 Scale 대신 SmokeRadius로 조절
	const float MeshScale = (SmokeRadius * 2.f) / 100.f;
	VolumeMesh->SetWorldScale3D(FVector(MeshScale));
}

void ASmokeField::UpdateMaterialParameters() const
{
	if (!SmokeMID)
	{
		return;
	}
	
	using namespace GLSmokeParameters;
	
	SmokeMID->SetVectorParameterValue(GLSmokeParameters::SmokeCenter, ToVectorParameter(GetActorLocation()));
	SmokeMID->SetScalarParameterValue(GLSmokeParameters::SmokeRadius, SmokeRadius);
	SmokeMID->SetScalarParameterValue(GLSmokeParameters::SmokeDensity, FMath::Max(SmokeDensity, 0.f));
	
	SmokeMID->SetVectorParameterValue(SphereCutCenter, ToVectorParameter(SphereCut.Start));
	SmokeMID->SetScalarParameterValue(SphereCutRadius, SphereCut.GetRadius());
	SmokeMID->SetScalarParameterValue(SphereCutStrength, SphereCut.GetStrength());
	
	SmokeMID->SetVectorParameterValue(SegmentCutStart, ToVectorParameter(SegmentCut.Start));
	SmokeMID->SetVectorParameterValue(SegmentCutEnd, ToVectorParameter(SegmentCut.End));
	SmokeMID->SetScalarParameterValue(SegmentCutRadius, SegmentCut.GetRadius());
	SmokeMID->SetScalarParameterValue(SegmentCutStrength, SegmentCut.GetStrength());
}

void ASmokeField::FCutState::Advance(const float DeltaSeconds)
{
	Elapsed += FMath::Max(DeltaSeconds, 0.f);
}

float ASmokeField::FCutState::GetRadius() const
{
	if (ExpandDuration <= 0.f)
	{
		return Radius;
	}
	
	const float Alpha = FMath::Clamp(Elapsed / ExpandDuration, 0.f, 1.f);
	
	// 초반에 빠르게 확장하고 목표 반경에서 감속
	const float Expansion = 1.f - FMath::Square(1.f - Alpha);
	
	return Radius * Expansion;
}

float ASmokeField::FCutState::GetStrength() const
{
	if (Radius <= 0.f || InitialStrength <= 0.f)
	{
		return 0.f;
	}
	
	// Duration이 0인 경우 복구 없음
	if (RecoveryDuration <= 0.f)
	{
		return InitialStrength;
	}

	const float RecoveryStartTime = ExpandDuration + HoldDuration;
	
	// 확장 및 유지 구간에서는 제거 강도를 유지
	if (Elapsed <= RecoveryStartTime)
	{
		return InitialStrength;
	}
	
	const float Alpha = FMath::Clamp((Elapsed - RecoveryStartTime) / RecoveryDuration, 0.f, 1.f);
	const float Recovery = Alpha * Alpha * (3.f - 2.f * Alpha);
	
	return InitialStrength * (1.f - Recovery);
}
