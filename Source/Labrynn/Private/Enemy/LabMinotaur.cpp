#include "Enemy/LabMinotaur.h"

#include "Enemy/LabMinotaurController.h"
#include "Enemy/LabMinotaurSettings.h"
#include "Player/LabCharacter.h"
#include "Interact/LabLantern.h"
#include "Core/LabGameState.h"
#include "Maze/LabMaze.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Labrynn.h"

ALabMinotaur::ALabMinotaur()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true; // sus pasos se oyen lejos
	AIControllerClass = ALabMinotaurController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(48.f, 130.f);

	static ConstructorHelpers::FObjectFinder<ULabMinotaurSettings> DA(TEXT("/Game/Labrynn/Core/DA_Minotaur.DA_Minotaur"));
	Settings = DA.Object;

	// Cuerpo provisorio: el maniquí de la plantilla, más grande (reemplazable en un BP hijo).
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Body(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> Anim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (Body.Succeeded()) { GetMesh()->SetSkeletalMeshAsset(Body.Object); }
	if (Anim.Succeeded()) { GetMesh()->SetAnimInstanceClass(Anim.Class); }
	GetMesh()->SetRelativeScale3D(FVector(1.35f));
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -130.f), FRotator(0.f, -90.f, 0.f));

	// Ojos rojos: se ven en la oscuridad antes que el cuerpo (telegrafía).
	EyesGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("EyesGlow"));
	EyesGlow->SetupAttachment(GetCapsuleComponent());
	EyesGlow->SetRelativeLocation(FVector(35.f, 0.f, 105.f));
	EyesGlow->SetLightColor(FLinearColor(1.f, 0.05f, 0.02f));
	EyesGlow->SetIntensity(900.f);
	EyesGlow->SetAttenuationRadius(320.f);
	EyesGlow->SetCastShadows(false);

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 360.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
	GetCharacterMovement()->MaxStepHeight = 85.f; // pasa escombros y vigas sin saltar
	bUseControllerRotationYaw = false;
}

const ULabMinotaurSettings* ALabMinotaur::GetSettings() const
{
	return Settings ? Settings.Get() : GetDefault<ULabMinotaurSettings>();
}

void ALabMinotaur::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALabMinotaur, bChasing);
	DOREPLIFETIME(ALabMinotaur, bRepelled);
}

void ALabMinotaur::BeginPlay()
{
	Super::BeginPlay();
	NextSnort = GetWorld()->GetTimeSeconds() + FMath::FRandRange(2.f, 5.f);
	UE_LOG(LogLabrynn, Log, TEXT("Minotauro: apareció en %s"), *GetActorLocation().ToCompactString());
}

ALabMaze* ALabMinotaur::GetMaze() const
{
	const ALabGameState* GS = GetWorld()->GetGameState<ALabGameState>();
	return GS ? GS->GetMaze() : nullptr;
}

void ALabMinotaur::OnHeardNoise(const FVector& Location, float Strength, FName Tag)
{
	const ULabMinotaurSettings* S = GetSettings();
	if (!HasAuthority() || Strength < S->MinNoiseStrength)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	const bool bOld = Now - NoiseTime > 3.f;
	if (bOld || Strength >= NoiseStrength * 0.7f)
	{
		const bool bWasChasing = bChasing;
		NoiseStrength = Strength;
		NoiseTime = Now;
		bChasing = true;
		UE_LOG(LogLabrynn, Log, TEXT("Minotauro: oyó '%s' (%.2f) en %s"), *Tag.ToString(), Strength, *Location.ToCompactString());
		if (!bWasChasing)
		{
			MulticastSound(RoarSound);
		}
		if (const ALabMaze* Maze = GetMaze())
		{
			RepathTo(Maze->WorldToCell(Location), /*bAvoidFire*/ true);
		}
	}
}

void ALabMinotaur::MulticastSound_Implementation(USoundBase* Sound)
{
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
}

void ALabMinotaur::RepathTo(const FIntPoint& RawTarget, bool bAvoidFire)
{
	const ALabMaze* Maze = GetMaze();
	if (!Maze)
	{
		return;
	}
	const FLabMazeLayout& L = Maze->GetLayout();
	const FIntPoint Target(FMath::Clamp(RawTarget.X, 0, L.Width - 1), FMath::Clamp(RawTarget.Y, 0, L.Height - 1));
	FIntPoint Start = Maze->WorldToCell(GetActorLocation());
	Start = FIntPoint(FMath::Clamp(Start.X, 0, L.Width - 1), FMath::Clamp(Start.Y, 0, L.Height - 1));

	// Celdas con fuego: se evitan (si se puede).
	TSet<FIntPoint> Fire;
	if (bAvoidFire)
	{
		for (TActorIterator<ALabLantern> It(GetWorld()); It; ++It)
		{
			if (It->IsLit())
			{
				Fire.Add(Maze->WorldToCell(It->GetActorLocation()));
			}
		}
		Fire.Remove(Start);
	}

	static const FIntPoint Dirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
	auto Bfs = [&](bool bUseFire, TMap<FIntPoint, FIntPoint>& Parent)
	{
		TArray<FIntPoint> Queue;
		Queue.Add(Start);
		Parent.Add(Start, Start);
		for (int32 Head = 0; Head < Queue.Num(); ++Head)
		{
			const FIntPoint C = Queue[Head];
			if (C == Target)
			{
				return true;
			}
			for (const FIntPoint& D : Dirs)
			{
				const FIntPoint N = C + D;
				if (L.IsValidCell(N) && !Parent.Contains(N) && !L.HasWallBetween(C, D) && !Maze->GetPitCells().Contains(N)
					&& !Maze->IsTunnelCell(N) // es demasiado grande para los túneles bajos
					&& !(bUseFire && Fire.Contains(N)))
				{
					Parent.Add(N, C);
					Queue.Add(N);
				}
			}
		}
		return false;
	};

	TMap<FIntPoint, FIntPoint> Parent;
	bool bFound = Bfs(Fire.Num() > 0, Parent);
	if (!bFound && Fire.Num() > 0)
	{
		Parent.Reset();
		bFound = Bfs(false, Parent); // no hay otro camino: igual va (y el fuego lo frenará en el lugar)
	}

	Path.Reset();
	if (!bFound)
	{
		return;
	}
	for (FIntPoint C = Target; C != Start; C = Parent[C])
	{
		Path.Insert(C, 0);
	}
	GoalCell = Target;
}

void ALabMinotaur::PickWanderTarget()
{
	const ALabMaze* Maze = GetMaze();
	if (Maze)
	{
		const FLabMazeLayout& L = Maze->GetLayout();
		RepathTo(FIntPoint(FMath::RandRange(0, L.Width - 1), FMath::RandRange(0, L.Height - 1)), true);
	}
}

ALabLantern* ALabMinotaur::FindScaryLantern(const FVector& GoalDir) const
{
	const ULabMinotaurSettings* S = GetSettings();
	for (TActorIterator<ALabLantern> It(GetWorld()); It; ++It)
	{
		if (!It->IsLit())
		{
			continue;
		}
		const FVector To = It->GetActorLocation() - GetActorLocation();
		if (To.Size2D() < S->FireFearRadius && FVector::DotProduct(GoalDir, To.GetSafeNormal2D()) > S->FireAheadDot)
		{
			return *It;
		}
	}
	return nullptr;
}

void ALabMinotaur::ServerThink()
{
	const ULabMinotaurSettings* S = GetSettings();
	const float Now = GetWorld()->GetTimeSeconds();
	if (bChasing && Now - NoiseTime > S->NoiseMemory && Path.Num() == 0)
	{
		bChasing = false;
		UE_LOG(LogLabrynn, Log, TEXT("Minotauro: perdió el rastro, deambula"));
	}
	if (Path.Num() == 0 && !bChasing)
	{
		PickWanderTarget();
	}
	GetCharacterMovement()->MaxWalkSpeed = bRepelled ? S->WanderSpeed : (bChasing ? S->ChaseSpeed : S->WanderSpeed);

	if (Now >= NextLog)
	{
		NextLog = Now + 2.f;
		UE_LOG(LogLabrynn, Log, TEXT("Minotauro: celda=%s persigue=%d repelido=%d pasos=%d vel=%.0f"),
			*GetMaze()->WorldToCell(GetActorLocation()).ToString(), bChasing ? 1 : 0, bRepelled ? 1 : 0, Path.Num(), GetVelocity().Size2D());
	}
}

void ALabMinotaur::TryAttack()
{
	const ULabMinotaurSettings* S = GetSettings();
	for (TActorIterator<ALabCharacter> It(GetWorld()); It; ++It)
	{
		ALabCharacter* Ch = *It;
		if (!Ch->IsDetectableByMinotaur()) // escondido sin farol: no lo ve
		{
			continue;
		}
		const FVector To = Ch->GetActorLocation() - GetActorLocation();
		if (To.Size2D() <= S->AttackRange && FMath::Abs(To.Z) < 150.f)
		{
			const FVector Push = To.GetSafeNormal2D() * 600.f + FVector(0.f, 0.f, 450.f);
			Ch->AddWound(S->AttackWound, Push);
			Ch->KnockDown(Push);
			StunnedUntil = GetWorld()->GetTimeSeconds() + S->StunAfterHit;
			Path.Reset();
			UE_LOG(LogLabrynn, Log, TEXT("Minotauro: ¡atrapó a %s! (aturdido %.1f s)"), *Ch->GetName(), S->StunAfterHit);
			MulticastSound(RoarSound);
			return;
		}
	}
}

void ALabMinotaur::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Now = GetWorld()->GetTimeSeconds();
	const ULabMinotaurSettings* S = GetSettings();

	if (HasAuthority())
	{
		if (Now < StunnedUntil)
		{
			return;
		}
		if (Now >= NextThink)
		{
			NextThink = Now + 0.25f;
			ServerThink();
		}

		const ALabMaze* Maze = GetMaze();
		// ¿Hacia dónde quiere ir?
		FVector Goal = FVector::ZeroVector;
		bool bHasGoal = false;
		if (Maze && Path.Num() > 0)
		{
			Goal = Maze->CellToWorld(Path[0]);
			bHasGoal = true;
			if (FVector::Dist2D(Goal, GetActorLocation()) < 70.f)
			{
				Path.RemoveAt(0);
			}
		}
		else if (bChasing)
		{
			float Best = 450.f;
			for (TActorIterator<ALabCharacter> It(GetWorld()); It; ++It)
			{
				const float D = FVector::Dist2D(It->GetActorLocation(), GetActorLocation());
				if (It->IsDetectableByMinotaur() && D < Best)
				{
					Best = D;
					Goal = It->GetActorLocation();
					bHasGoal = true;
				}
			}
		}
		const FVector GoalDir = bHasGoal ? (Goal - GetActorLocation()).GetSafeNormal2D() : GetActorForwardVector();

		// Miedo al fuego: frena, retrocede y después busca otro camino.
		if (Now < RetreatUntil)
		{
			AddMovementInput(RetreatDir, 1.f);
			if (ALabLantern* L = RepellingLantern.Get())
			{
				L->SetRepelling(0.3f, S->RepelOilDrainMultiplier); // se renueva mientras dura el retroceso
			}
			return;
		}
		if (bRepelled)
		{
			bRepelled = false;
			if (Maze && GoalCell.X >= 0)
			{
				RepathTo(GoalCell, /*bAvoidFire*/ true);
				UE_LOG(LogLabrynn, Log, TEXT("Minotauro: busca otro camino (%d pasos)"), Path.Num());
			}
		}
		if (ALabLantern* Scary = FindScaryLantern(GoalDir))
		{
			bRepelled = true;
			RepellingLantern = Scary;
			RetreatUntil = Now + S->RetreatSeconds;
			RetreatDir = (GetActorLocation() - Scary->GetActorLocation()).GetSafeNormal2D();
			Scary->SetRepelling(S->RetreatSeconds, S->RepelOilDrainMultiplier);
			Path.Reset();
			UE_LOG(LogLabrynn, Log, TEXT("Minotauro: ¡el fuego de %s lo frena! retrocede %.0f s"), *GetNameSafe(Scary->GetHolder()), S->RetreatSeconds);
			MulticastSound(FireFearSound);
			return;
		}

		TryAttack();
		if (bHasGoal)
		{
			AddMovementInput(GoalDir, 1.f);
		}
	}

	// Telegrafía (todas las máquinas): pasos pesados y resoplidos 3D.
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (GetVelocity().Size2D() > 50.f && Now >= NextFootstep)
		{
			NextFootstep = Now + (bChasing ? 0.38f : 0.6f);
			if (FootstepSound) { UGameplayStatics::PlaySoundAtLocation(this, FootstepSound, GetActorLocation()); }
		}
		if (Now >= NextSnort)
		{
			NextSnort = Now + FMath::FRandRange(4.f, 8.f);
			if (SnortSound) { UGameplayStatics::PlaySoundAtLocation(this, SnortSound, GetActorLocation()); }
		}
		EyesGlow->SetIntensity(bChasing ? 1600.f : 700.f);
	}
}
