// Fill this file with the following content:
#include "MyGameMode.h"
#include "MyCharacter.h" // Nacitaj kod pre tvoju postavicku
#include "MyPlayerState.h" // Nacitaj kod pre stav hraca (skore, atd.)
#include "MyGameState.h" // Nacitaj kod pre stav hry (kto koho zabil, atd.)
#include "MyHUD.h" // Nacitaj kod pre zobrazenie na obrazovke (HUD)
#include "UObject/ConstructorHelpers.h" // Pre hladanie tried (niekedy ina moznost ako priame nacitanie)
#include "GameFramework/PlayerStart.h" // Pre hladanie miest narodenia
#include "EngineUtils.h" // Pre prechadzanie vsetkych veci v hre (ako miesta narodenia)
#include "TimerManager.h" // Pre casovac ozivenia
#include "GameFramework/Controller.h" // Zakladna trieda pre ovladace (hracov alebo AI)

// Konstruktor pre GameMode. Nastavuje predvolene triedy pre Pawn, HUD, PlayerState, GameState.
AMyGameMode::AMyGameMode()
{
	// Nastav predvolenu postavicku na nasu vlastnu C++ postavicku
	DefaultPawnClass = AMyCharacter::StaticClass();

	// Nastav predvolene zobrazenie na obrazovke (HUD)
	HUDClass = AMyHUD::StaticClass();

	// Nastav predvoleny stav hraca
	PlayerStateClass = AMyPlayerState::StaticClass();

	// Nastav predvoleny stav hry
	GameStateClass = AMyGameState::StaticClass();

    // Povol hracom pripojit sa pocas hry
	bUseSeamlessTravel = true; // Odporucane pre plynulejsie prechody, ale vyzaduje viac nastaveni, ak menis levely
}

// Volane, ked je hrac zabity. Aktualizuje statistiky (Kills/Deaths), kill feed a spusti casovac pre respawn.
void AMyGameMode::PlayerKilled(AController* VictimController, AController* KillerController)
{
    // UE_LOG(LogTemp, Log, TEXT("PlayerKilled: Victim %s, Killer %s"),
    //     VictimController ? *VictimController->GetName() : TEXT("NULL"),
    //     KillerController ? *KillerController->GetName() : TEXT("NULL")
    // );

	AMyPlayerState* VictimPlayerState = VictimController ? Cast<AMyPlayerState>(VictimController->PlayerState) : nullptr;
	AMyPlayerState* KillerPlayerState = KillerController ? Cast<AMyPlayerState>(KillerController->PlayerState) : nullptr;
	AMyGameState* MyGameState = GetGameState<AMyGameState>(); // Pouzi konkretny GameState

	FString VictimName = "Player";
	if (VictimPlayerState)
	{
		VictimName = VictimPlayerState->GetPlayerName();
		VictimPlayerState->AddDeath(); // Zvys pocet smrti v stave hraca na serveri
		// UE_LOG(LogTemp, Log, TEXT("%s death count: %d"), *VictimName, VictimPlayerState->Deaths);
	}
	else
	{
		// UE_LOG(LogTemp, Warning, TEXT("PlayerKilled: Could not get VictimPlayerState."));
	}

	FString KillerName = "Environment"; // Predvolene meno, ak nie je znamy zabijak
	bool bSuicide = (KillerController == VictimController || KillerController == nullptr);

	if (KillerPlayerState)
	{
		KillerName = KillerPlayerState->GetPlayerName();
		// Nedavaj bod za zabitie, ak to bola samovrazda
		if (!bSuicide)
		{
			KillerPlayerState->AddKill(); // Zvys pocet zabiti v stave hraca na serveri
			// UE_LOG(LogTemp, Log, TEXT("%s kill count: %d"), *KillerName, KillerPlayerState->Kills);
		}
	}
	// else if (KillerController) { UE_LOG(LogTemp, Warning, TEXT("PlayerKilled: Could not get KillerPlayerState, but KillerController exists.")); }


	if (MyGameState)
	{
		FString KillMessage;
		if (bSuicide)
		{
			KillMessage = FString::Printf(TEXT("%s died."), *VictimName);
		}
		else
		{
			KillMessage = FString::Printf(TEXT("%s killed %s"), *KillerName, *VictimName);
		}
		MyGameState->AddKillFeedMessage(KillMessage); // Pridaj spravu do stavu hry, aby sa poslala vsetkym
	}
	else
	{
		// UE_LOG(LogTemp, Warning, TEXT("PlayerKilled: Could not get MyGameState."));
	}

	// Spusti casovac ozivenia pre obet
	if (VictimController)
	{
		FTimerHandle& TimerHandle = RespawnTimerHandles.FindOrAdd(VictimController); // Ziskaj existujuci casovac alebo pridaj novy
		if (GetWorldTimerManager().IsTimerActive(TimerHandle))
		{
			// UE_LOG(LogTemp, Warning, TEXT("Respawn timer already active for %s, clearing old one."), *VictimController->GetName());
			GetWorldTimerManager().ClearTimer(TimerHandle); // Zrus predchadzajuci casovac, ak nejaky bol (napr. rychle umrtia)
		}

		FTimerDelegate RespawnDelegate;
		RespawnDelegate.BindUFunction(this, FName("RespawnPlayer"), VictimController);
		GetWorldTimerManager().SetTimer(TimerHandle, RespawnDelegate, RespawnDelay, false); // Pouzi premennu RespawnDelay (oneskorenie ozivenia)
		// UE_LOG(LogTemp, Log, TEXT("Respawn timer set for %s (%.2f seconds)"), *VictimController->GetName(), RespawnDelay);
	}
	else
	{
		// UE_LOG(LogTemp, Warning, TEXT("PlayerKilled: VictimController is NULL, cannot start respawn timer."));
	}
}

// Vybera miesto narodenia (spawn point) pre hraca. Prechadza vsetky PlayerStart objekty v scene.
AActor* AMyGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    // Vyber miesto narodenia
    TArray<APlayerStart*> PreferredSpawns;
    TArray<APlayerStart*> FallbackSpawns;

    // Prejdi vsetky miesta narodenia v hre
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        APlayerStart* TestSpawn = *It;
        if (TestSpawn && TestSpawn->IsA<APlayerStart>()) // Zakladna kontrola
        {
            // Tu mozes pridat logiku na kontrolu, ci je miesto volne (napr. nie je zablokovane)
            // Pre jednoduchost pridame vsetky najdene miesta.
            // Mozes uprednostnit miesta podla znaciek alebo inych vlastnosti.
            FallbackSpawns.Add(TestSpawn);
        }
    }

    APlayerStart* BestStart = nullptr;
    if (PreferredSpawns.Num() > 0)
    {
        BestStart = PreferredSpawns[FMath::RandRange(0, PreferredSpawns.Num() - 1)];
    }
    else if (FallbackSpawns.Num() > 0)
    {
        BestStart = FallbackSpawns[FMath::RandRange(0, FallbackSpawns.Num() - 1)];
    }

    // UE_LOG(LogTemp, Log, TEXT("Chose PlayerStart: %s"), BestStart ? *BestStart->GetName() : TEXT("NULL (will use default)"));
    return BestStart ? BestStart : Super::ChoosePlayerStart_Implementation(Player); // Vrat najdene miesto alebo pouzi predvolene spravanie
}

// Vykonava samotne ozivenie (respawn) hraca na vybranom mieste narodenia.
void AMyGameMode::RespawnPlayer(AController* PlayerController)
{
	if (!PlayerController)
	{
		// UE_LOG(LogTemp, Error, TEXT("RespawnPlayer called with NULL Controller!"));
		return;
	}

	// UE_LOG(LogTemp, Log, TEXT("Attempting to respawn player %s"), *PlayerController->GetName());

	// Uprac casovac pre tohto hraca
	RespawnTimerHandles.Remove(PlayerController);

	// Najdi miesto narodenia
	AActor* SpawnPoint = FindPlayerStart(PlayerController); // Pouzi funkciu FindPlayerStart z GameMode, ktora vola ChoosePlayerStart

	if (SpawnPoint)
	{
		// UE_LOG(LogTemp, Log, TEXT("Respawning %s at %s"), *PlayerController->GetName(), *SpawnPoint->GetName());
		RestartPlayerAtPlayerStart(PlayerController, SpawnPoint);
	}
	else
	{
		// UE_LOG(LogTemp, Warning, TEXT("No suitable PlayerStart found for %s. Using default RestartPlayer."), *PlayerController->GetName());
		RestartPlayer(PlayerController); // Pouzi predvolenu logiku narodenia, ak sa nenaslo ziadne miesto
	}

	// Moznost: Obnov zdravie/naboje na novo narodenej postavicke, ak to nerobi konstruktor
	// AMyCharacter* NewPawn = Cast<AMyCharacter>(PlayerController->GetPawn());
	// if (NewPawn) { // Reset stats if needed }
}

// Volane, ked sa hrac odpoji z hry. Uprace casovac respawnu a informuje ostatnych.
void AMyGameMode::Logout(AController* Exiting)
{
	// UE_LOG(LogTemp, Log, TEXT("Player %s is logging out."), Exiting ? *Exiting->GetName() : TEXT("NULL"));

	// Uprac pripadny cakajuci casovac ozivenia pre odchadzajuceho hraca
	if (Exiting)
	{
		FTimerHandle* TimerHandlePtr = RespawnTimerHandles.Find(Exiting);
		if (TimerHandlePtr && GetWorldTimerManager().IsTimerActive(*TimerHandlePtr))
		{
			// UE_LOG(LogTemp, Log, TEXT("Clearing respawn timer for logging out player %s."), *Exiting->GetName());
			GetWorldTimerManager().ClearTimer(*TimerHandlePtr);
		}
		RespawnTimerHandles.Remove(Exiting);
	}

	// Moznost: Informuj stav hry alebo ostatnych hracov o odpojeni
	AMyGameState* MyGameState = GetGameState<AMyGameState>();
	AMyPlayerState* PS = Exiting ? Cast<AMyPlayerState>(Exiting->PlayerState) : nullptr;
	if (MyGameState && PS)
	{
		FString LogoutMessage = FString::Printf(TEXT("%s left the game."), *PS->GetPlayerName());
		MyGameState->AddKillFeedMessage(LogoutMessage);
	}

	Super::Logout(Exiting); // Zavolaj kod z rodicovskej triedy
}
