// Fill this file with the following content:
#include "MyGameState.h"
#include "Net/UnrealNetwork.h" // Potrebne pre posielanie informacii (replikaciu)
#include "MyHUD.h" // Nacitaj kod pre tvoje zobrazenie na obrazovke (HUD)
#include "GameFramework/PlayerController.h" // Pre ziskanie ovladaca lokalneho hraca
#include "Kismet/GameplayStatics.h" // Pre funkciu GetPlayerController

void AMyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Posli spravy o zabitiach vsetkym klientom
	DOREPLIFETIME(AMyGameState, KillFeedMessages);
}

void AMyGameState::AddKillFeedMessage(const FString& Message)
{
	// Tato funkcia sa MUSI volat iba na serveri (vacsinou hernym modom)
	if (HasAuthority())
	{
		KillFeedMessages.Add(Message);

		// Obmedz pocet sprav
		while (KillFeedMessages.Num() > MaxFeedMessages)
		{
			KillFeedMessages.RemoveAt(0);
		}

        // UE_LOG(LogTemp, Log, TEXT("Added Kill Feed Message (Server): %s"), *Message);

		// Rucne zavolaj OnRep funkciu aj pre samotny server
		OnRep_KillFeed();
        // ForceNetUpdate(); // Vacsinou netreba, ak sa vola OnRep
	}
}

void AMyGameState::OnRep_KillFeed()
{
    // Toto sa zavola u klientov, ked pridu nove spravy o zabitiach
    // UE_LOG(LogTemp, Log, TEXT("OnRep_KillFeed called. Message Count: %d"), KillFeedMessages.Num());
	UpdateHUDKillFeed();
}

void AMyGameState::UpdateHUDKillFeed()
{
    // GameState existuje u vsetkych, ale chceme menit len obrazovku *lokalneho* hraca.
    // Potrebujeme ziskat lokalneho ovladaca. Index 0 je vacsinou lokalny hrac.
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC && PC->IsLocalController()) // Pre istotu skontroluj, ci je lokalny
    {
        AMyHUD* HUD = Cast<AMyHUD>(PC->GetHUD());
        if (HUD)
        {
            // UE_LOG(LogTemp, Log, TEXT("Updating Kill Feed HUD."));
            HUD->UpdateKillFeed(KillFeedMessages);
        }
        // else { UE_LOG(LogTemp, Warning, TEXT("UpdateHUDKillFeed: Could not get MyHUD.")); }
    }
    // else { UE_LOG(LogTemp, Warning, TEXT("UpdateHUDKillFeed: Could not get local PlayerController(0).")); }
}
