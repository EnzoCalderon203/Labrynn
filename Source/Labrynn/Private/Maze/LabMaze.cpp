#include "Maze/LabMaze.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
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
		// Movable: en los clientes el laberinto se construye después de BeginPlay, y
		// SetStaticMesh falla en componentes Static ya registrados. (Sin luz estática no hay costo extra.)
		C->SetMobility(EComponentMobility::Movable);
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
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// Exposición fija: sin esto la auto-exposición "aclara" la oscuridad y arruina el juego.
	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("DarknessPostProcess"));
	PostProcess->SetupAttachment(Root);
	PostProcess->bUnbound = true;
	PostProcess->Priority = -1.f; // un volumen del mapa (Luis) puede pisarlo

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
		C->SetStaticMesh(BlockMesh);
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

	// Piso y techo de la grilla: una sola instancia grande cada uno.
	AddBox(Floors, FVector(W * 0.5f, H * 0.5f, -FloorThickness * 0.5f), FVector(W, H, FloorThickness));
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
