// Copyright Epic Games, Inc. All Rights Reserved.

#include "GraduationProjectCharacter.h"
#include "GraduationProjectProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

//////////////////////////////////////////////////////////////////////////
// AGraduationProjectCharacter

void AGraduationProjectCharacter::ResetAttackTimer()
{
	// Timer has finished, so reset the timer handle
	TimerHandle.Invalidate();
}

void AGraduationProjectCharacter::Fire()
{
	if (GetController() == nullptr)
	{
		return;
	}

	if (!CanMove)
	{
		return;
	}

	// Try and fire a projectile
	if (ProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			APlayerController* PlayerController = Cast<APlayerController>(GetController());

			FRotator SpawnRotation(0.0f, 0.0f, 0.0f);

			switch (CurrentView)
			{
			case Views::FirstPerson:
				SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
				break;
			case Views::Top:
			case Views::Side:
				SpawnRotation = GetActorRotation();
				break;
			}

			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			const FVector SpawnLocation = GetActorLocation() + SpawnRotation.RotateVector(MuzzleOffset);

			switch (CurrentView)
			{
			case Views::Side:
				// ¿ìÃøÀ» ¹Ù¶óº½
				if (-90 <= GetActorRotation().Yaw && GetActorRotation().Yaw < 90)
				{
					SpawnRotation.Yaw = 0.0f;
				}
				// ÁÂÃøÀ» ¹Ù¶óº½
				else
				{
					SpawnRotation.Yaw = 180.0f;
				}
			}

			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			// Spawn the projectile at the muzzle
			switch (CurrentShotMode)
			{
			case ShotModes::Normal:
			case ShotModes::Wing:
			case ShotModes::Chaser:
			case ShotModes::Piercing:
				World->SpawnActor<AGraduationProjectProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
				break;
			case ShotModes::Shotgun:
				UProjectileMovementComponent* ProjectileMovement = nullptr;
				FVector NewVelocity = FVector(0.0f, 0.0f, 0.0f);
				float BuckShotAngle = 0.1f;

				ProjectileMovement = World->SpawnActor<AGraduationProjectProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams)->GetProjectileMovement();
				NewVelocity = FVector(1.0f, 0.0f, BuckShotAngle).GetSafeNormal() * ProjectileMovement->InitialSpeed;
				ProjectileMovement->SetVelocityInLocalSpace(NewVelocity);

				ProjectileMovement = World->SpawnActor<AGraduationProjectProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams)->GetProjectileMovement();
				NewVelocity = FVector(1.0f, -BuckShotAngle / 2 * sqrt(3), -BuckShotAngle / 2).GetSafeNormal() * ProjectileMovement->InitialSpeed;
				ProjectileMovement->SetVelocityInLocalSpace(NewVelocity);

				ProjectileMovement = World->SpawnActor<AGraduationProjectProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams)->GetProjectileMovement();
				NewVelocity = FVector(1.0f, BuckShotAngle / 2 * sqrt(3), -BuckShotAngle / 2).GetSafeNormal() * ProjectileMovement->InitialSpeed;
				ProjectileMovement->SetVelocityInLocalSpace(NewVelocity);

				break;
			}
		}
	}

	// Try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// Try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

AGraduationProjectCharacter::AGraduationProjectCharacter()
	: Hp(5),
	InitialDamage(1.0f),
	Damage(InitialDamage),
	InitialFireRate(1.0f),
	FireRate(InitialFireRate),
	InitialSpeed(500.0f),
	Speed(InitialSpeed),
	InitialFireRange(0.2f),
	FireRange(InitialFireRange),
	CurrentView(Views::FirstPerson),
	CurrentShotMode(ShotModes::Normal),
	CanMove(true),
	FireSound(nullptr),
	InputComponent(nullptr)
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	TurnRateGamepad = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));
}

void AGraduationProjectCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
}

//////////////////////////////////////////////////////////////////////////// Input

void AGraduationProjectCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	InputComponent = PlayerInputComponent;

	// Set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AGraduationProjectCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("PrimaryAction", IE_Pressed, this, &AGraduationProjectCharacter::OnPrimaryAction);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AGraduationProjectCharacter::Interact);
	PlayerInputComponent->BindAction("Interact", IE_Released, this, &AGraduationProjectCharacter::StopInteracting);

	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	// Bind movement events
	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &AGraduationProjectCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AGraduationProjectCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "Mouse" versions handle devices that provide an absolute delta, such as a mouse.
	// "Gamepad" versions are for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &AGraduationProjectCharacter::ControllerYawInput);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &AGraduationProjectCharacter::ControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &AGraduationProjectCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &AGraduationProjectCharacter::LookUpAtRate);
}

void AGraduationProjectCharacter::OnPrimaryAction()
{
	// Check if the timer is active
	if (GetWorldTimerManager().IsTimerActive(TimerHandle))
	{
		return; // If the timer is still active, don't do anything
	}

	// Trigger the OnItemUsed Event
	//OnUseItem.Broadcast();

	Fire();

	// Start the timer to prevent the player from attacking again before the attack speed time has passed
	GetWorldTimerManager().SetTimer(TimerHandle, this, &AGraduationProjectCharacter::ResetAttackTimer, 1 / FireRate, false);
}

void AGraduationProjectCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnPrimaryAction();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AGraduationProjectCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

void AGraduationProjectCharacter::MoveForward(float Value)
{
	if (!CanMove)
	{
		return;
	}

	if (Value != 0.0f)
	{
		switch (CurrentView)
		{
		case Views::FirstPerson:
			// add movement in that direction
			AddMovementInput(GetActorForwardVector(), Value);
			break;

		case Views::Top:
		case Views::Side:
			AddMovementInput(FVector(1.0f, 0.0f, 0.0f), Value);
			break;
		}
	}
}

void AGraduationProjectCharacter::MoveRight(float Value)
{
	if (!CanMove)
	{
		return;
	}

	if (Value != 0.0f)
	{
		switch (CurrentView)
		{
		case Views::FirstPerson:
			// add movement in that direction
			AddMovementInput(GetActorRightVector(), Value);
			break;

		case Views::Top:
			AddMovementInput(FVector(0.0f, 1.0f, 0.0f), Value);
			break;

		case Views::Side:
			break;
		}
	}
}

void AGraduationProjectCharacter::Jump()
{
	if (!CanMove)
	{
		return;
	}

	ACharacter::Jump();
}

void AGraduationProjectCharacter::ControllerYawInput(float Rate)
{
	if (!CanMove)
	{
		return;
	}

	APawn::AddControllerYawInput(Rate);
}

void AGraduationProjectCharacter::ControllerPitchInput(float Rate)
{
	if (!CanMove)
	{
		return;
	}

	APawn::AddControllerPitchInput(Rate);
}

void AGraduationProjectCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AGraduationProjectCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

bool AGraduationProjectCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AGraduationProjectCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &AGraduationProjectCharacter::EndTouch);

		return true;
	}
	
	return false;
}

void AGraduationProjectCharacter::setKeyMapping(Views NewView)
{
	InputComponent->AxisBindings.Reset();
	InputComponent->ClearActionBindings();

	// Set up gameplay key bindings
	check(InputComponent);

	// Bind fire event
	InputComponent->BindAction("PrimaryAction", IE_Pressed, this, &AGraduationProjectCharacter::OnPrimaryAction);

	switch (CurrentView)
	{
	case Views::FirstPerson:
		// Bind jump events
		InputComponent->BindAction("Jump", IE_Pressed, this, &AGraduationProjectCharacter::Jump);
		InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

		InputComponent->BindAction("Interact", IE_Pressed, this, &AGraduationProjectCharacter::Interact);
		InputComponent->BindAction("Interact", IE_Released, this, &AGraduationProjectCharacter::StopInteracting);

		// Enable touchscreen input
		EnableTouchscreenMovement(InputComponent);

		// Bind movement events
		InputComponent->BindAxis("Move Forward / Backward", this, &AGraduationProjectCharacter::MoveForward);
		InputComponent->BindAxis("Move Right / Left", this, &AGraduationProjectCharacter::MoveRight);

		// We have 2 versions of the rotation bindings to handle different kinds of devices differently
		// "Mouse" versions handle devices that provide an absolute delta, such as a mouse.
		// "Gamepad" versions are for devices that we choose to treat as a rate of change, such as an analog joystick
		InputComponent->BindAxis("Turn Right / Left Mouse", this, &AGraduationProjectCharacter::ControllerYawInput);
		InputComponent->BindAxis("Look Up / Down Mouse", this, &AGraduationProjectCharacter::ControllerPitchInput);
		InputComponent->BindAxis("Turn Right / Left Gamepad", this, &AGraduationProjectCharacter::TurnAtRate);
		InputComponent->BindAxis("Look Up / Down Gamepad", this, &AGraduationProjectCharacter::LookUpAtRate);
		break;

	case Views::Top:

		InputComponent->BindAction("Interact", IE_Pressed, this, &AGraduationProjectCharacter::Interact);
		InputComponent->BindAction("Interact", IE_Released, this, &AGraduationProjectCharacter::StopInteracting);

		// Enable touchscreen input
		EnableTouchscreenMovement(InputComponent);

		// Bind movement events
		InputComponent->BindAxis("Move Forward / Backward", this, &AGraduationProjectCharacter::MoveForward);
		InputComponent->BindAxis("Move Right / Left", this, &AGraduationProjectCharacter::MoveRight);
		break;

	case Views::Side:
		// Bind jump events
		InputComponent->BindAction("Jump", IE_Pressed, this, &AGraduationProjectCharacter::Jump);
		InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

		// Enable touchscreen input
		EnableTouchscreenMovement(InputComponent);

		// Bind movement events
		InputComponent->BindAxis("Move Right / Left", this, &AGraduationProjectCharacter::MoveForward);

		break;
	}
}

uint8 AGraduationProjectCharacter::setHpByItem(uint8 NewHp)
{
	if (NewHp == 0 || 250 < NewHp)
	{
		Hp = 1;
		return Hp;
	}

	Hp = NewHp;
	return Hp;
}

float AGraduationProjectCharacter::setSpeed(float NewSpeed)
{
	return GetCharacterMovement()->MaxWalkSpeed = Speed = NewSpeed;
}