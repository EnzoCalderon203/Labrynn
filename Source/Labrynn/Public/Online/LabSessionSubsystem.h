#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "LabSessionSubsystem.generated.h"

/** Resultado de búsqueda simplificado para la UI. */
USTRUCT(BlueprintType)
struct FLabSessionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Lab|Online")
	FString HostName;

	UPROPERTY(BlueprintReadOnly, Category = "Lab|Online")
	int32 OpenSlots = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Lab|Online")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Lab|Online")
	int32 PingMs = 0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FLabOnStatus, const FString& /*Mensaje*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FLabOnFindComplete, const TArray<FLabSessionInfo>& /*Resultados*/);

/**
 * Crear / buscar / unirse a partidas.
 * - Con Steam (AppID 480 en desarrollo): lobbies de Steam, invitaciones del overlay, NAT traversal y relay.
 * - Sin Steam (-nosteam o Steam cerrado): OnlineSubsystem NULL en LAN. Sirve para probar en una sola PC.
 * Como el AppID 480 es compartido por miles de juegos, filtramos por una clave propia (LABRYNN_BUILD).
 * Siempre se puede usar como fallback "open <ip>" (puerto 7777).
 */
UCLASS()
class LABRYNN_API ULabSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Crea la sesión y viaja al mapa del laberinto como listen server. */
	UFUNCTION(BlueprintCallable, Category = "Lab|Online")
	void HostGame(int32 MaxPlayers = 4);

	/** Busca partidas de Labrynn. Resultado en OnFindComplete. */
	UFUNCTION(BlueprintCallable, Category = "Lab|Online")
	void FindGames();

	/** Se une al resultado Index de la última búsqueda. */
	UFUNCTION(BlueprintCallable, Category = "Lab|Online")
	void JoinGame(int32 Index);

	/** Fallback: conexión directa por IP (LAN, VPN tipo Tailscale, o puerto abierto). */
	UFUNCTION(BlueprintCallable, Category = "Lab|Online")
	void JoinByAddress(const FString& Address);

	/** Sale de la sesión y vuelve al menú. */
	UFUNCTION(BlueprintCallable, Category = "Lab|Online")
	void LeaveGame();

	/** "STEAM" o "NULL". */
	UFUNCTION(BlueprintPure, Category = "Lab|Online")
	FString GetOnlineServiceName() const;

	/** Nombre del jugador local según el servicio (nombre de Steam si hay). */
	UFUNCTION(BlueprintPure, Category = "Lab|Online")
	FString GetLocalPlayerName() const;

	bool IsBusy() const { return bBusy; }
	const TArray<FLabSessionInfo>& GetLastResults() const { return LastResults; }

	FLabOnStatus OnStatus;
	FLabOnFindComplete OnFindComplete;

	/** Mapa de juego y de menú. */
	static const TCHAR* GameMapURL;
	static const TCHAR* MenuMapURL;

private:
	IOnlineSessionPtr GetSessionInterface() const;
	bool IsLAN() const;
	void SetStatus(const FString& Msg);

	void OnCreateComplete(FName SessionName, bool bOk);
	void OnFindSessionsComplete(bool bOk);
	void OnJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroyComplete(FName SessionName, bool bOk);
	void OnInviteAccepted(bool bOk, int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& Invite);

	/** Sesión pendiente de crear tras destruir una vieja. */
	int32 PendingHostMaxPlayers = 0;
	/** Invitación pendiente de unirse tras destruir una vieja. */
	TOptional<FOnlineSessionSearchResult> PendingJoin;

	void JoinSearchResult(const FOnlineSessionSearchResult& Result);

	TSharedPtr<FOnlineSessionSearch> Search;
	TArray<FLabSessionInfo> LastResults;
	bool bBusy = false;

	FDelegateHandle CreateHandle, FindHandle, JoinHandle, DestroyHandle, InviteHandle;
};
