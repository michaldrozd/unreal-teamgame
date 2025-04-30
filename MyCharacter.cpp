// Fill this file with the following content:
#include "MyCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h" // May not be needed if camera is directly attached
#include "EnhancedInputComponent.h" // Required for Enhanced Input
#include "EnhancedInputSubsystems.h" // Required for Enhanced Input
#include "Net/UnrealNetwork.h" // Required for DOREPLIFETIME
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "MyGameMode.h" // Include GameMode for death notification
#include "MyPlayerState.h" // Include PlayerState for getting info
#include "MyHUD.h" // Include HUD for updating health display
#include "GameFramework/PlayerController.h" // For disabling input and getting HUD
#include "Components/SkeletalMeshComponent.h" // For ragdoll mesh access
#include "Kismet/GameplayStatics.h" // For playing sounds and spawning emitters
#include "Sound/SoundBase.h" // For USoundBase
#include "Particles/ParticleSystem.h" // For UParticleSystem
#include "Particles/ParticleSystemComponent.h" // For spawning emitters

//////////////////////////////////////////////////////////////////////////
// AMyCharacter

// Konstruktor pre postavu. Nastavuje komponenty a ich pociatocne hodnoty.
AMyCharacter::AMyCharacter()
{
	// Nastavi velkost "neviditelnej kapsule" okolo postavicky, aby vedela, do coho naraza
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Nastavenia pohybu postavicky
	GetCharacterMovement()->bOrientRotationToMovement = true; // Postavicka sa otoci tam, kam stlacis...
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...takouto rychlostou otacania

	// Poznamka: Tieto a ine veci sa daju lahsie menit v editore (Blueprint)
	// namiesto toho, aby sme museli znova kompilovat kod
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Vytvori kameru, cez ktoru sa pozeras
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Umiestni kameru na spravne miesto
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Nastavi pociatocne zdravie
	CurrentHealth = MaxHealth;

	// Nastav pociatocnu municiu. Toto by sa malo udiat len raz na serveri pri spawne.
	// OnRep funkcie zabezpecia synchronizaciu s klientami.
	if (HasAuthority())
	{
		CurrentAmmoInClip = MaxAmmoInClip;
		ReserveAmmo = StartingReserveAmmo;
	}

	// Povie postavicke, aby kazdy moment (frame) nieco robila. Da sa to vypnut pre lepsi vykon.
	// PrimaryActorTick.bCanEverTick = true; // Vacsinou to netreba, iba ak potrebujes nieco specialne robit kazdy moment

	// Poznamka: Model postavicky (kostra) a animacie sa nastavuju v editore (Blueprint)
	// v subore odvodeneho blueprintu
	// (alebo pre pohlad z prvej osoby)
	// Tu nastavujeme viditelnost a poziciu modelu pre pohlad z prvej osoby
	GetMesh()->SetOwnerNoSee(true); // Skryje hlavne telo postavicky pred hracom, ktory ju ovlada (aby si nevidel svoje telo)
	GetMesh()->SetupAttachment(FirstPersonCameraComponent); // Pripoji model ku kamere (alebo vytvori samostatny model ruk)
	GetMesh()->bCastDynamicShadow = false;
	GetMesh()->CastShadow = false;
	//GetMesh()->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f)); // Uprav podla potreby
	//GetMesh()->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f)); // Uprav podla potreby
}

// Volane na zaciatku hry alebo ked sa postava objavi (spawn). Pridava Input Mapping Context.
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Prida nastavenia ovladania pre hraca na tomto pocitaci
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

// Definuje, ktore premenne sa maju posielat (replikovat) cez siet.
void AMyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMyCharacter, CurrentHealth);
	DOREPLIFETIME(AMyCharacter, CurrentAmmoInClip);
	DOREPLIFETIME(AMyCharacter, ReserveAmmo);
	DOREPLIFETIME(AMyCharacter, bIsReloading);
	// Poznamka: Maximalne zdravie, MaxAmmoInClip, StartingReserveAmmo, ReloadDuration, BaseSpreadAngle
	// netreba posielat ostatnym, ak sa menia len v editore/pri spawne.
}

// Volane u klientov, ked sa zmeni premenna CurrentHealth (ked server posle aktualizaciu).
void AMyCharacter::OnRep_Health()
{
	// Moznost: Reakcia na zmenu zdravia u hraca (napr. zvuk bolesti, zmena na obrazovke)
	// Moznost: Reakcia na zmenu zdravia u hraca (napr. zvuk bolesti, zmena na obrazovke)
	// Aktualizuj HUD pre lokalneho hraca
	UpdateHUDHealth();
}

// Volane u klientov, ked sa zmeni premenna CurrentAmmoInClip. Aktualizuje HUD.
void AMyCharacter::OnRep_CurrentAmmoInClip()
{
	// Toto sa zavola u klientov, ked sa zmeni pocet nabojov v zasobniku
	// UE_LOG(LogTemp, Warning, TEXT("Player %s OnRep_CurrentAmmoInClip: %d"), *GetName(), CurrentAmmoInClip);
	// Aktualizuj aj HUD s municou
	AMyPlayerState* PS = GetPlayerState<AMyPlayerState>();
	if (PS && IsLocallyControlled())
	{
		// Predpokladame, ze PlayerState ma funkciu na aktualizaciu municie v HUD
		// Ak nie, musis ju volat priamo z Character -> HUD (podobne ako zdravie)
		// Zatial volajme priamo z Character -> HUD pre jednoduchost
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC && PC->IsLocalController())
		{
			AMyHUD* HUD = Cast<AMyHUD>(PC->GetHUD());
			if (HUD)
			{
				HUD->UpdateAmmo(CurrentAmmoInClip, MaxAmmoInClip, ReserveAmmo);
			}
		}
	}
}

// Volane u klientov, ked sa zmeni premenna bIsReloading. Spusti/zastavi animaciu znovunabijania.
void AMyCharacter::OnRep_IsReloading()
{
	// UE_LOG(LogTemp, Warning, TEXT("OnRep_IsReloading: %s for %s"), bIsReloading ? TEXT("True") : TEXT("False"), *GetName());
	if (bIsReloading)
	{
		// Spusti animaciu znovunabijania na klientovi
		// Hraj animaciu znovunabijania
		// PlayAnimMontage(ReloadMontage); // Predpoklada existenciu ReloadMontage
	}
	else
	{
		// Ukonci animaciu znovunabijania na klientovi (ak predtym bezala)
		// StopAnimMontage(ReloadMontage); // Predpoklada existenciu ReloadMontage
	}
	// Mozno aktualizovat aj stav HUDu (napr. zobrazit ikonku prebíjania)
}

// Inicializuje HUD pre lokalneho hraca. Volane po PossessedBy a OnRep_PlayerState.
// Pridana inicializacia municie.
void AMyCharacter::InitializeHUD()
{
	// Tato funkcia sa moze volat, ked server zacne ovladat postavicku
	// a ked klient dostane informacie o hracovi, aby sa zobrazili spravne udaje
	// ked su informacie o hracovi pripravene.
	AMyPlayerState* PS = GetPlayerState<AMyPlayerState>();
	if (PS && IsLocallyControlled()) // Uisti sa, ze menime len obrazovku hraca na tomto pocitaci
	{
		// Aktualizuj zdravie na HUD pri inicializacii
		UpdateHUDHealth();
		// Aktualizuj municiu na HUD pri inicializacii
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC && PC->IsLocalController())
		{
			AMyHUD* HUD = Cast<AMyHUD>(PC->GetHUD());
			if (HUD)
			{
				HUD->UpdateAmmo(CurrentAmmoInClip, MaxAmmoInClip, ReserveAmmo);
			}
		}
		// Ak potrebujes inicializovat aj ine veci z PlayerState (Kills/Deaths),
		// je lepsie mat centralnu funkciu v PlayerState, ktora aktualizuje vsetko.
	}
}

// Pomocna funkcia na aktualizaciu zdravia na HUD pre lokalneho hraca.
void AMyCharacter::UpdateHUDHealth()
{
	// Ziskaj lokalneho ovladaca
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC && PC->IsLocalController())
	{
		// Ziskaj HUD
		AMyHUD* HUD = Cast<AMyHUD>(PC->GetHUD());
		if (HUD)
		{
			// Aktualizuj zdravie
			HUD->UpdateHealth(CurrentHealth, MaxHealth);
		}
		// else { UE_LOG(LogTemp, Warning, TEXT("UpdateHUDHealth: Could not get MyHUD for %s"), *GetName()); }
	}
	// else { // Toto sa moze stat na serveri alebo u neovladanych postaviciek, co je v poriadku
	//     // UE_LOG(LogTemp, Log, TEXT("UpdateHUDHealth: Not a local controller for %s"), *GetName());
	// }
}

//////////////////////////////////////////////////////////////////////////
// Vstup (Ovladanie)

// Nastavuje prepojenie medzi vstupnymi akciami (Input Actions) a funkciami v tejto triede.
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Nastavi, co ktore tlacidla robia v hre
	check(PlayerInputComponent);

	// Pouzije novy system pre ovladanie
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Strelba
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &AMyCharacter::StartFire);

		// Skok (ak pouzivame novy system)
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Pohyb (ak pouzivame novy system)
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCharacter::Move);

		// Pozeranie (ak pouzivame novy system)
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyCharacter::Look);

		// Moznost: Pridaj akciu pre znovunabijanie
		// EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &AMyCharacter::StartReload); // Potrebujes ReloadAction Input Action
	}
}

// Spracovava vstup pre pohyb dopredu/dozadu a dolava/doprava.
void AMyCharacter::Move(const FInputActionValue& Value)
{
	// Ak sa prave znovunabija, neumozni pohyb, ak by to bolo zavisle od animacie (zvycajne sa umozni pohyb)
	// if (bIsReloading) return; // Odkomentuj, ak sa neda hybat pocas znovunabijania
	// Ak si mrtvy, neumozni pohyb
	if (CurrentHealth <= 0) return;


	// vstup su dve cisla (smer dopredu/dozadu a dolava/doprava)
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// zisti, ktorym smerom sa pozeras
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// ziskaj smer dopredu
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// ziskaj smer doprava
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// pridaj pohyb
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

// Spracovava vstup pre otacanie pohladu (mys).
void AMyCharacter::Look(const FInputActionValue& Value)
{
	// Ak si mrtvy, neumozni pozeranie (vacsinou sa povoli pozeranie okolo)
	// if (CurrentHealth <= 0) return; // Odkomentuj, ak sa neda pozerat po smrti

	// vstup su dve cisla (pohyb mysou hore/dole a dolava/doprava)
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// pridaj otacanie dolava/doprava a hore/dole
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

// Volane, ked hrac stlaci tlacidlo pre strelbu. Skontroluje rychlost strelby a spusti Server_Fire.
// Pridana kontrola municie a stavu znovunabijania.
void AMyCharacter::StartFire(const FInputActionValue& Value) // Upravene pre Enhanced Input
{
	// Nekonaj, ak si mrtvy, znovunabijas, alebo nemas naboje v zasobniku, alebo sa neda strielat (FireRate)
	if (CurrentHealth <= 0 || bIsReloading || CurrentAmmoInClip <= 0 || GetWorld()->GetTimeSeconds() - LastFireTime < FireRate)
	{
		// UE_LOG(LogTemp, Warning, TEXT("StartFire Blocked: Health(%d) Reloading(%s) Ammo(%d) FireRate Met(%s)"),
		// 	(int)CurrentHealth, bIsReloading ? TEXT("True") : TEXT("False"), CurrentAmmoInClip,
		// 	(GetWorld()->GetTimeSeconds() - LastFireTime >= FireRate) ? TEXT("True") : TEXT("False"));
		return;
	}

	// UE_LOG(LogTemp, Warning, TEXT("StartFire Called (Client or Server Local), firing allowed."));
	// Hned zobraz efekt strelby u hraca (napr. zablesk pri hlavni) pre client-side prediction FX pre lepsiu odozvu.
	// Server_Fire_Implementation spusti efekty znova cez Multicast_PlayFireEffects pre konzistenciu.
	Multicast_PlayFireEffects(); // Mozes volat aj tu pre client-side prediction FX
	Server_Fire(); // Zavolaj funkciu na serveri, ktory overi a aplikuje poskodenie

	// Klient-side predikcia spotreby municie
	// Toto je len pre okamzitu vizualnu spatnu vazbu, server je autoritativny
	// Ak server odmietne vystrel (napr. pre cheat), klient by mal zosynchronizovat stav municie
	// For now, rely on server to replicate ammo changes.
}

// Volane klientom na spustenie procesu znovunabijania.
void AMyCharacter::StartReload()
{
	// Spusti znovunabijanie, ak uz nie sme v procese, nie sme mrtvi, a mame nejake naboje mimo zasobnika, a zasobnik nie je plny
	if (!bIsReloading && CurrentHealth > 0 && ReserveAmmo > 0 && CurrentAmmoInClip < MaxAmmoInClip)
	{
		// UE_LOG(LogTemp, Warning, TEXT("StartReload Called (Client or Server Local)"));
		Server_StartReload(); // Povedz serveru, ze chces znovunabijat
	}
	// else { UE_LOG(LogTemp, Warning, TEXT("StartReload Blocked: Reloading(%s) Health(%d) Reserve(%d) AmmoInClip(%d/%d)"),
	// 	bIsReloading ? TEXT("True") : TEXT("False"), (int)CurrentHealth, ReserveAmmo, CurrentAmmoInClip, MaxAmmoInClip); }
}

// Serverova implementacia znovunabijania. Nastavi stav, casovac a spusti OnRep.
bool AMyCharacter::Server_StartReload_Validate()
{
	// Zakladna validacia: hrac nie je mrtvy, uz neznovunabija, ma naboje mimo zasobnika a zasobnik nie je plny
	return CurrentHealth > 0 && !bIsReloading && ReserveAmmo > 0 && CurrentAmmoInClip < MaxAmmoInClip;
}

void AMyCharacter::Server_StartReload_Implementation()
{
	// UE_LOG(LogTemp, Warning, TEXT("Server_StartReload_Implementation Called on Server"));

	// Nastav stav znovunabijania
	bIsReloading = true;

	// Spusti OnRep_IsReloading rucne na serveri (spusti sa aj na klientoch automaticky)
	OnRep_IsReloading();

	// Spusti casovac na dokonceni znovunabijania
	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AMyCharacter::FinishReload, ReloadDuration, false);

	// Moznost: Hraj animaciu znovunabijania na serveri (nemusí byt vidiet, ale kvoli konzistencii logiky)
	// PlayAnimMontage(ReloadMontage); // Predpoklada existenciu ReloadMontage
}

// Volane casovacom na serveri po skonceni znovunabijania. Doplna municiu.
void AMyCharacter::FinishReload()
{
	// UE_LOG(LogTemp, Warning, TEXT("FinishReload Called on Server"));

	// Vypocitaj, kolko nabojov treba doplnit
	int32 AmmoNeeded = MaxAmmoInClip - CurrentAmmoInClip;
	// Vypocitaj, kolko nabojov mozeme realne pouzit z rezervy
	int32 AmmoToTakeFromReserve = FMath::Min(AmmoNeeded, ReserveAmmo);

	// Doplna municiu v zasobniku
	CurrentAmmoInClip += AmmoToTakeFromReserve;
	// Uber z rezervy
	ReserveAmmo -= AmmoToTakeFromReserve;

	// Ukonci stav znovunabijania
	bIsReloading = false;

	// Spusti OnRep_IsReloading rucne na serveri (spusti sa aj na klientoch automaticky)
	OnRep_IsReloading();

	// Spusti OnRep_CurrentAmmoInClip rucne na serveri, aby sa aktualizoval HUD (spusti sa aj na klientoch automaticky)
	OnRep_CurrentAmmoInClip();

	// UE_LOG(LogTemp, Warning, TEXT("Reload Finished on Server. Ammo: %d / %d"), CurrentAmmoInClip, ReserveAmmo);
}


// Validacna funkcia pre Server_Fire. Kontroluje, ci moze server spustit strelbu.
// Pridana kontrola stavu municie a znovunabijania.
bool AMyCharacter::Server_Fire_Validate()
{
	// Zakladna kontrola + kontrola municie a znovunabijania
	return CurrentHealth > 0 && !bIsReloading && CurrentAmmoInClip > 0 && GetWorld()->GetTimeSeconds() - LastFireTime >= FireRate;
}

// Funkcia pre strelbu vykonavana na serveri. Robi raycast, spotrebuje municiu a aplikuje poskodenie.
// Pridana logika spotreby municie a rozptylu.
void AMyCharacter::Server_Fire_Implementation()
{
	// UE_LOG(LogTemp, Warning, TEXT("Server_Fire_Implementation Called on Server"));

	// Ziskaj ovladaca postavicky
	AController* MyController = GetController();
	if (!MyController)
	{
		// UE_LOG(LogTemp, Error, TEXT("Server_Fire_Implementation: No controller found. Aborting."));
		return;
	}

	// Kontrola rychlosti strelby aj na serveri (autoritativna kontrola)
	// Túto kontrolu robíme už aj vo Validate, ale je dobré ju mať aj tu pre istotu
	if (GetWorld()->GetTimeSeconds() - LastFireTime < FireRate || bIsReloading || CurrentAmmoInClip <= 0)
	{
		// UE_LOG(LogTemp, Warning, TEXT("Server_Fire_Implementation Blocked by State Check."));
		return; // Ak rychlost strelby nie je splnena, alebo znovunabijame, alebo nemame naboje, funkciu ukonci
	}

	// UE_LOG(LogTemp, Warning, TEXT("Server_Fire_Implementation Called on Server, firing allowed."));

	// Aktualizuj cas posledneho vystrelu hned po uspesnej kontrole, nezavisle od zasahu
	LastFireTime = GetWorld()->GetTimeSeconds();

	// Spotrebuj municiu (iba na serveri)
	CurrentAmmoInClip--;
	// Spusti OnRep rucne na serveri
	OnRep_CurrentAmmoInClip();


	// Spusti efekty strelby u vsetkych hracov (zvuk, zablesk). Toto sa zavola aj na serveri,
	// cize na klientovi sa to zavola 2x (raz ako client-side prediction, raz replikaciou),
	// ale Unreal Engine si s tym poradi.
	Multicast_PlayFireEffects();

	// --- Vypocitaj smer strely s rozptylom ---
	FVector Start = FVector::ZeroVector;
	FRotator Rot = FRotator::ZeroRotator;

	// Ziskaj pohlad ovladaca (zvycajne kamery)
	MyController->GetPlayerViewPoint(Start, Rot);

	FVector ForwardVector = Rot.Vector(); // Pouzi smer otocenia ovladaca
	FVector End = Start + (ForwardVector * WeaponRange); // Zakladny koniec luca bez rozptylu

	// Aplikuj rozptyl (Spread)
	// Generuj nahodnu rotaciu v kuzeli s uhlom BaseSpreadAngle
	FRotator SpreadRotation = FMath::VRandCone(ForwardVector, FMath::DegreesToRadians(BaseSpreadAngle)).Rotation();
	FVector EndWithSpread = Start + (SpreadRotation.Vector() * WeaponRange); // Koniec luca s rozptylom

	// 1. Vystrel "luc" z pohladu ovladaca, aby sme zistili, co sme trafili
	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // Ignoruj sam seba (aby si sa netrafil)
	QueryParams.bTraceComplex = true; // Pre presnejsi zasah na komplexnych modeloch

	// Pouzi vlastny kolizny kanal pre strelbu. Predpoklada, ze 'WeaponTrace' je definovany v Project Settings -> Collision a mapuje sa na ECC_WeaponTrace.
	// Ak pouzivas iny nazov kanalu, alebo ho nemas nastaveny cez Project Settings, musis tu pouzit spravny enum (napr. ECC_GameTraceChannel1)
	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, EndWithSpread, ECC_WeaponTrace, QueryParams); // Pouzi koniec luca s rozptylom

	// Moznost: Zobraz "debug line" pre raycast
	// DrawDebugLine(GetWorld(), Start, bHit ? Hit.ImpactPoint : EndWithSpread, bHit ? FColor::Green : FColor::Red, false, 2.0f, 0, 1.0f);


	if (bHit && Hit.GetActor())
	{
		// UE_LOG(LogTemp, Warning, TEXT("Server Fire Hit: %s at %.1f distance"), *Hit.GetActor()->GetName(), Hit.Distance);
		// 2. Skontroluj, ci trafena vec je ina postavicka hraca
		AMyCharacter* HitCharacter = Cast<AMyCharacter>(Hit.GetActor());
		if (HitCharacter && HitCharacter != this) // Uisti sa, ze je to postavicka a nie ty sam
		{
			// UE_LOG(LogTemp, Warning, TEXT("Applying Damage to: %s"), *HitCharacter->GetName());
			// 3. Daj zranenie - pouzi BaseDamage namiesto priameho cisla
			float DamageAmount = BaseDamage; // Pouzi premennu BaseDamage
			FPointDamageEvent DamageEvent(DamageAmount, Hit, ForwardVector, nullptr); // Pouzi povodny ForwardVector pre vyhodnotenie smeru poskodenia
			// EventInstigator je ovladac (hrac alebo AI), ktory sposobil zranenie
			HitCharacter->TakeDamage(DamageAmount, DamageEvent, MyController, this); // Pouzi DamageAmount (BaseDamage)
		}
		// Moznost: Daj zranenie aj inym veciam, ktore sa daju rozbit
	}
	else
	{
		// UE_LOG(LogTemp, Warning, TEXT("Server Fire Missed or Hit Non-Actor"));
	}

	// Moznost: Ak po vystrele ostalo 0 nabojov a mas rezervu, automaticky zacni znovunabijat
	// if (CurrentAmmoInClip <= 0 && ReserveAmmo > 0 && !bIsReloading)
	// {
	// 	StartReload(); // Alebo Server_StartReload(); ak chces, aby klient predikoval zaciatok
	// }
}

// Inicializuje HUD pre lokalneho hraca. Volane po PossessedBy a OnRep_PlayerState.
void AMyCharacter::InitializeHUD()
{
	// Tato funkcia sa moze volat, ked server zacne ovladat postavicku
	// a ked klient dostane informacie o hracovi, aby sa zobrazili spravne udaje
	// ked su informacie o hracovi pripravene.
	AMyPlayerState* PS = GetPlayerState<AMyPlayerState>();
	if (PS && IsLocallyControlled()) // Uisti sa, ze menime len obrazovku hraca na tomto pocitaci
	{
		// Aktualizuj zdravie na HUD pri inicializacii
		UpdateHUDHealth();
		// Ak potrebujes inicializovat aj ine veci z PlayerState (Kills/Deaths),
		// je lepsie mat centralnu funkciu v PlayerState, ktora aktualizuje vsetko.
	}
}

// Volane na serveri, ked ovladac (Controller) prevezme kontrolu nad touto postavou.
void AMyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeHUD(); // Pripravi zobrazenie pre hraca, ktory je zaroven serverom
}

// Volane u klientov, ked sa priradi PlayerState k tejto postave.
void AMyCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitializeHUD(); // Pripravi zobrazenie pre klientov, ked dostanu informacie o hracovi
}


//////////////////////////////////////////////////////////////////////////
// Vstup (Ovladanie)

// Nastavuje prepojenie medzi vstupnymi akciami (Input Actions) a funkciami v tejto triede.
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Nastavi, co ktore tlacidla robia v hre
	check(PlayerInputComponent);

	// Pouzije novy system pre ovladanie
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Strelba
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &AMyCharacter::StartFire);

		// Skok (ak pouzivame novy system)
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Pohyb (ak pouzivame novy system)
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCharacter::Move);

		// Pozeranie (ak pouzivame novy system)
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyCharacter::Look);
	}
}

// Spracovava vstup pre pohyb dopredu/dozadu a dolava/doprava.
void AMyCharacter::Move(const FInputActionValue& Value)
{
	// vstup su dve cisla (smer dopredu/dozadu a dolava/doprava)
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// zisti, ktorym smerom sa pozeras
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// ziskaj smer dopredu
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// ziskaj smer doprava
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// pridaj pohyb
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

// Spracovava vstup pre otacanie pohladu (mys).
void AMyCharacter::Look(const FInputActionValue& Value)
{
	// vstup su dve cisla (pohyb mysou hore/dole a dolava/doprava)
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// pridaj otacanie dolava/doprava a hore/dole
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

// Volane, ked hrac stlaci tlacidlo pre strelbu. Skontroluje rychlost strelby a spusti Server_Fire.
void AMyCharacter::StartFire(const FInputActionValue& Value) // Upravene pre Enhanced Input
{
	// Kontrola na strane klienta pre rychlejsiu odozvu (predikcia)
	// Autoritatívna kontrola prebehne aj na serveri v Server_Fire
	if (GetWorld()->GetTimeSeconds() - LastFireTime >= FireRate)
	{
		// UE_LOG(LogTemp, Warning, TEXT("StartFire Called (Client or Server Local), firing allowed."));
		// Hned zobraz efekt strelby u hraca (napr. zablesk pri hlavni) pre client-side prediction FX pre lepsiu odozvu.
		// Server_Fire_Implementation spusti efekty znova cez Multicast_PlayFireEffects pre konzistenciu.
		Multicast_PlayFireEffects(); // Mozes volat aj tu pre client-side prediction FX
		Server_Fire(); // Zavolaj funkciu na serveri, ktory overi a aplikuje poskodenie
	}
	// else { UE_LOG(LogTemp, Warning, TEXT("StartFire Called, FireRate not met.")); }
}

// Validacna funkcia pre Server_Fire. Kontroluje, ci moze server spustit strelbu.
bool AMyCharacter::Server_Fire_Validate()
{
	return true; // Zakladna kontrola, pridaj dalsie, ak treba (napr. ci mas naboje, ci mozes strielat)
}

// Funkcia pre strelbu vykonavana na serveri. Robi raycast a aplikuje poskodenie.
void AMyCharacter::Server_Fire_Implementation()
{
	// UE_LOG(LogTemp, Warning, TEXT("Server_Fire_Implementation Called on Server"));

	// 1. Vystrel "luc" z kamery, aby sme zistili, co sme trafili
	FVector Start = FVector::ZeroVector;
	FRotator Rot = FRotator::ZeroRotator;

	// Ziskaj pohlad ovladaca namiesto kamery pre lepsiu presnost
	AController* MyController = GetController();
	if (!MyController)
	{
		return;
	}

	// Kontrola rychlosti strelby aj na serveri (autoritativna kontrola)
	if (GetWorld()->GetTimeSeconds() - LastFireTime < FireRate)
	{
		// UE_LOG(LogTemp, Warning, TEXT("Server_Fire_Implementation Called, FireRate not met on server. Aborting."));
		return; // Ak rychlost strelby nie je splnena, funkciu ukonci
	}

	// UE_LOG(LogTemp, Warning, TEXT("Server_Fire_Implementation Called on Server, firing allowed."));

	// Aktualizuj cas posledneho vystrelu hned po uspesnej kontrole, nezavisle od zasahu
	LastFireTime = GetWorld()->GetTimeSeconds();

	// Spusti efekty strelby u vsetkych hracov (zvuk, zablesk). Toto sa zavola aj na serveri,
	// cize na klientovi sa to zavola 2x (raz ako client-side prediction, raz replikaciou),
	// ale Unreal Engine si s tym poradi.
	Multicast_PlayFireEffects();

	// 1. Vystrel "luc" z pohladu ovladaca, aby sme zistili, co sme trafili
	FVector Start = FVector::ZeroVector;
	FRotator Rot = FRotator::ZeroRotator;

	// Ziskaj pohlad ovladaca (zvycajne kamery)
	MyController->GetPlayerViewPoint(Start, Rot);
	FVector ForwardVector = Rot.Vector(); // Pouzi smer otocenia ovladaca
	FVector End = Start + (ForwardVector * WeaponRange); // Pouzi nastavitelny dosah zbrane
	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // Ignoruj sam seba (aby si sa netrafil)
	QueryParams.bTraceComplex = true;

	// Pouzi vlastny kolizny kanal pre strelbu. Predpoklada, ze 'WeaponTrace' je definovany v Project Settings -> Collision a mapuje sa na ECC_WeaponTrace.
	// Ak pouzivas iny nazov kanalu, alebo ho nemas nastaveny cez Project Settings, musis tu pouzit spravny enum (napr. ECC_GameTraceChannel1)
	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WeaponTrace, QueryParams); // Pouzi definovany WeaponTrace kanal

	if (bHit && Hit.GetActor())
	{
		// UE_LOG(LogTemp, Warning, TEXT("Server Fire Hit: %s"), *Hit.GetActor()->GetName());
		// 2. Skontroluj, ci trafena vec je ina postavicka hraca
		AMyCharacter* HitCharacter = Cast<AMyCharacter>(Hit.GetActor());
		if (HitCharacter && HitCharacter != this) // Uisti sa, ze je to postavicka a nie ty sam
		{
			// UE_LOG(LogTemp, Warning, TEXT("Applying Damage to: %s"), *HitCharacter->GetName());
			// 3. Daj zranenie - pouzi BaseDamage namiesto priameho cisla
			float DamageAmount = BaseDamage; // Pouzi premennu BaseDamage
			FPointDamageEvent DamageEvent(DamageAmount, Hit, ForwardVector, nullptr);
			// EventInstigator je ovladac (hrac alebo AI), ktory sposobil zranenie
			HitCharacter->TakeDamage(DamageAmount, DamageEvent, MyController, this); // Pouzi DamageAmount (BaseDamage)
		}
		// Moznost: Daj zranenie aj inym veciam, ktore sa daju rozbit
	}
	else
	{
		// UE_LOG(LogTemp, Warning, TEXT("Server Fire Missed or Hit Non-Actor"));
	}

}

// Prehra efekty strelby (zvuk, zablesk) na vsetkych klientoch a serveri.
void AMyCharacter::Multicast_PlayFireEffects_Implementation()
{
	// Prehraj zvuk strelby
	if (FireSound)
	{
		// Moznost 1: Zvuk na mieste postavicky
		// UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());

		// Moznost 2: Zvuk pripojeny k modelu (lepsie pre 3D zvuk)
		// Potrebujes mat spravne nastaveny model v GetMesh()
		UGameplayStatics::SpawnSoundAttached(FireSound, GetMesh(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, false);
		// Ak mas specificky socket pre hlaven zbrane na modeli, pouzi ho namiesto NAME_None
		// napr. UGameplayStatics::SpawnSoundAttached(FireSound, GetMesh(), FName("MuzzleSocket"));
	}

	// Zobraz efekt zablesku
	if (MuzzleFlashFX)
	{
		// Potrebujes mat spravne nastaveny model v GetMesh()
		// Ak mas specificky socket pre hlaven zbrane na modeli, pouzi ho namiesto NAME_None
		// napr. UGameplayStatics::SpawnEmitterAttached(MuzzleFlashFX, GetMesh(), FName("MuzzleSocket"), FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget);
		UGameplayStatics::SpawnEmitterAttached(MuzzleFlashFX, GetMesh(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget);
	}
}


// Spracovava prijate poskodenie. Volane iba na serveri. Zmensuje zdravie a vola Die, ak zdravie klesne na 0.
float AMyCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	// Spracuj zranenie iba na serveri
	if (GetLocalRole() < ROLE_Authority)
	{
		// UE_LOG(LogTemp, Warning, TEXT("TakeDamage called on client for %s, ignoring."), *GetName());
		return 0.f;
	}

	// UE_LOG(LogTemp, Warning, TEXT("%s Taking Damage: %.2f from %s (Instigator: %s)"),
	//     *GetName(),
	//     DamageAmount,
	//     DamageCauser ? *DamageCauser->GetName() : TEXT("None"),
	//     EventInstigator ? *EventInstigator->GetName() : TEXT("None")
	// );


	// Neprijimaj zranenie, ak uz si mrtvy
	if (CurrentHealth <= 0.f)
	{
		return 0.f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage > 0.f)
	{
		CurrentHealth -= ActualDamage;
		// UE_LOG(LogTemp, Warning, TEXT("%s Health: %.2f"), *GetName(), CurrentHealth);
		if (CurrentHealth <= 0.f)
		{
			// UE_LOG(LogTemp, Warning, TEXT("%s Died."), *GetName());
			Die(EventInstigator);
		}
		else
		{
			// Moznost: Prehraj zvuk bolesti/efekt cez Multicast RPC, ak treba
		}
	}
	return ActualDamage;
}

// Logika smrti postavy. Volane iba na serveri. Informuje GameMode, spusti ragdoll a nastavi znicenie postavy.
void AMyCharacter::Die(AController* KillerController)
{
	// Uisti sa, ze logika smrti bezi iba na serveri
	if (GetLocalRole() == ROLE_Authority)
	{
		// UE_LOG(LogTemp, Warning, TEXT("Die function executing on server for %s"), *GetName());

		// Povedz hernemu modu (pravidlam hry), ze doslo k zabitiu
		AMyGameMode* GM = GetWorld()->GetAuthGameMode<AMyGameMode>();
		if (GM)
		{
			// UE_LOG(LogTemp, Warning, TEXT("Notifying GameMode about kill. Victim: %s, Killer: %s"),
			//     Controller ? *Controller->GetName() : TEXT("None"),
			//     KillerController ? *KillerController->GetName() : TEXT("None")
			// );
			GM->PlayerKilled(GetController(), KillerController);
		}
		else
		{
			// UE_LOG(LogTemp, Warning, TEXT("Could not get MyGameMode on server during Die."));
		}

		// --- Efekty pri smrti ---
		Multicast_Ragdoll(); // Spusti "hadrovu babiku" (ragdoll) u vsetkych hracov

		// Vypni koliziu (narazanie) na kapsuli
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCharacterMovement()->DisableMovement(); // Zastav pohyb

		// Odpoj ovladac a vypni ovladanie (dolezite!)
		if (Controller)
		{
			Controller->UnPossess();
			// Moznost: Vypni ovladanie na ovladaci, ak stale existuje (napr. umela inteligencia)
			// APlayerController* PC = Cast<APlayerController>(Controller);
			// if(PC) PC->DisableInput(PC);
		}

		// Nastav cas, po ktorom postavicka zmizne z hry (s pouzitim nastavitelnej premennej)
		SetLifeSpan(RagdollLifeSpan);
	}
	else
	{
		// UE_LOG(LogTemp, Warning, TEXT("Die function called on client for %s, ignoring server logic."), *GetName());
	}
}

// Funkcia volana na vsetkych klientoch (a serveri) na spustenie ragdoll efektu.
void AMyCharacter::Multicast_Ragdoll_Implementation()
{
	// UE_LOG(LogTemp, Warning, TEXT("Multicast_Ragdoll executing on %s"), *GetName());
	USkeletalMeshComponent* CharacterMesh = GetMesh(); // Pouzi GetMesh(), co je standard pre postavu
	if (CharacterMesh)
	{
		// UE_LOG(LogTemp, Warning, TEXT("Enabling physics on mesh for %s"), *GetName());
		// Nastav kolizny profil pre ragdoll pouzitim premennej z editora
		CharacterMesh->SetCollisionProfileName(RagdollCollisionProfileName); // Pouzi nastavitelny nazov profilu
		CharacterMesh->SetSimulatePhysics(true);
		CharacterMesh->SetOwnerNoSee(false); // Uisti sa, ze model je teraz viditelny
	}
	else
	{
		// UE_LOG(LogTemp, Warning, TEXT("Could not get mesh for ragdoll on %s"), *GetName());
	}

	// Vypni koliziu kapsule aj u klientov, aby to bolo rovnake ako na serveri
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}


// Pomocna funkcia na aktualizaciu zdravia na HUD pre lokalneho hraca.
void AMyCharacter::UpdateHUDHealth()
{
	// Ziskaj lokalneho ovladaca
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC && PC->IsLocalController())
	{
		// Ziskaj HUD
		AMyHUD* HUD = Cast<AMyHUD>(PC->GetHUD());
		if (HUD)
		{
			// Aktualizuj zdravie
			HUD->UpdateHealth(CurrentHealth, MaxHealth);
		}
		// else { UE_LOG(LogTemp, Warning, TEXT("UpdateHUDHealth: Could not get MyHUD for %s"), *GetName()); }
	}
	// else { // Toto sa moze stat na serveri alebo u neovladanych postaviciek, co je v poriadku
	//     // UE_LOG(LogTemp, Log, TEXT("UpdateHUDHealth: Not a local controller for %s"), *GetName());
	// }
}
