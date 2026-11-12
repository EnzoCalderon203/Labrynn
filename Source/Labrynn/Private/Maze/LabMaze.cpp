#include "Maze/LabMaze.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Maze/LabVisualSettings.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Labrynn.h"

namespace
{
	UInstancedStaticMeshComponent* MakeISM(AActor* Owner, USceneComponent* Parent, FName Name)
	{
		UInstancedStaticMeshComponent* C = Owner->CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		C->SetupAttachment(Parent);
		// STATIC a propósito: si el piso fuera Movable, el CharacterMovement lo trata como "base dinámica" y el
		// servidor manda las posiciones relativas al ALabMaze, que NO se replica (cada máquina tiene el suyo).
		// El cliente no puede resolver esa base e ignora TODAS las correcciones (bug encontrado con -LabAutoRun).
		// El mesh se asigna en el constructor (SetStaticMesh falla en Static ya registrados).
		C->SetMobility(EComponentMobility::Static);
		C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		C->SetCanEverAffectNavigation(false);
		return C;
	}
}

ALabMaze::ALabMaze()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false; // se construye localmente en cada máquina

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	RootComponent = Root;

	// Exposición fija: sin esto la auto-exposición "aclara" la oscuridad y arruina el juego.
	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("DarknessPostProcess"));
	PostProcess->SetupAttachment(Root);
	PostProcess->bUnbound = true;
	PostProcess->Priority = -1.f; // un volumen del mapa (Luis) puede pisarlo

	Moss = MakeISM(this, Root, TEXT("Moss"));
	Moss->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Moss->CastShadow = false;
	Cracks = MakeISM(this, Root, TEXT("Cracks"));
	Cracks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Cracks->CastShadow = false;

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(Root);

	static ConstructorHelpers::FObjectFinder<ULabVisualSettings> VisDA(TEXT("/Game/Labrynn/Core/DA_Visuals.DA_Visuals"));
	VisualSettings = VisDA.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMat(TEXT("/Game/Labrynn/Core/Materials/M_Glow.M_Glow"));
	GlowMaterial = GlowMat.Object;

	Walls = MakeISM(this, Root, TEXT("Walls"));
	Floors = MakeISM(this, Root, TEXT("Floors"));
	Ceilings = MakeISM(this, Root, TEXT("Ceilings"));
	ExitDoor = MakeISM(this, Root, TEXT("ExitDoor"));
	ExitDoor->SetMobility(EComponentMobility::Movable); // se quita en runtime

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		BlockMesh = CubeFinder.Object;
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (MatFinder.Succeeded())
	{
		WallMaterial = MatFinder.Object;
		FloorMaterial = MatFinder.Object;
		CeilingMaterial = MatFinder.Object;
	}

	// Materiales de grilla (LevelPrototyping, contenido de la plantilla): ayudan a percibir distancia.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallGrid(TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray_02.MI_PrototypeGrid_Gray_02"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FloorGrid(TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark.MI_PrototypeGrid_TopDark"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CeilGrid(TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray.MI_PrototypeGrid_Gray"));
	if (WallGrid.Succeeded())  { WallMaterial = WallGrid.Object; }
	if (FloorGrid.Succeeded()) { FloorMaterial = FloorGrid.Object; }
	if (CeilGrid.Succeeded())  { CeilingMaterial = CeilGrid.Object; }

	// Mesh y materiales por defecto ya en el constructor (componentes Static).
	for (UInstancedStaticMeshComponent* C : { Walls.Get(), Floors.Get(), Ceilings.Get(), ExitDoor.Get(), Moss.Get(), Cracks.Get() })
	{
		C->SetStaticMesh(BlockMesh);
	}
	Walls->SetMaterial(0, WallMaterial);
	ExitDoor->SetMaterial(0, WallMaterial);
	Floors->SetMaterial(0, FloorMaterial);
	Ceilings->SetMaterial(0, CeilingMaterial);
}

void ALabMaze::BeginPlay()
{
	Super::BeginPlay();
}

void ALabMaze::Build(int32 InSeed)
{
	if (bBuilt)
	{
		return;
	}
	bBuilt = true;
	Seed = InSeed;
	Layout = FLabMazeLayout::Generate(Seed, Params);

	for (UInstancedStaticMeshComponent* C : { Walls.Get(), Floors.Get(), Ceilings.Get(), ExitDoor.Get() })
	{
		if (C->GetStaticMesh() != BlockMesh)
		{
			C->SetStaticMesh(BlockMesh); // solo si un Blueprint hijo cambió el mesh
		}
		C->ClearInstances();
	}
#if !UE_BUILD_SHIPPING
	FParse::Value(FCommandLine::Get(), TEXT("LabExposure="), ExposureBias); // calibración rápida
#endif
	FPostProcessSettings& PP = PostProcess->Settings;
	PP.bOverride_AutoExposureMethod = true;
	PP.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	PP.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	PP.AutoExposureApplyPhysicalCameraExposure = false;
	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = ExposureBias;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.6f;

	Walls->SetMaterial(0, WallMaterial);
	ExitDoor->SetMaterial(0, WallMaterial);
	Floors->SetMaterial(0, FloorMaterial);
	Ceilings->SetMaterial(0, CeilingMaterial);

	const float W = Layout.Width * CellSize;
	const float H = Layout.Height * CellSize;
	const float FloorThickness = 20.f;

	// Fosos: callejones sin salida elegidos con la seed (idéntico en todas las máquinas).
	// Solo en dead-ends: un foso abierto nunca corta el camino a la reliquia.
	PitCells.Reset();
	{
		FRandomStream PitRng(Seed * 7919 + 13);
		TArray<FIntPoint> Candidates = Layout.FindDeadEnds();
		Candidates.RemoveAll([this](const FIntPoint& C)
		{
			return FMath::Abs(C.X - Layout.Entrance.X) + FMath::Abs(C.Y - Layout.Entrance.Y) <= 2 || C == Layout.Exit;
		});
		for (int32 i = Candidates.Num() - 1; i > 0; --i)
		{
			Candidates.Swap(i, PitRng.RandRange(0, i));
		}
		for (int32 i = 0; i < FMath::Min(PitCount, Candidates.Num()); ++i)
		{
			PitCells.Add(Candidates[i]);
		}
	}

	// Piso por celda (con huecos en los fosos) y techo en una sola instancia.
	for (int32 Y = 0; Y < Layout.Height; ++Y)
	{
		for (int32 X = 0; X < Layout.Width; ++X)
		{
			if (!PitCells.Contains(FIntPoint(X, Y)))
			{
				AddBox(Floors, FVector((X + 0.5f) * CellSize, (Y + 0.5f) * CellSize, -FloorThickness * 0.5f),
					FVector(CellSize + 1.f, CellSize + 1.f, FloorThickness));
			}
		}
	}
	for (const FIntPoint& P : PitCells)
	{
		// Fondo y paredes del foso.
		const FVector C((P.X + 0.5f) * CellSize, (P.Y + 0.5f) * CellSize, -PitDepth);
		AddBox(Floors, C - FVector(0, 0, FloorThickness * 0.5f), FVector(CellSize, CellSize, FloorThickness));
		const float Half = CellSize * 0.5f;
		const float WallZ = -PitDepth * 0.5f;
		AddBox(Walls, FVector(C.X - Half, C.Y, WallZ), FVector(WallThickness, CellSize, PitDepth));
		AddBox(Walls, FVector(C.X + Half, C.Y, WallZ), FVector(WallThickness, CellSize, PitDepth));
		AddBox(Walls, FVector(C.X, C.Y - Half, WallZ), FVector(CellSize, WallThickness, PitDepth));
		AddBox(Walls, FVector(C.X, C.Y + Half, WallZ), FVector(CellSize, WallThickness, PitDepth));
	}
	AddBox(Ceilings, FVector(W * 0.5f, H * 0.5f, WallHeight + FloorThickness * 0.5f), FVector(W, H, FloorThickness));

	// Paredes horizontales (lado -Y de cada celda, más la fila superior).
	for (int32 Y = 0; Y <= Layout.Height; ++Y)
	{
		for (int32 X = 0; X < Layout.Width; ++X)
		{
			if (Layout.HasHWall(X, Y))
			{
				AddWallH(X, Y);
			}
		}
	}

	// Paredes verticales.
	for (int32 Y = 0; Y < Layout.Height; ++Y)
	{
		for (int32 X = 0; X <= Layout.Width; ++X)
		{
			if (Layout.HasVWall(X, Y))
			{
				AddWallV(X, Y);
			}
		}
	}

	BuildEntranceHall();

	BuildAmbience();

	// Luces tenues: hall de entrada (para orientarse al empezar) y sala central (brillo de la reliquia).
	AddGuideLight(FVector((Layout.Entrance.X + 0.5f) * CellSize, -EntranceHallDepth * CellSize * 0.5f, WallHeight - 60.f),
		FLinearColor(1.f, 0.75f, 0.45f), EntranceLightIntensity);
	AddGuideLight(GetActorTransform().InverseTransformPosition(GetCenterRoomLocation()) + FVector(0.f, 0.f, 120.f),
		FLinearColor(0.35f, 0.8f, 1.f), CenterLightIntensity);

	UE_LOG(LogLabrynn, Log, TEXT("[%s] Laberinto construido: seed=%d %dx%d checksum=%08X entrada=(%d,%d) salida=(%d,%d)"),
		*GetNameSafe(GetWorld()), Seed, Layout.Width, Layout.Height, Layout.ComputeChecksum(),
		Layout.Entrance.X, Layout.Entrance.Y, Layout.Exit.X, Layout.Exit.Y);
}

void ALabMaze::AddGuideLight(const FVector& LocalPos, const FLinearColor& Color, float Intensity)
{
	UPointLightComponent* L = NewObject<UPointLightComponent>(this);
	L->SetupAttachment(Root);
	L->SetMobility(EComponentMobility::Movable);
	L->SetRelativeLocation(LocalPos);
	L->SetLightColor(Color);
	L->SetIntensity(Intensity);
	L->SetAttenuationRadius(CellSize * 2.5f);
	L->SetCastShadows(false);
	L->RegisterComponent();
	AddInstanceComponent(L);
}

void ALabMaze::AddBox(UInstancedStaticMeshComponent* Comp, const FVector& Center, const FVector& SizeCm)
{
	// El cubo de BasicShapes mide 100 cm con pivote centrado.
	Comp->AddInstance(FTransform(FRotator::ZeroRotator, Center, SizeCm / 100.f), /*bWorldSpace*/ false);
}

void ALabMaze::AddWallH(int32 X, int32 Y)
{
	const FVector Center((X + 0.5f) * CellSize, Y * CellSize, WallHeight * 0.5f);
	const FVector Size(CellSize + WallThickness, WallThickness, WallHeight);

	// La pared norte de la celda de salida va en un componente aparte.
	const bool bIsExitWall = (Y == Layout.Height && X == Layout.Exit.X);
	AddBox(bIsExitWall ? ExitDoor.Get() : Walls.Get(), Center, Size);
}

void ALabMaze::AddWallV(int32 X, int32 Y)
{
	const FVector Center(X * CellSize, (Y + 0.5f) * CellSize, WallHeight * 0.5f);
	const FVector Size(WallThickness, CellSize + WallThickness, WallHeight);
	AddBox(Walls, Center, Size);
}

void ALabMaze::BuildEntranceHall()
{
	BuildHall(Layout.Entrance.X, /*bNorth*/ false);
	BuildHall(Layout.Exit.X, /*bNorth*/ true);

	// Mesa de faroles contra la pared del fondo del hall de entrada.
	const FVector TableCenter = TableLocalCenter();
	AddBox(Walls, TableCenter - FVector(0.f, 0.f, TableHeight * 0.5f), FVector(TableWidth, 70.f, TableHeight));

	// Baliza de la salida: se prende cuando se abre (fase de escape).
	const FVector BeaconPos((Layout.Exit.X + 0.5f) * CellSize, (Layout.Height + EntranceHallDepth * 0.5f) * CellSize, WallHeight - 60.f);
	ExitBeacon = NewObject<UPointLightComponent>(this);
	ExitBeacon->SetupAttachment(Root);
	ExitBeacon->SetMobility(EComponentMobility::Movable);
	ExitBeacon->SetRelativeLocation(BeaconPos);
	ExitBeacon->SetLightColor(FLinearColor(1.f, 0.15f, 0.08f));
	ExitBeacon->SetIntensity(ExitBeaconIntensity);
	ExitBeacon->SetAttenuationRadius(CellSize * 3.5f);
	ExitBeacon->SetCastShadows(false);
	ExitBeacon->SetVisibility(false);
	ExitBeacon->RegisterComponent();
	AddInstanceComponent(ExitBeacon);
}

void ALabMaze::BuildHall(int32 CellX, bool bNorth)
{
	// Hall rectangular fuera de la grilla (sur: Y<0; norte: Y>Height), abierto hacia la celda CellX.
	const int32 HalfW = EntranceHallWidth / 2;
	const float XMin = (CellX - HalfW) * CellSize;
	const float XMax = (CellX + HalfW + 1) * CellSize;
	const float YMin = bNorth ? Layout.Height * CellSize : -EntranceHallDepth * CellSize;
	const float YMax = bNorth ? (Layout.Height + EntranceHallDepth) * CellSize : 0.f;
	const float YFar = bNorth ? YMax : YMin;
	const float HallW = XMax - XMin;
	const float HallD = YMax - YMin;
	const float FloorThickness = 20.f;

	AddBox(Floors, FVector((XMin + XMax) * 0.5f, (YMin + YMax) * 0.5f, -FloorThickness * 0.5f), FVector(HallW, HallD, FloorThickness));
	AddBox(Ceilings, FVector((XMin + XMax) * 0.5f, (YMin + YMax) * 0.5f, WallHeight + FloorThickness * 0.5f), FVector(HallW, HallD, FloorThickness));

	// Pared del fondo y laterales.
	AddBox(Walls, FVector((XMin + XMax) * 0.5f, YFar, WallHeight * 0.5f), FVector(HallW + WallThickness, WallThickness, WallHeight));
	AddBox(Walls, FVector(XMin, (YMin + YMax) * 0.5f, WallHeight * 0.5f), FVector(WallThickness, HallD + WallThickness, WallHeight));
	AddBox(Walls, FVector(XMax, (YMin + YMax) * 0.5f, WallHeight * 0.5f), FVector(WallThickness, HallD + WallThickness, WallHeight));

	// El borde sur del laberinto ya tiene paredes en Y=0 salvo en la entrada: el hall queda sellado.
}

FVector ALabMaze::CellToWorld(FIntPoint Cell) const
{
	return GetActorTransform().TransformPosition(FVector((Cell.X + 0.5f) * CellSize, (Cell.Y + 0.5f) * CellSize, 0.f));
}

FIntPoint ALabMaze::WorldToCell(const FVector& World) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(World);
	return FIntPoint(FMath::FloorToInt(Local.X / CellSize), FMath::FloorToInt(Local.Y / CellSize));
}

FTransform ALabMaze::GetSpawnTransform(int32 Index) const
{
	// Hasta 4 puestos en el hall, en fila, mirando hacia +Y (hacia la entrada).
	static const float Offsets[4] = { -0.6f, 0.6f, -1.8f, 1.8f };
	const float OffsetX = Offsets[Index % 4] * (CellSize * 0.4f);
	const float BackY = -EntranceHallDepth * CellSize * 0.5f - (Index / 4) * 80.f;
	const FVector Local((Layout.Entrance.X + 0.5f) * CellSize + OffsetX, BackY, 100.f);
	const FVector World = GetActorTransform().TransformPosition(Local);
	const FRotator Facing = GetActorTransform().TransformRotation(FRotator(0.f, 90.f, 0.f).Quaternion()).Rotator();
	return FTransform(Facing, World);
}

void ALabMaze::SetExitOpen(bool bOpen)
{
	ExitDoor->SetVisibility(!bOpen);
	ExitDoor->SetCollisionEnabled(bOpen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	if (ExitBeacon)
	{
		ExitBeacon->SetVisibility(bOpen);
	}
}

const ULabVisualSettings* ALabMaze::GetVisualSettings() const
{
	return VisualSettings ? VisualSettings.Get() : GetDefault<ULabVisualSettings>();
}

void ALabMaze::BuildAmbience()
{
	const ULabVisualSettings* V = GetVisualSettings();
	FRandomStream Rng(Seed * 977 + 41); // determinista: igual en todas las máquinas

	// Materiales emisivos (instancias dinámicas de M_Glow).
	if (GlowMaterial)
	{
		UMaterialInstanceDynamic* MossMID = UMaterialInstanceDynamic::Create(GlowMaterial, this);
		MossMID->SetVectorParameterValue(TEXT("EmissiveColor"), V->MossEmissive);
		Moss->SetMaterial(0, MossMID);
		UMaterialInstanceDynamic* CrackMID = UMaterialInstanceDynamic::Create(GlowMaterial, this);
		CrackMID->SetVectorParameterValue(TEXT("EmissiveColor"), V->CrackEmissive);
		Cracks->SetMaterial(0, CrackMID);
	}

	// Celdas candidatas (fuera de la sala central y sin foso), mezcladas con la seed.
	TArray<FIntPoint> Cells;
	for (int32 Y = 0; Y < Layout.Height; ++Y)
	{
		for (int32 X = 0; X < Layout.Width; ++X)
		{
			const FIntPoint C(X, Y);
			if (!Layout.IsInCenterRoom(C) && !PitCells.Contains(C))
			{
				Cells.Add(C);
			}
		}
	}
	for (int32 i = Cells.Num() - 1; i > 0; --i)
	{
		Cells.Swap(i, Rng.RandRange(0, i));
	}

	// Musgo: tiras finas al pie de las paredes de algunas celdas.
	static const FIntPoint Dirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
	const int32 NumMoss = FMath::Min(V->MossCellCount, Cells.Num());
	const float Inset = WallThickness * 0.5f + 3.f;
	for (int32 i = 0; i < NumMoss; ++i)
	{
		const FIntPoint C = Cells[i];
		const FVector Center((C.X + 0.5f) * CellSize, (C.Y + 0.5f) * CellSize, 5.f);
		for (const FIntPoint& D : Dirs)
		{
			if (Layout.HasWallBetween(C, D) && Rng.FRand() < 0.6f)
			{
				const FVector Off(D.X * (CellSize * 0.5f - Inset), D.Y * (CellSize * 0.5f - Inset), 0.f);
				const FVector Size = D.X != 0 ? FVector(6.f, CellSize * Rng.FRandRange(0.4f, 0.85f), 10.f) : FVector(CellSize * Rng.FRandRange(0.4f, 0.85f), 6.f, 10.f);
				AddBox(Moss, Center + Off, Size);
			}
		}
	}

	// Luna: grietas en el techo de algunas celdas, cada una con un haz frío hacia abajo.
	const int32 NumMoon = FMath::Min(V->MoonBeamCount, Cells.Num() - NumMoss);
	for (int32 i = 0; i < NumMoon; ++i)
	{
		const FIntPoint C = Cells[NumMoss + i];
		const FVector Top((C.X + 0.5f) * CellSize + Rng.FRandRange(-60.f, 60.f), (C.Y + 0.5f) * CellSize + Rng.FRandRange(-60.f, 60.f), WallHeight);
		AddBox(Cracks, Top, FVector(Rng.FRandRange(40.f, 90.f), Rng.FRandRange(6.f, 14.f), 4.f));

		USpotLightComponent* Beam = NewObject<USpotLightComponent>(this);
		Beam->SetupAttachment(Root);
		Beam->SetMobility(EComponentMobility::Movable);
		Beam->SetRelativeLocationAndRotation(Top - FVector(0.f, 0.f, 4.f), FRotator(-90.f, 0.f, 0.f));
		Beam->SetLightColor(V->MoonColor);
		Beam->SetIntensity(V->MoonIntensity);
		Beam->SetAttenuationRadius(WallHeight + 150.f);
		Beam->SetInnerConeAngle(V->MoonConeAngle * 0.4f);
		Beam->SetOuterConeAngle(V->MoonConeAngle);
		Beam->SetCastShadows(false);
		Beam->SetVolumetricScatteringIntensity(V->MoonVolumetricScattering);
		Beam->RegisterComponent();
		AddInstanceComponent(Beam);
	}

	// Niebla tenue: hace visibles los haces de luna (y el halo del farol).
	Fog->SetFogDensity(V->FogDensity);
	Fog->SetFogHeightFalloff(0.2f);
	Fog->SetFogInscatteringColor(FLinearColor(0.01f, 0.012f, 0.015f));
	Fog->SetVolumetricFog(V->bVolumetricFog);
	Fog->SetVolumetricFogScatteringDistribution(0.6f);
	Fog->SetVolumetricFogExtinctionScale(0.6f);

	UE_LOG(LogLabrynn, Log, TEXT("Ambiente: musgo en %d celdas (%d tiras), %d haces de luna"), NumMoss, Moss->GetInstanceCount(), NumMoon);
}

FVector ALabMaze::TableLocalCenter() const
{
	// Centro de la tapa de la mesa (local): al fondo del hall de entrada.
	return FVector((Layout.Entrance.X + 0.5f) * CellSize, -EntranceHallDepth * CellSize + 70.f, TableHeight);
}

FTransform ALabMaze::GetLanternTableSlot(int32 Index) const
{
	// Hasta 4 faroles en fila sobre la mesa; si hay más, segunda fila.
	const int32 Col = Index % 4;
	const int32 Row = Index / 4;
	const FVector Local = TableLocalCenter() + FVector(-TableWidth * 0.375f + Col * TableWidth * 0.25f, Row * 25.f, 1.f);
	return FTransform(GetActorTransform().TransformPosition(Local));
}

bool ALabMaze::IsInExitHall(const FVector& World) const
{
	const FIntPoint C = WorldToCell(World);
	return C.Y >= Layout.Height && FMath::Abs(C.X - Layout.Exit.X) <= EntranceHallWidth / 2;
}

FVector ALabMaze::GetCenterRoomLocation() const
{
	const FVector Local(
		(Layout.CenterMin.X + Layout.CenterMax.X + 1) * 0.5f * CellSize,
		(Layout.CenterMin.Y + Layout.CenterMax.Y + 1) * 0.5f * CellSize,
		0.f);
	return GetActorTransform().TransformPosition(Local);
}
