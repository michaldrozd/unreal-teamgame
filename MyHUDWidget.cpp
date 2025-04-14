// Fill this file with the following content:
#include "MyHUDWidget.h"
#include "Components/TextBlock.h" // Nacitaj skutocny kod pre TextBlock
#include "Containers/UnrealString.h" // Pre pracu s textom (FString)

// Volane, ked sa widget vytvori. Nastavuje pociatocne hodnoty textovych poli.
void UMyHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

	// Nastav textove polia na pociatocne hodnoty alebo stav nacitavania
	UpdateKillCount(0);
	UpdateDeathCount(0);
	UpdateKillFeed({}); // Na zaciatku posli prazdne pole
}

// Aktualizuje text zobrazujuci pocet zabiti.
void UMyHUDWidget::UpdateKillCount(int32 Kills)
{
	if (KillCountText)
	{
		KillCountText->SetText(FText::FromString(FString::Printf(TEXT("Zabitia: %d"), Kills))); // Zmeneny text na Slovensky
	}
	// else { UE_LOG(LogTemp, Warning, TEXT("UMyHUDWidget::UpdateKillCount - KillCountText is null")); }
}

// Aktualizuje text zobrazujuci pocet smrti.
void UMyHUDWidget::UpdateDeathCount(int32 Deaths)
{
	if (DeathCountText)
	{
		DeathCountText->SetText(FText::FromString(FString::Printf(TEXT("Smrti: %d"), Deaths))); // Zmeneny text na Slovensky
	}
	// else { UE_LOG(LogTemp, Warning, TEXT("UMyHUDWidget::UpdateDeathCount - DeathCountText is null")); }
}

// Aktualizuje text zobrazujuci spravy o zabitiach (kill feed).
void UMyHUDWidget::UpdateKillFeed(const TArray<FString>& Messages)
{
	if (KillFeedText)
	{
		FString FeedContent = "";
		for (const FString& Msg : Messages)
		{
			FeedContent += Msg + TEXT("\n"); // Pridaj kazdu spravu na novy riadok
		}
		KillFeedText->SetText(FText::FromString(FeedContent.TrimEnd())); // Odstran posledny prazdny riadok
	}
	// else { UE_LOG(LogTemp, Warning, TEXT("UMyHUDWidget::UpdateKillFeed - KillFeedText is null")); }
}
