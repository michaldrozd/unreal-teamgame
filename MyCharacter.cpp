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
#include "GameFramework/PlayerController.h" // For disabling input
#include "Components/SkeletalMeshComponent.h" // For ragdoll mesh access

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
	// Poznamka: Maximalne zdravie netreba posielat ostatnym, ak sa meni len v editore
}

// Volane u klientov, ked sa zmeni premenna CurrentHealth (ked server posle aktualizaciu).
void AMyCharacter::OnRep_Health()
{
	// Moznost: Reakcia na zmenu zdravia u hraca (napr. zvuk bolesti, zmena na obrazovke)
	// Pozor, aby sa to nerobilo dvakrat, ak to uz riesi iny kod (PlayerState/GameState)
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
		// Ak potrebujes ukazat zdravie priamo z tejto postavicky
		// Napriklad: Zavolaj funkciu v PlayerState, ktora aktualizuje obrazovku
		// PS->ClientInitializeHUD();
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

// Volane, ked hrac stlaci tlacidlo pre strelbu. Spusti Server_Fire.
void AMyCharacter::StartFire(const FInputActionValue& Value) // Upravene pre Enhanced Input
{
	// Hned zobraz efekt strelby u hraca (napr. zablesk pri hlavni)
	// UE_LOG(LogTemp, Warning, TEXT("StartFire Called (Client or Server Local)"));
	Server_Fire(); // Zavolaj funkciu na serveri
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

	MyController->GetPlayerViewPoint(Start, Rot);
	FVector ForwardVector = Rot.Vector(); // Pouzi smer otocenia ovladaca
	FVector End = Start + (ForwardVector * 10000.0f); // Uprav dosah strely podla potreby
	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // Ignoruj sam seba (aby si sa netrafil)
	QueryParams.bTraceComplex = true;

	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams);

	// Moznost: Zavolaj funkciu pre efekty u vsetkych hracov, aj ked si nic netrafil
	// Multicast_PlayFireEffects(); // Urob toto, ak chces zvuky/efekty strelby u vsetkych

	if (bHit && Hit.GetActor())
	{
		// UE_LOG(LogTemp, Warning, TEXT("Server Fire Hit: %s"), *Hit.GetActor()->GetName());
		// 2. Skontroluj, ci trafena vec je ina postavicka hraca
		AMyCharacter* HitCharacter = Cast<AMyCharacter>(Hit.GetActor());
		if (HitCharacter && HitCharacter != this) // Uisti sa, ze je to postavicka a nie ty sam
		{
			// UE_LOG(LogTemp, Warning, TEXT("Applying Damage to: %s"), *HitCharacter->GetName());
			// 3. Daj zranenie
			float DamageAmount = 25.0f; // Priklad velkosti zranenia
			FPointDamageEvent DamageEvent(DamageAmount, Hit, ForwardVector, nullptr);
			// EventInstigator je ovladac (hrac alebo AI), ktory sposobil zranenie
			HitCharacter->TakeDamage(DamageAmount, DamageEvent, MyController, this);
		}
		// Moznost: Daj zranenie aj inym veciam, ktore sa daju rozbit
	}
	else
	{
		// UE_LOG(LogTemp, Warning, TEXT("Server Fire Missed or Hit Non-Actor"));
	}
}

// Urob funkciu Multicast_PlayFireEffects, ak treba (zvuk, zablesk)
// void AMyCharacter::Multicast_PlayFireEffects_Implementation() { ... }

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

		// Nastav cas, po ktorom postavicka zmizne z hry
		SetLifeSpan(5.0f);
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
		CharacterMesh->SetCollisionProfileName(TEXT("Ragdoll")); // Uisti sa, ze profil pre Ragdoll existuje a je spravne nastaveny
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
