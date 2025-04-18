// Fill this file with the following content:
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h" // Potrebne pre posielanie informacii (replikaciu)
#include "MyPlayerState.generated.h"

/**
 * Uchovava informacie specificke pre hraca, ktore zostavaju aj ked zmeni postavicku
 * a posielaju sa vsetkym hracom (ako skore, meno, atd.).
 */
UCLASS()
class AMyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	// Konstruktor pre PlayerState.
    AMyPlayerState();

	// Volane u klientov, ked sa zmeni standardna premenna Score.
	virtual void OnRep_Score() override; // Priklad prepisania, ak pouzivas zakladne Skore
	// virtual void OnRep_PlayerName() override; // Priklad prepisania

	// Vlastne statistiky
	UPROPERTY(VisibleAnywhere, Category = "Stats", ReplicatedUsing = OnRep_Kills)
	int32 Kills;

	UPROPERTY(VisibleAnywhere, Category = "Stats", ReplicatedUsing = OnRep_Deaths)
	int32 Deaths;

	// Funkcie, ktore sa spustia u klientov, ked sa zmenia vlastne statistiky
	// Volane u klientov, ked sa zmeni premenna Kills.
	UFUNCTION()
	void OnRep_Kills();

	// Volane u klientov, ked sa zmeni premenna Deaths.
	UFUNCTION()
	void OnRep_Deaths();

	// Funkcie volane hernym modom na SERVERI na zmenu statistik
	// Prida jedno zabitie hracovi. Volane iba na serveri.
	void AddKill();
	// Prida jednu smrt hracovi. Volane iba na serveri.
	void AddDeath();

	// Nastavenie posielania informacii ostatnym hracom
	// Definuje, ktore premenne sa maju posielat (replikovat) cez siet.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Pomocna funkcia na pripravenie HUDu na strane klienta
	// UFUNCTION(Client, Reliable) // Pouzi, ak potrebujes RPC (specialne volanie), inak staci priame volanie z OnRep
	// void ClientInitializeHUD();

private:
	// Pomocna funkcia na aktualizaciu HUDu (volana na serveri a u klientov cez OnRep)
	// Aktualizuje HUD lokalneho hraca s aktualnymi statistikami (Kills, Deaths).
	void UpdateHUD();
};
