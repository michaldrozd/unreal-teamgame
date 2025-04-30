// Fill this file with the following content:
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h" // Required for Enhanced Input
#include "Net/UnrealNetwork.h" // Required for replication
#include "MyCharacter.generated.h"

// Povieme pocitacu, ze tato vec existuje, ale detaily su inde
class UInputMappingContext;
// Povieme pocitacu, ze tato vec existuje, ale detaily su inde
class UInputAction;
// Povieme pocitacu, ze tato vec existuje, ale detaily su inde
class UCameraComponent;
// Povieme pocitacu, ze tieto assety existuju
class USoundBase;
class UParticleSystem; // Alebo UNiagaraSystem, ak pouzivas Niagara

UCLASS(config=Game)
class AMyCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Toto urcuje, ktore tlacidla co robia */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Toto je akcia pre strelbu */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> FireAction;

	/** Toto je akcia pre skok - ak pouzivame novy system ovladania */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> JumpAction;

	/** Toto je akcia pre pohyb - ak pouzivame novy system ovladania */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	/** Toto je akcia pre pozeranie sa okolo - ak pouzivame novy system ovladania */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	/** Zvuk, ktory sa prehra pri vystrele */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon FX")
	TObjectPtr<USoundBase> FireSound;

	/** Efekt castic (napr. zablesk), ktory sa zobrazi pri vystrele */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon FX")
	TObjectPtr<UParticleSystem> MuzzleFlashFX; // Alebo UNiagaraSystem

public:
	AMyCharacter();

	/** Kamera, cez ktoru sa pozeras v hre */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Volane, ked stlacis tlacidla pre pohyb */
	void Move(const FInputActionValue& Value);

	/** Volane, ked hybes mysou a pozeras sa */
	void Look(const FInputActionValue& Value);

	/** Volane, ked stlacis tlacidlo pre strelbu */
	void StartFire(const FInputActionValue& Value); // Upravene pre Enhanced Input

	// Funkcie pre ovladanie postavicky
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	// Koniec funkcii pre ovladanie postavicky

	// Zdravie postavicky
	UPROPERTY(EditDefaultsOnly, Category = "Health", ReplicatedUsing = OnRep_Health)
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, Category = "Health", ReplicatedUsing = OnRep_Health)
	float CurrentHealth;

	UFUNCTION()
	void OnRep_Health();

	// Spracovanie, ked ta niekto trafi
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// Co sa stane, ked zomries
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void Die(AController* KillerController);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Ragdoll();

	// Dlzka trvania ragdoll efektu a zivotnosti mrtvej postavicky
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay")
	float RagdollLifeSpan = 5.0f;

	// Nazov kolizneho profilu pre Ragdoll, nastavy sa v editore
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay")
	FName RagdollCollisionProfileName = TEXT("Ragdoll"); // Predvoleny nazov, uisti sa, ze profil existuje!

	// Nastavenia zbrane
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float FireRate = 0.1f; // Ako casto mozes strielat (v sekundach)

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float BaseDamage = 20.0f; // Zakladne poskodenie jednej strely

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float WeaponRange = 10000.0f; // Dosah strely

	// Interna premenna na sledovanie casu posledneho vystrelu
	float LastFireTime;

	// Veci pre strelbu
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Fire();

	// Funkcia na prehratie efektov strelby u vsetkych hracov
	UFUNCTION(NetMulticast, Unreliable) // Unreliable je OK pre kozmeticke efekty
	void Multicast_PlayFireEffects();

	// Posielanie informacii ostatnym hracom
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// Pomocna funkcia na zobrazenie informacii na obrazovke hraca
	void InitializeHUD();
	// Pomocna funkcia na aktualizaciu zdravia na HUD
	void UpdateHUDHealth();

public:
	/** Vrati kameru, cez ktoru sa pozeras */
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }
};
