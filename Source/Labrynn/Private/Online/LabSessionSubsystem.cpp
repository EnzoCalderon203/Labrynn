#include "Online/LabSessionSubsystem.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "Labrynn.h"

const TCHAR* ULabSessionSubsystem::GameMapURL = TEXT("/Game/Labrynn/Maps/L_Labrynn");
const TCHAR* ULabSessionSubsystem::MenuMapURL = TEXT("/Game/Labrynn/Maps/L_Menu");

namespace
{
	/** Clave para distinguir nuestras sesiones de las de otros juegos que usan el AppID 480. Cambiar si cambia el protocolo. */
	const FName KeyBuild(TEXT("LABRYNN_BUILD"));
	const FString BuildValue(TEXT("labrynn-jam26-v1"));
}

void ULabSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (IOnlineSessionPtr S = GetSessionInterface())
	{
		CreateHandle = S->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateComplete));
		FindHandle = S->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete));
		JoinHandle = S->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinComplete));
		DestroyHandle = S->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroyComplete));
		InviteHandle = S->AddOnSessionUserInviteAcceptedDelegate_Handle(FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::OnInviteAccepted));
	}
	UE_LOG(LogLabrynn, Log, TEXT("Online: servicio=%s, LAN=%d, clave de búsqueda %s=%s"), *GetOnlineServiceName(), IsLAN() ? 1 : 0, *KeyBuild.ToString(), *BuildValue);
	if (GEngine)
	{
		// Errores de red y de viaje: al log y al menú (lo primero que hay que mirar si la prueba online falla).
		GEngine->OnNetworkFailure().AddWeakLambda(this, [this](UWorld*, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Msg)
		{
			SetStatus(FString::Printf(TEXT("Error de red: %s (%s)"), ENetworkFailure::ToString(Type), *Msg));
		});
		GEngine->OnTravelFailure().AddWeakLambda(this, [this](UWorld*, ETravelFailure::Type Type, const FString& Msg)
		{
			SetStatus(FString::Printf(TEXT("Error de viaje: %s (%s)"), ETravelFailure::ToString(Type), *Msg));
		});
	}
}

void ULabSessionSubsystem::Deinitialize()
{
	if (IOnlineSessionPtr S = GetSessionInterface())
	{
		S->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		S->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		S->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		S->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
		S->ClearOnSessionUserInviteAcceptedDelegate_Handle(InviteHandle);
	}
	Super::Deinitialize();
}

IOnlineSessionPtr ULabSessionSubsystem::GetSessionInterface() const
{
	const IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld());
	if (!OSS)
	{
		OSS = IOnlineSubsystem::Get();
	}
	return OSS ? OSS->GetSessionInterface() : nullptr;
}

FString ULabSessionSubsystem::GetOnlineServiceName() const
{
	const IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld());
	if (!OSS)
	{
		OSS = IOnlineSubsystem::Get();
	}
	return OSS ? OSS->GetSubsystemName().ToString() : TEXT("NINGUNO");
}

FString ULabSessionSubsystem::GetLocalPlayerName() const
{
	if (const ULocalPlayer* LP = GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr)
	{
		return LP->GetNickname();
	}
	return TEXT("Jugador");
}

bool ULabSessionSubsystem::IsLAN() const
{
	return GetOnlineServiceName() == TEXT("NULL");
}

void ULabSessionSubsystem::SetStatus(const FString& Msg)
{
	UE_LOG(LogLabrynn, Log, TEXT("Online: %s"), *Msg);
	OnStatus.Broadcast(Msg);
}

// ---------------------------------------------------------------- Host

void ULabSessionSubsystem::HostGame(int32 MaxPlayers)
{
	IOnlineSessionPtr S = GetSessionInterface();
	if (!S)
	{
		// Sin subsistema online: igual se puede jugar por IP.
		SetStatus(TEXT("Sin servicio online: abriendo partida por IP (puerto 7777)."));
		UGameplayStatics::OpenLevel(this, FName(GameMapURL), true, TEXT("listen"));
		return;
	}
	if (bBusy)
	{
		return;
	}
	bBusy = true;

	if (S->GetNamedSession(NAME_GameSession))
	{
		// Quedó una sesión vieja (por ejemplo, tras una desconexión): destruirla primero.
		PendingHostMaxPlayers = MaxPlayers;
		S->DestroySession(NAME_GameSession);
		return;
	}

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = FMath::Clamp(MaxPlayers, 1, 4);
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bIsLANMatch = IsLAN();
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.bAllowInvites = true;
	Settings.Set(KeyBuild, BuildValue, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	SetStatus(TEXT("Creando partida..."));
	const ULocalPlayer* LP = GetGameInstance()->GetFirstGamePlayer();
	if (!LP || !S->CreateSession(*LP->GetPreferredUniqueNetId(), NAME_GameSession, Settings))
	{
		bBusy = false;
		SetStatus(TEXT("No se pudo crear la partida."));
	}
}

void ULabSessionSubsystem::OnCreateComplete(FName SessionName, bool bOk)
{
	bBusy = false;
	if (!bOk)
	{
		SetStatus(TEXT("Falló la creación de la partida."));
		return;
	}
	SetStatus(TEXT("Partida creada. Cargando laberinto..."));
	UGameplayStatics::OpenLevel(this, FName(GameMapURL), true, TEXT("listen"));
}

// ---------------------------------------------------------------- Buscar

void ULabSessionSubsystem::FindGames()
{
	IOnlineSessionPtr S = GetSessionInterface();
	if (!S || bBusy)
	{
		return;
	}
	bBusy = true;

	Search = MakeShared<FOnlineSessionSearch>();
	Search->bIsLanQuery = IsLAN();
	Search->MaxSearchResults = 100;
	Search->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	Search->QuerySettings.Set(KeyBuild, BuildValue, EOnlineComparisonOp::Equals);

	SetStatus(TEXT("Buscando partidas..."));
	const ULocalPlayer* LP = GetGameInstance()->GetFirstGamePlayer();
	if (!LP || !S->FindSessions(*LP->GetPreferredUniqueNetId(), Search.ToSharedRef()))
	{
		bBusy = false;
		SetStatus(TEXT("No se pudo buscar."));
	}
}

void ULabSessionSubsystem::OnFindSessionsComplete(bool bOk)
{
	bBusy = false;
	LastResults.Reset();

	if (Search.IsValid())
	{
		// Filtro extra del lado del cliente (por si el servicio ignora la clave).
		const int32 Raw = Search->SearchResults.Num();
		Search->SearchResults.RemoveAll([](const FOnlineSessionSearchResult& R)
		{
			FString Value;
			return !R.IsValid() || !R.Session.SessionSettings.Get(KeyBuild, Value) || Value != BuildValue;
		});
		UE_LOG(LogLabrynn, Log, TEXT("Online: el servicio devolvió %d sesión(es); %d de otros juegos descartadas"), Raw, Raw - Search->SearchResults.Num());

		for (const FOnlineSessionSearchResult& R : Search->SearchResults)
		{
			FLabSessionInfo Info;
			Info.HostName = R.Session.OwningUserName;
			Info.MaxPlayers = R.Session.SessionSettings.NumPublicConnections;
			Info.OpenSlots = R.Session.NumOpenPublicConnections;
			Info.PingMs = R.PingInMs;
			LastResults.Add(Info);
		}
	}

	UE_LOG(LogLabrynn, Log, TEXT("Online: búsqueda ok=%d, %d resultado(s) de Labrynn"), bOk ? 1 : 0, LastResults.Num());
	for (const FLabSessionInfo& I : LastResults)
	{
		UE_LOG(LogLabrynn, Log, TEXT("Online:   - %s (%d/%d) %d ms"), *I.HostName, I.MaxPlayers - I.OpenSlots, I.MaxPlayers, I.PingMs);
	}
	SetStatus(bOk ? FString::Printf(TEXT("%d partida(s) encontrada(s)."), LastResults.Num()) : TEXT("La búsqueda falló."));
	OnFindComplete.Broadcast(LastResults);
}

// ---------------------------------------------------------------- Unirse

void ULabSessionSubsystem::JoinGame(int32 Index)
{
	if (!Search.IsValid() || !Search->SearchResults.IsValidIndex(Index))
	{
		SetStatus(TEXT("Esa partida ya no está en la lista."));
		return;
	}
	JoinSearchResult(Search->SearchResults[Index]);
}

void ULabSessionSubsystem::JoinSearchResult(const FOnlineSessionSearchResult& Result)
{
	IOnlineSessionPtr S = GetSessionInterface();
	if (!S || bBusy)
	{
		return;
	}
	bBusy = true;

	if (S->GetNamedSession(NAME_GameSession))
	{
		PendingJoin = Result;
		S->DestroySession(NAME_GameSession);
		return;
	}

	SetStatus(FString::Printf(TEXT("Uniéndose a %s..."), *Result.Session.OwningUserName));
	const ULocalPlayer* LP = GetGameInstance()->GetFirstGamePlayer();
	if (!LP || !S->JoinSession(*LP->GetPreferredUniqueNetId(), NAME_GameSession, Result))
	{
		bBusy = false;
		SetStatus(TEXT("No se pudo unir."));
	}
}

void ULabSessionSubsystem::OnJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	bBusy = false;
	IOnlineSessionPtr S = GetSessionInterface();
	FString Url;
	if (Result != EOnJoinSessionCompleteResult::Success || !S || !S->GetResolvedConnectString(SessionName, Url))
	{
		SetStatus(FString::Printf(TEXT("No se pudo unir (código %d)."), static_cast<int32>(Result)));
		return;
	}

	SetStatus(TEXT("Conectando..."));
	UE_LOG(LogLabrynn, Log, TEXT("Online: unido a la sesión, viajando a %s"), *Url);
	if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
	{
		PC->ClientTravel(Url, TRAVEL_Absolute);
	}
}

void ULabSessionSubsystem::OnInviteAccepted(bool bOk, int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& Invite)
{
	// Invitación aceptada desde el overlay de Steam ("Unirse a la partida").
	if (bOk && Invite.IsValid())
	{
		SetStatus(TEXT("Invitación aceptada."));
		bBusy = false;
		JoinSearchResult(Invite);
	}
}

void ULabSessionSubsystem::JoinByAddress(const FString& Address)
{
	const FString Trimmed = Address.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		return;
	}
	SetStatus(FString::Printf(TEXT("Conectando a %s..."), *Trimmed));
	if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
	{
		PC->ClientTravel(Trimmed, TRAVEL_Absolute);
	}
}

// ---------------------------------------------------------------- Salir

void ULabSessionSubsystem::LeaveGame()
{
	IOnlineSessionPtr S = GetSessionInterface();
	if (S && S->GetNamedSession(NAME_GameSession))
	{
		S->DestroySession(NAME_GameSession);
	}
	UGameplayStatics::OpenLevel(this, FName(MenuMapURL));
}

void ULabSessionSubsystem::OnDestroyComplete(FName SessionName, bool bOk)
{
	bBusy = false;
	if (PendingHostMaxPlayers > 0)
	{
		const int32 Max = PendingHostMaxPlayers;
		PendingHostMaxPlayers = 0;
		HostGame(Max);
	}
	else if (PendingJoin.IsSet())
	{
		const FOnlineSessionSearchResult R = PendingJoin.GetValue();
		PendingJoin.Reset();
		JoinSearchResult(R);
	}
}

// ---------------------------------------------------------------- Comandos de consola (pruebas)

namespace
{
	ULabSessionSubsystem* GetLabSessions(UWorld* World)
	{
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<ULabSessionSubsystem>() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs CmdHost(TEXT("Lab.Host"), TEXT("Crea una partida (Lab.Host [jugadores])"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (ULabSessionSubsystem* L = GetLabSessions(World)) { L->HostGame(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 4); }
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdFind(TEXT("Lab.Find"), TEXT("Busca partidas"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (ULabSessionSubsystem* L = GetLabSessions(World)) { L->FindGames(); }
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdJoin(TEXT("Lab.Join"), TEXT("Se une al resultado N de la última búsqueda (Lab.Join 0)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (ULabSessionSubsystem* L = GetLabSessions(World)) { L->JoinGame(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0); }
		}));
}
