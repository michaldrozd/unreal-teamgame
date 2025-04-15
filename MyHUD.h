// Fill this file with the following content:
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MyHUD.generated.h"

// Povieme pocitacu, ze tato vec (widget) existuje, detaily su inde
class UMyHUDWidget;

/**
 * Hlavna trieda pre HUD (zobrazenie na obrazovke). Vytvara a drzi si widget (UMG).
 */
UCLASS()
class AMyHUD : public AHUD
{
	GENERATED_BODY()

public:
	AMyHUD();

	// Volane inymi castami kodu (PlayerState, GameState, Character) na zmenu zobrazenych statistik/sprav
	void UpdateKillCount(int32 Kills);
	void UpdateDeathCount(int32 Deaths);
	void UpdateHealth(float CurrentHealth, float MaxHealth); // Pridane pre zdravie
	void UpdateKillFeed(const TArray<FString>& Messages);

protected:
	// Volane kratko po starte hry alebo ked sa vytvori HUD.
	virtual void BeginPlay() override;

	// Trieda UMG widgetu, ktory sa ma vytvorit. Nastav ju v editore Blueprintov.
	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<UMyHUDWidget> HUDWidgetClass;

	// Vytvoreny widget.
	UPROPERTY() // Pamataj si vytvoreny widget
	TObjectPtr<UMyHUDWidget> HUDWidget;
};
