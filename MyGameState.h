// Fill this file with the following content:
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h" // Required for replication
#include "MyGameState.generated.h"

/**
 * Uchovava informacie o celej hre, ktore vidia vsetci hraci (napr. casovac hry, kto koho zabil).
 */
UCLASS()
class AMyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	// Spravy o zabitiach (kto koho)
	UPROPERTY(VisibleAnywhere, Category = "Gameplay", ReplicatedUsing = OnRep_KillFeed)
	TArray<FString> KillFeedMessages;

	// Funkcia, ktora sa spusti u klientov, ked sa zmenia spravy o zabitiach
	UFUNCTION()
	void OnRep_KillFeed();

	// Funkcia volana hernym modom na SERVERI na pridanie spravy
	void AddKillFeedMessage(const FString& Message);

	// Nastavenie posielania informacii ostatnym hracom
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    // Pomocna funkcia na aktualizaciu obrazovky (volana na serveri a u klientov)
    void UpdateHUDKillFeed();

    // Maximalny pocet sprav, ktore sa zobrazia naraz
    const int32 MaxFeedMessages = 5;
};
