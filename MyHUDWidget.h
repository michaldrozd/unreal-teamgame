// Fill this file with the following content:
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyHUDWidget.generated.h"

// Povieme pocitacu, ze TextBlock existuje, aby sme nemuseli nacitavat cely jeho kod
class UTextBlock;

/**
 * Zakladna C++ trieda pre nas UMG HUD Widget (to, co vidis na obrazovke).
 * Ukazuje funkcie, ktore moze volat Blueprint, a drzi si odkazy na textove polia.
 */
UCLASS()
class UMyHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Prepoj UMG textove polia vytvorene v editore s tymito premennymi
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KillCountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DeathCountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KillFeedText; // Uisti sa, ze toto meno sedi s menom premennej v UMG editore

	// Funkcie na zmenu textu, volatelne z MyHUD
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateKillCount(int32 Kills);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateDeathCount(int32 Deaths);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateKillFeed(const TArray<FString>& Messages);

protected:
	// Volane, ked sa widget vytvori. Dobre miesto pre pociatocne nastavenia.
	virtual void NativeConstruct() override;
};
