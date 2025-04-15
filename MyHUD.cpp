// Fill this file with the following content:
#include "MyHUD.h"
#include "Blueprint/UserWidget.h" // Pre funkciu CreateWidget (vytvorenie widgetu)
#include "MyHUDWidget.h" // Nacitaj kod pre samotny widget

// Konstruktor pre HUD triedu.
AMyHUD::AMyHUD()
{
    // Konstruktor moze byt prazdny, ak staci predvolene nastavenie
}

// Volane kratko po starte hry alebo ked sa vytvori HUD.
// Vytvara a pridava UMG widget na obrazovku.
void AMyHUD::BeginPlay()
{
	Super::BeginPlay();

    // UE_LOG(LogTemp, Log, TEXT("AMyHUD::BeginPlay for %s"), *GetNameSafe(GetOwningPlayerController()));

	// Uisti sa, ze mame platnu triedu widgetu nastavenu v Blueprint verzii tohto HUDu
	if (HUDWidgetClass)
	{
		APlayerController* PC = GetOwningPlayerController();
		if (PC)
		{
			// UE_LOG(LogTemp, Log, TEXT("Creating HUD Widget"));
			HUDWidget = CreateWidget<UMyHUDWidget>(PC, HUDWidgetClass);
			if (HUDWidget)
			{
				// UE_LOG(LogTemp, Log, TEXT("Adding HUD Widget to viewport"));
				HUDWidget->AddToViewport();
				// Poznamka: Pociatocne udaje pridu pravdepodobne z OnRep funkcii v PlayerState/GameState
				// potom, co sa tento HUD a jeho widget vytvoria.
			}
			// else { UE_LOG(LogTemp, Error, TEXT("Failed to create HUD Widget!")); }
		}
		// else { UE_LOG(LogTemp, Warning, TEXT("AMyHUD::BeginPlay - Owning Player Controller is null.")); }
	}
	// else { UE_LOG(LogTemp, Error, TEXT("AMyHUD::BeginPlay - HUDWidgetClass is not set! Assign WBP_HUD in BP_MyHUD defaults.")); }
}

// Aktualizuje pocet zabiti zobrazeny na HUD widgete.
void AMyHUD::UpdateKillCount(int32 Kills)
{
	if (HUDWidget)
	{
		// UE_LOG(LogTemp, Verbose, TEXT("AMyHUD forwarding Kill Count: %d"), Kills);
		HUDWidget->UpdateKillCount(Kills);
	}
	// else { UE_LOG(LogTemp, Warning, TEXT("AMyHUD::UpdateKillCount - HUDWidget is null")); }
}

// Aktualizuje pocet smrti zobrazeny na HUD widgete.
void AMyHUD::UpdateDeathCount(int32 Deaths)
{
	if (HUDWidget)
	{
		// UE_LOG(LogTemp, Verbose, TEXT("AMyHUD forwarding Death Count: %d"), Deaths);
		HUDWidget->UpdateDeathCount(Deaths);
	}
	// else { UE_LOG(LogTemp, Warning, TEXT("AMyHUD::UpdateDeathCount - HUDWidget is null")); }
}

// Aktualizuje zoznam sprav o zabitiach (kill feed) na HUD widgete.
void AMyHUD::UpdateKillFeed(const TArray<FString>& Messages)
{
	if (HUDWidget)
	{
		// UE_LOG(LogTemp, Verbose, TEXT("AMyHUD forwarding Kill Feed. Message Count: %d"), Messages.Num());
		HUDWidget->UpdateKillFeed(Messages);
	}
	// else { UE_LOG(LogTemp, Warning, TEXT("AMyHUD::UpdateKillFeed - HUDWidget is null")); }
}

// Aktualizuje zdravie zobrazené na HUD widgete.
void AMyHUD::UpdateHealth(float CurrentHealth, float MaxHealth)
{
	if (HUDWidget)
	{
		// UE_LOG(LogTemp, Verbose, TEXT("AMyHUD forwarding Health: %.1f / %.1f"), CurrentHealth, MaxHealth);
		HUDWidget->UpdateHealth(CurrentHealth, MaxHealth);
	}
	// else { UE_LOG(LogTemp, Warning, TEXT("AMyHUD::UpdateHealth - HUDWidget is null")); }
}
