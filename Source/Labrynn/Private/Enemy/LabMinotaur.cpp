#include "Enemy/LabMinotaur.h"

#include "Enemy/LabMinotaurController.h"
#include "Player/LabCharacter.h"
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

	// Cuerpo: el maniquí de la plantilla, más grande. (Luis puede cambiar el mesh en un BP hijo.)
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
	GetCharacterMovement()->MaxWalkSpeed = WanderSpeed;
	bUseControllerRotationYaw = false;
}

void ALabMinotaur::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALabMinotaur, bChasing);
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
	if (!HasAuthority() || Strength < MinNoiseStrength)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	// Prefiere el ruido más fuerte y reciente; uno viejo cede ante cualquiera nuevo.
	const bool bOld = Now - NoiseTime > 3.f;
	if (bOld || Strength >= NoiseStrength * 0.7f)
	{
		const bool bWasChasing = bChasing;
		NoiseLocation = Location;
		NoiseStrength = Strength;
		NoiseTime = Now;
		bChasing = true;
		UE_LOG(LogLabrynn, Log, TEXT("Minotauro: oyó '%s' (%.2f) en %s"), *Tag.ToString(), Strength, *Location.ToCompactString());
		if (!bWasChasing)
		{
			MulticastRoar();
		}
		if (const ALabMaze* Maze = GetMaze())
		{
			RepathTo(Maze->WorldToCell(Location));
		}
	}
}

void ALabMinotaur::MulticastRoar_Implementation()
{
	if (RoarSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, RoarSound, GetActorLocation());
	}
}

void ALabMinotaur::RepathTo(const FIntPoint& RawTarget)
{
	const ALabMaze* Maze = GetMaze();
	if (!Maze)
	{
		return;
	}
	const FLabMazeLayout& L = Maze->GetLayout();
	// Objetivos fuera de la grilla (halls): a la celda válida más cercana.
	const FIntPoint Target(FMath::Clamp(RawTarget.X, 0, L.Width - 1), FMath::Clamp(RawTarget.Y, 0, L.Height - 1));
	FIntPoint Start = Maze->WorldToCell(GetActorLocation());
	Start = FIntPoint(FMath::Clamp(Start.X, 0, L.Width - 1), FMath::Clamp(Start.Y, 0, L.Height - 1));

	// BFS sobre la grilla (paredes y fosos bloquean).
	static const FIntPoint Dirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
	TMap<FIntPoint, FIntPoint> Parent;
	TArray<FIntPoint> Queue;
	Queue.Add(Start);
	Parent.Add(Start, Start);
	for (int32 Head = 0; Head < Queue.Num(); ++Head)
	{
		const FIntPoint C = Queue[Head];
		if (C == Target)
		{
			break;
		}
		for (const FIntPoint& D : Dirs)
		{
			const FIntPoint N = C + D;
			if (L.IsValidCell(N) && !Parent.Contains(N) && !L.HasWallBetween(C, D) && !Maze->GetPitCells().Contains(N))
			{
				Parent.Add(N, C);
				Queue.Add(N);
			}
		}
	}

	Path.Reset();
	if (!Parent.Contains(Target))
	{
		return; // inalcanzable (ej. dentro de un foso)
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
	if (!Maze)
	{
		return;
	}
	const FLabMazeLayout& L = Maze->GetLayout();
	RepathTo(FIntPoint(FMath::RandRange(0, L.Width - 1), FMath::RandRange(0, L.Height - 1)));
}

void ALabMinotaur::ServerThink()
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (bChasing && Now - NoiseTime > NoiseMemory && Path.Num() == 0)
	{
		bChasing = false; // se le perdió el rastro
		UE_LOG(LogLabrynn, Log, TEXT("Minotauro: perdió el rastro, deambula"));
	}
	if (Path.Num() == 0)
	{
		if (bChasing)
		{
			// Llegó adonde sonó: si hay alguien cerca y en la misma celda o vecina, va directo.
			return;
		}
		PickWanderTarget();
	}
	GetCharacterMovement()->MaxWalkSpeed = bChasing ? ChaseSpeed : WanderSpeed;

	if (Now >= NextLog)
	{
		NextLog = Now + 2.f;
		UE_LOG(LogLabrynn, Log, TEXT("Minotauro: celda=%s persigue=%d pasos=%d vel=%.0f"),
			*GetMaze()->WorldToCell(GetActorLocation()).ToString(), bChasing ? 1 : 0, Path.Num(), GetVelocity().Size2D());
	}
}

void ALabMinotaur::TryAttack()
{
	for (TActorIterator<ALabCharacter> It(GetWorld()); It; ++It)
	{
		ALabCharacter* Ch = *It;
		if (!Ch->IsAliveAndFree())
		{
			continue;
		}
		const FVector To = Ch->GetActorLocation() - GetActorLocation();
		if (To.Size2D() <= AttackRange && FMath::Abs(To.Z) < 150.f)
		{
			const FVector Push = To.GetSafeNormal2D() * 600.f + FVector(0.f, 0.f, 450.f);
			Ch->AddWound(AttackWound, Push);
			Ch->KnockDown(Push); // siempre lo tira (si AddWound no lo tiró ya)
			StunnedUntil = GetWorld()->GetTimeSeconds() + StunAfterHit;
			Path.Reset();
			UE_LOG(LogLabrynn, Log, TEXT("Minotauro: ¡atrapó a %s! (aturdido %.1f s)"), *Ch->GetName(), StunAfterHit);
			MulticastRoar();
			return;
		}
	}
}

void ALabMinotaur::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Now = GetWorld()->GetTimeSeconds();

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
		TryAttack();

		// Moverse hacia la próxima celda del camino; al final de una persecución, directo al jugador más cercano.
		if (const ALabMaze* Maze = GetMaze())
		{
			FVector Goal;
			bool bHasGoal = false;
			if (Path.Num() > 0)
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
					if (It->IsAliveAndFree() && D < Best)
					{
						Best = D;
						Goal = It->GetActorLocation();
						bHasGoal = true;
					}
				}
			}
			if (bHasGoal)
			{
				AddMovementInput((Goal - GetActorLocation()).GetSafeNormal2D(), 1.f);
			}
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
