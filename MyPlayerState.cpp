// Fill this file with the following content:
#include "MyPlayerState.h"
#include "Net/UnrealNetwork.h" // Potrebne pre posielanie informacii (replikaciu)
#include "MyHUD.h" // Nacitaj kod pre tvoje zobrazenie na obrazovke (HUD)
#include "GameFramework/PlayerController.h" // Pre ziskanie ovladaca, ktory vlastni tento stav

AMyPlayerState::AMyPlayerState()
{
    // Nastav pociatocne hodnoty
    Kills = 0;
    Deaths = 0;
}

void AMyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Posli pocet zabiti a smrti vsetkym klientom
	DOREPLIFETIME(AMyPlayerState, Kills);
	DOREPLIFETIME(AMyPlayerState, Deaths);
}

void AMyPlayerState::AddKill()
{
	// Tato funkcia sa MUSI volat iba na serveri (vacsinou hernym modom)
	if (HasAuthority())
	{
		Kills++;
        // UE_LOG(LogTemp, Warning, TEXT("Player %s Kills incremented to: %d (Server)"), *GetPlayerName(), Kills);
		// Rucne zavolaj OnRep pre samotny server, lebo OnRep sa automaticky spusta len u klientov
		OnRep_Kills();
        // ForceNetUpdate(); // Vacsinou netreba, ak sa vola OnRep, ale niekedy pomoze
	}
}

void AMyPlayerState::AddDeath()
{
	// Tato funkcia sa MUSI volat iba na serveri (vacsinou hernym modom)
	if (HasAuthority())
	{
		Deaths++;
        // UE_LOG(LogTemp, Warning, TEXT("Player %s Deaths incremented to: %d (Server)"), *GetPlayerName(), Deaths);
		// Rucne zavolaj OnRep pre samotny server
		OnRep_Deaths();
        // ForceNetUpdate();
	}
}

void AMyPlayerState::OnRep_Score()
{
    Super::OnRep_Score();
    // Aktualizuj HUD, ak pouzivas zakladne Skore
    // UpdateHUD();
}

void AMyPlayerState::OnRep_Kills()
{
    // Toto sa zavola u klientov, ked sa zmeni premenna Kills
    // UE_LOG(LogTemp, Warning, TEXT("Player %s OnRep_Kills: %d (Client)"), *GetPlayerName(), Kills);
	UpdateHUD();
}

void AMyPlayerState::OnRep_Deaths()
{
    // Toto sa zavola u klientov, ked sa zmeni premenna Deaths
    // UE_LOG(LogTemp, Warning, TEXT("Player %s OnRep_Deaths: %d (Client)"), *GetPlayerName(), Deaths);
	UpdateHUD();
}

void AMyPlayerState::UpdateHUD()
{
    // Ziskaj ovladac hraca spojeny s tymto stavom hraca
    // Poznamka: Na serveri moze GetPlayerController() vratit nic, ak sa zavola prilis skoro.
    // U klientov by mal byt platny potom, co sa stav hraca posle a priradi.
    APlayerController* PC = GetPlayerController();
    if (PC && PC->IsLocalController()) // Uisti sa, ze menime HUD iba pre lokalne ovladaneho hraca
    {
        AMyHUD* HUD = Cast<AMyHUD>(PC->GetHUD());
        if (HUD)
        {
            // UE_LOG(LogTemp, Log, TEXT("Updating HUD for %s - Kills: %d, Deaths: %d"), *GetPlayerName(), Kills, Deaths);
            HUD->UpdateKillCount(Kills);
            HUD->UpdateDeathCount(Deaths);
        }
        // else { UE_LOG(LogTemp, Warning, TEXT("UpdateHUD: Could not get MyHUD for %s"), *GetPlayerName()); }
    }
    // else { UE_LOG(LogTemp, Warning, TEXT("UpdateHUD: Could not get local PlayerController for %s"), *GetPlayerName()); }
}

// Urob funkciu ClientInitializeHUD, ak treba
// void AMyPlayerState::ClientInitializeHUD_Implementation()
// {
//     UpdateHUD();
// }
