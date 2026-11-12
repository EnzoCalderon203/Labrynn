#include "Traps/LabPitTrap.h"

#include "Player/LabCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ALabPitTrap::ALabPitTrap()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FloorMat(TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark.MI_PrototypeGrid_TopDark"));

	FakeFloor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FakeFloor"));
	FakeFloor->SetupAttachment(Root);
	FakeFloor->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	if (Cube.Succeeded()) { FakeFloor->SetStaticMesh(Cube.Object); }
	if (FloorMat.Succeeded()) { FakeFloorMaterial = FloorMat.Object; }

	Bottom = CreateDefaultSubobject<UBoxComponent>(TEXT("Bottom"));
	Bottom->SetupAttachment(Root);
	Bottom->SetCollisionProfileName(TEXT("Trigger"));

	WarningTime = 0.5f;
	FiringTime = 0.5f;
	CooldownTime = 0.f; // queda abierto
}

void ALabPitTrap::BeginPlay()
{
	// Dimensiones a partir de CellSize/PitDepth (fijados antes de BeginPlay por quien spawnea).
	FakeFloor->SetRelativeScale3D(FVector(CellSize / 100.f, CellSize / 100.f, 0.2f));
	FakeFloor->SetRelativeLocation(FVector(0.f, 0.f, -10.f));
	if (FakeFloorMaterial) { FakeFloor->SetMaterial(0, FakeFloorMaterial); }
	Trigger->SetBoxExtent(FVector(CellSize * 0.35f, CellSize * 0.35f, 60.f));
	Bottom->SetBoxExtent(FVector(CellSize * 0.48f, CellSize * 0.48f, 60.f));
	Bottom->SetRelativeLocation(FVector(0.f, 0.f, -PitDepth + 60.f));

	Super::BeginPlay();

	if (HasAuthority())
	{
		Bottom->OnComponentBeginOverlap.AddDynamic(this, &ALabPitTrap::OnBottomOverlap);
	}
}

void ALabPitTrap::OnStateChanged(ELabTrapState OldState)
{
	Super::OnStateChanged(OldState);
	const ELabTrapState S = GetTrapState();
	if (S == ELabTrapState::Warning)
	{
		ShakeTime = WarningTime;
	}
	const bool bOpen = (S == ELabTrapState::Firing || S == ELabTrapState::Cooldown);
	FakeFloor->SetVisibility(!bOpen);
	FakeFloor->SetCollisionEnabled(bOpen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
}

void ALabPitTrap::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (ShakeTime > 0.f)
	{
		ShakeTime -= DeltaSeconds;
		const FVector Jitter = ShakeTime > 0.f ? FVector(FMath::FRandRange(-3.f, 3.f), FMath::FRandRange(-3.f, 3.f), FMath::FRandRange(-4.f, 0.f)) : FVector::ZeroVector;
		FakeFloor->SetRelativeLocation(FVector(0.f, 0.f, -10.f) + Jitter);
	}
}

void ALabPitTrap::OnBottomOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (ALabCharacter* Ch = Cast<ALabCharacter>(Other))
	{
		if (Ch->IsAliveAndFree())
		{
			Ch->KnockDown(FVector(0.f, 0.f, -100.f)); // golpe seco contra el fondo
		}
	}
}
