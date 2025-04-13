// Fill this file with the following content:
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyGameMode.generated.h"

/**
 * Urcuje pravidla hry, riesi pripajanie/odpajanie hracov, skore a ozivovanie.
 * Existuje IBA na serveri (pocitaci, ktory riadi hru).
 */
UCLASS()
class AMyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMyGameMode();

	// Zavola sa, ked je zabita postavicka ovladana hracom
	// KillerController (ovladac zabijaka) moze byt prazdny (napr. smrt padom)
	void PlayerKilled(AController* VictimController, AController* KillerController);

	// Upravene, aby riesilo odpojenie hraca
	virtual void Logout(AController* Exiting) override;

protected:
    // Upravene, aby sa dalo vybrat miesto narodenia (spawn point)
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	// Funkcia, ktora riesi samotne ozivenie
	void RespawnPlayer(AController* PlayerController);

    // Casovac pre oneskorenie ozivenia
    // Pouzivame TMap, aby sme mohli mat viac casovacov naraz (pre viac hracov)
    TMap<AController*, FTimerHandle> RespawnTimerHandles;

    // Oneskorenie ozivenia v sekundach
    UPROPERTY(EditDefaultsOnly, Category="Game Rules")
    float RespawnDelay = 5.0f;
};
