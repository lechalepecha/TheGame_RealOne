// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainCharacter.h"
#include "TheProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "Components/TimelineComponent.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "TP_WeaponComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "MeleeAnimNotifyState.h"

//#include "SightMeshComponent.h"
#include "Public/FPAnimInstance.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AMainCharacter

AMainCharacter::AMainCharacter()
{
	// Character doesnt have a rifle at start
	bHasWeapon = false;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(35.f, 96.0f);

	// default character movement values for responsive and weighty control
	GetCharacterMovement()->GravityScale = 1.5f;
	GetCharacterMovement()->MaxAcceleration = 3072.f;
	GetCharacterMovement()->BrakingFrictionFactor = 1.f;
	GetCharacterMovement()->PerchRadiusThreshold = 30.f;
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;
	GetCharacterMovement()->JumpZVelocity = 650.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 200.f;
	GetCharacterMovement()->AirControl = 0.275f;

	FP_Root = CreateDefaultSubobject<USceneComponent>(TEXT("FP_Root"));
	FP_Root->SetupAttachment(GetCapsuleComponent());

	Mesh_Root = CreateDefaultSubobject<USpringArmComponent>(TEXT("Mesh_Root"));
	Mesh_Root->SetupAttachment(FP_Root);
	Mesh_Root->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	Mesh_Root->TargetArmLength = 0;
	Mesh_Root->bDoCollisionTest = false;
	Mesh_Root->bUsePawnControlRotation = true;
	Mesh_Root->bInheritPitch = true;
	Mesh_Root->bInheritYaw = true;
	Mesh_Root->bInheritRoll = false;

	Offset_Root = CreateDefaultSubobject<USceneComponent>(TEXT("Offset_Root"));
	Offset_Root->SetupAttachment(Mesh_Root);
	Offset_Root->SetRelativeLocation(FVector(0.f, 0.f, -70.f));

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(Offset_Root);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Mesh1P->SetRelativeLocation(FVector(0.f, 0.f, -96.f));

	Cam_Root = CreateDefaultSubobject<USpringArmComponent>(TEXT("Cam_Root"));
	Cam_Root->SetupAttachment(FP_Root);
	Cam_Root->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	Cam_Root->TargetArmLength = 0;
	Cam_Root->bDoCollisionTest = false;
	Cam_Root->bUsePawnControlRotation = true;
	Cam_Root->bInheritPitch = true;
	Cam_Root->bInheritYaw = true;
	Cam_Root->bInheritRoll = false;

	Cam_Skel = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Cam_Skel"));
	Cam_Skel->SetupAttachment(Mesh1P, FName("head"));
	Cam_Skel->SetRelativeLocation(FVector(0.f, 0.f, -60.f));

	Magnet_Object = CreateDefaultSubobject<USceneComponent>("Magnet_Object");
	Magnet_Object->SetupAttachment(Mesh1P);
	Magnet_Object->SetRelativeLocation(FVector(158.f, -100.f, 0.f));

	PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));

	CrouchTL = CreateDefaultSubobject<UTimelineComponent>(FName("CrouchTL"));
	CrouchTL->SetTimelineLength(0.2f);
	CrouchTL->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	FOnTimelineFloat onCrouchTLCallback;
	onCrouchTLCallback.BindUFunction(this, FName{ TEXT("CrouchTLCallback") });
	CrouchAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("CrouchAlphaCurve"));
	FKeyHandle KeyHandle = CrouchAlphaCurve->FloatCurve.AddKey(0.f, 0.f);
	CrouchAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = CrouchAlphaCurve->FloatCurve.AddKey(0.2f, 1.f);
	CrouchAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	CrouchTL->AddInterpFloat(CrouchAlphaCurve, onCrouchTLCallback);

	DashCamTL = CreateDefaultSubobject<UTimelineComponent>(FName("DashCamTL"));
	DashCamTL->SetTimelineLength(0.12f);
	DashCamTL->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	FOnTimelineFloat onDashCamTLCallback;
	onDashCamTLCallback.BindUFunction(this, FName{ TEXT("DashCamUpdateTLCallback") });
	DashCamAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("DashCamAlphaCurve"));
	KeyHandle = DashCamAlphaCurve->FloatCurve.AddKey(0.f, 0.f);
	DashCamAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = DashCamAlphaCurve->FloatCurve.AddKey(0.15f, 1.f);
	DashCamAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = DashCamAlphaCurve->FloatCurve.AddKey(0.3f, 0.f);
	DashCamAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	DashCamTL->AddInterpFloat(DashCamAlphaCurve, onDashCamTLCallback);

	SprintTL = CreateDefaultSubobject<UTimelineComponent>(FName("SprintTL"));
	SprintTL->SetTimelineLength(0.2f);
	SprintTL->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	FOnTimelineFloat onSprintTLCallback;
	onSprintTLCallback.BindUFunction(this, FName{ TEXT("SprintTLCallback") });
	SprintAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("SprintAlphaCurve"));
	KeyHandle = SprintAlphaCurve->FloatCurve.AddKey(0.f, 1.f);
	SprintAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = SprintAlphaCurve->FloatCurve.AddKey(2.5f, 1.75f);
	SprintAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	SprintTL->AddInterpFloat(SprintAlphaCurve, onSprintTLCallback);
	FOnTimelineEvent onSprintTLFinished;
	onSprintTLFinished.BindUFunction(this, FName{ TEXT("FinishedSprintDelegate") });
	SprintTL->SetTimelineFinishedFunc(onSprintTLFinished);

	DipTL = CreateDefaultSubobject<UTimelineComponent>(FName("DipTL"));
	DipTL->SetTimelineLength(1.f);
	DipTL->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	FOnTimelineFloat onDipTLCallback;
	onDipTLCallback.BindUFunction(this, FName{ TEXT("DipTLCallback") });
	DipAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("DipAlphaCurve"));
	KeyHandle = DipAlphaCurve->FloatCurve.AddKey(0.f, 0.f);
	DipAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = DipAlphaCurve->FloatCurve.AddKey(0.2f, 0.95f);
	DipAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = DipAlphaCurve->FloatCurve.AddKey(0.63f, 0.12f);
	DipAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = DipAlphaCurve->FloatCurve.AddKey(1.f, 0.f);
	DipAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	DipTL->AddInterpFloat(DipAlphaCurve, onDipTLCallback);

	WalkingTL = CreateDefaultSubobject<UTimelineComponent>(FName("WalkingTL"));
	WalkingTL->SetTimelineLength(1.f);
	WalkingTL->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);
	WalkingTL->SetLooping(true);

	FOnTimelineFloat onWalkingLeftRightTLCallback;
	onWalkingLeftRightTLCallback.BindUFunction(this, FName{ TEXT("WalkLeftRightTLCallback") });
	WalkLeftRightAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("WalkLeftRightAlphaCurve"));
	KeyHandle = WalkLeftRightAlphaCurve->FloatCurve.AddKey(0.f, 0.f);
	WalkLeftRightAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkLeftRightAlphaCurve->FloatCurve.AddKey(0.25f, 0.5f);
	WalkLeftRightAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkLeftRightAlphaCurve->FloatCurve.AddKey(0.5f, 1.f);
	WalkLeftRightAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkLeftRightAlphaCurve->FloatCurve.AddKey(0.75f, 0.5f);
	WalkLeftRightAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkLeftRightAlphaCurve->FloatCurve.AddKey(1.f, 0.f);
	WalkLeftRightAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkingTL->AddInterpFloat(WalkLeftRightAlphaCurve, onWalkingLeftRightTLCallback);

	FOnTimelineFloat onWalkingUpDownTLCallback;
	onWalkingUpDownTLCallback.BindUFunction(this, FName{ TEXT("WalkUpDownTLCallback") });
	WalkUpDownAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("WalkUpDownAlphaCurve"));
	KeyHandle = WalkUpDownAlphaCurve->FloatCurve.AddKey(0.f, 0.f);
	WalkUpDownAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkUpDownAlphaCurve->FloatCurve.AddKey(0.3f, 1.f);
	WalkUpDownAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkUpDownAlphaCurve->FloatCurve.AddKey(0.5f, 0.f);
	WalkUpDownAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkUpDownAlphaCurve->FloatCurve.AddKey(0.8f, 1.f);
	WalkUpDownAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkUpDownAlphaCurve->FloatCurve.AddKey(1.f, 0.f);
	WalkUpDownAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkingTL->AddInterpFloat(WalkUpDownAlphaCurve, onWalkingUpDownTLCallback);

	FOnTimelineFloat onWalkingRollTLCallback;
	onWalkingRollTLCallback.BindUFunction(this, FName{ TEXT("WalkRollTLCallback") });
	WalkRollAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("WalkRollAlphaCurve"));
	KeyHandle = WalkRollAlphaCurve->FloatCurve.AddKey(0.f, 0.18f);
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkRollAlphaCurve->FloatCurve.AddKey(0.15f, 0.f);
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkRollAlphaCurve->FloatCurve.AddKey(0.4f, 0.5f);
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkRollAlphaCurve->FloatCurve.AddKey(0.65f, 1.f);
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkRollAlphaCurve->FloatCurve.AddKey(0.9f, 0.5f);
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = WalkRollAlphaCurve->FloatCurve.AddKey(1.f, 0.18f);
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkingTL->AddInterpFloat(WalkRollAlphaCurve, onWalkingRollTLCallback);

	FOnTimelineEvent footstepEvent;
	footstepEvent.BindUFunction(this, FName{ TEXT("WalkTLFootstepCallback") });
	WalkingTL->AddEvent(0.35f, footstepEvent);
	WalkingTL->AddEvent(0.85f, footstepEvent);

	FOnTimelineEvent updateWalkEvent;
	updateWalkEvent.BindUFunction(this, FName{ TEXT("WalkTLUpdateEvent") });
	WalkingTL->SetTimelinePostUpdateFunc(updateWalkEvent);

	MoveMode = ECustomMovementMode::Walking;

	SlideTL = CreateDefaultSubobject<UTimelineComponent>(FName("SlideTL"));
	SlideTL->SetTimelineLength(1.f);
	SlideTL->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	FOnTimelineFloat onSlideTLCallback;
	onSlideTLCallback.BindUFunction(this, FName{ TEXT("SlideTLCallback") });
	SlideAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("SlideAlphaCurve"));
	KeyHandle = SlideAlphaCurve->FloatCurve.AddKey(0.f, 1.f);
	SlideAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	KeyHandle = SlideAlphaCurve->FloatCurve.AddKey(1.f, 0.f);
	SlideAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	SlideTL->AddInterpFloat(SlideAlphaCurve, onSlideTLCallback);
	FOnTimelineEvent onSlideTLFinished;
	onSlideTLFinished.BindUFunction(this, FName{ TEXT("FinishedSlideDelegate") });
	SlideTL->SetTimelineFinishedFunc(onSlideTLFinished);
}

void AMainCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	if (UFPAnimInstance* fpAnimInstance = Cast<UFPAnimInstance>(Mesh1P->GetAnimInstance()))
	{
		FPAnimInstance = fpAnimInstance;
	}

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Ensure camera is set in blueprint class
	if (auto camera = Cast<UCameraComponent>(Cam_Skel->GetChildComponent(0)))
	{
		FirstPersonCameraComponent = camera;
		FirstPersonCameraComponent->PostProcessSettings.bOverride_VignetteIntensity = true;
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("The camera component is not set!"));
	}
	PhysicsHandle = FindComponentByClass<UPhysicsHandleComponent>();
	if (PhysicsHandle == nullptr)
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("The physics component is not set!"));

	}
	WalkingTL->Play();
}

void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//StartSprint();

	if (PhysicsHandle != nullptr)
	{
		FVector MagnetNewLoc = Magnet_Object->GetComponentLocation();
		PhysicsHandle->SetTargetLocation(MagnetNewLoc);
		//UE_LOG(LogTemp, Warning, TEXT("MagnetNewLoc is: %s"), *MagnetNewLoc.ToString());
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("PhysicsHandle is null!"));
		return;
	}
	
	//UE_LOG(LogTemplateCharacter, Error, TEXT("Velocity: %s"), *GetCharacterMovement()->Velocity.ToString());
}

//INPUT

void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMainCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AMainCharacter::StopMove);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMainCharacter::Look);

		// Crouch
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AMainCharacter::CustomCrouch);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AMainCharacter::ReleaseCrouch);

		// sprint
		//EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AMainCharacter::PressedSprint);

		// dash
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &AMainCharacter::StartDash);

		// arm input
		//EnhancedInputComponent->BindAction(MagnetAction, ETriggerEvent::Started, this, &AMainCharacter::CheckGrabObject);
		EnhancedInputComponent->BindAction(QuickMeleeAction, ETriggerEvent::Started, this, &AMainCharacter::PressedQuickMelee);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component!"), *GetNameSafe(this));
	}
}

void AMainCharacter::UpdateGroundMovementSpeed()
{
	float newWalkSpeed = FMath::Lerp(BaseWalkSpeed, (BaseWalkSpeed * 0.65f), CrouchAlpha);
	GetCharacterMovement()->MaxWalkSpeed = newWalkSpeed;
}

void AMainCharacter::UpdatePlayerCapsuleHeight()
{
	float newCapsuleHalfHeight = FMath::Lerp(StandHeight, CrouchHeight, CrouchAlpha);
	GetCapsuleComponent()->SetCapsuleHalfHeight(newCapsuleHalfHeight, true);
}

void AMainCharacter::CrouchTLCallback(float val)
{
	CrouchAlpha = val;

	if (CurrentWeapon != nullptr)
	{

		if (val < CrouchAlphaPrev)
		{
			// Возвращаем исходный разброс
			CurrentWeapon->MaxSpread = FMath::Lerp(CurrentWeapon->MinSpread, WeaponSpread, 1.0f - val);
		}
		// Если приседаем
		else
		{
			// Уменьшаем разброс
			CurrentWeapon->MaxSpread = FMath::Lerp(CurrentWeapon->MaxSpread, CurrentWeapon->MinSpread, val);
		}
	}

	CrouchAlphaPrev = val;

	switch (MoveMode)
	{
	case ECustomMovementMode::Walking:
		UpdateGroundMovementSpeed();
		break;
	case ECustomMovementMode::Crouching:
		UpdateGroundMovementSpeed();
		break;
	default:
		break;
	}

	UpdatePlayerCapsuleHeight();
}

void AMainCharacter::OnSprintTimerEnd()
{
	Sprinting = true;
	MoveMode = ECustomMovementMode::Sprinting;
	GetWorld()->GetTimerManager().ClearTimer(SprintTimerHandle);
	SprintTimerHandle.Invalidate();

	SprintTL->Play();
	UE_LOG(LogTemplateCharacter, Warning, TEXT("Timer ended start encreasing max walk speed"));

}

void AMainCharacter::SprintTLCallback(float val)
{
	SprintAlpha = val;
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * SprintAlpha;
	UE_LOG(LogTemplateCharacter, Error, TEXT("encreasing max walk speed"));

}

void AMainCharacter::FinishedSprintDelegate()
{
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * 1.75f;
	UE_LOG(LogTemplateCharacter, Error, TEXT("encreasing max walk speed ended, character is running"));

}

void AMainCharacter::StartSprint()
{	
	if (GetCharacterMovement()->GetLastUpdateVelocity().Length() > 0.f && CanSprint && !Sprinting && !GetWorld()->GetTimerManager().IsTimerActive(SprintTimerHandle))
	{
		
		switch (MoveMode)
		{
		case ECustomMovementMode::Walking:
			//GetFPAnimInstance()->SetSprintBlendOutTime(GetFPAnimInstance()->BaseSprintBlendOutTime);
			//MoveMode = ECustomMovementMode::Sprinting;

			GetWorld()->GetTimerManager().SetTimer(SprintTimerHandle, this, &AMainCharacter::OnSprintTimerEnd, 5.f, false);
			UE_LOG(LogTemplateCharacter, Warning, TEXT("Character started moving, start Timer to start running"));
			break;

		case ECustomMovementMode::Crouching:
			break;
		default:
			break;
		}

	}
	else
	{
		if (GetCharacterMovement()->GetLastUpdateVelocity().Length() == 0.f) 
		{
			GetWorld()->GetTimerManager().ClearTimer(SprintTimerHandle);
			SprintTimerHandle.Invalidate();
		}

	}
}

void AMainCharacter::StartDash()
{
	if (DashesLeft != 0 && !isDashing)
	{
		GetWorldTimerManager().SetTimer(DashTime, this, &AMainCharacter::EndDash, 0.25f, true);
		
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

		FVector CurrentVelocity = FVector(
			GetCharacterMovement()->Velocity.X,
			GetCharacterMovement()->Velocity.Y,
			GetCharacterMovement()->Velocity.Z);

		if (GetCharacterMovement()->Velocity == FVector(0.f, 0.f, 0.f))
		{
			CurrentVelocity = FVector(
				GetActorForwardVector().X * 500,
				GetActorForwardVector().Y * 500,
				0.f);
		}

		GetCharacterMovement()->GravityScale = 0.f;
		GetCharacterMovement()->AirControl = 0;
		GetCharacterMovement()->BrakingFrictionFactor = 1.5f;
		GetCharacterMovement()->GroundFriction = 7.f;
		GetCharacterMovement()->FallingLateralFriction = 9.0f;

		GetController()->SetIgnoreMoveInput(true);

		LastVelocity = FVector(CurrentVelocity.X, CurrentVelocity.Y, 0.f);

		DashDirection = FVector(LastVelocity.X * DashStrenght, LastVelocity.Y * DashStrenght, 0);

		DashDirection.X = FMath::Clamp(DashDirection.X, -10000.f, 10000.f);
		DashDirection.Y = FMath::Clamp(DashDirection.Y, -10000.f, 10000.f);
	
		UE_LOG(LogTemplateCharacter, Error, TEXT("Velocity: %s"), *DashDirection.ToString());

		GetCharacterMovement()->Velocity = FVector(DashDirection.X, DashDirection.Y, 0);

		DashCamTL->PlayFromStart();

		isDashing = true;

		UGameplayStatics::PlaySoundAtLocation(this, DashCue, GetActorLocation());
		MakeNoise(1.f, this, GetActorLocation());
	}
}

void AMainCharacter::EndDash()
{
	DashesLeft--;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	GetCharacterMovement()->AirControl = 0.275f;
	GetCharacterMovement()->GravityScale = 1.5f;
	GetCharacterMovement()->GroundFriction = 8.f;

	GetCharacterMovement()->BrakingFrictionFactor = 1.f;
	GetCharacterMovement()->FallingLateralFriction = 0.f;

	GetController()->SetIgnoreMoveInput(false);
	GetCharacterMovement()->Velocity = LastVelocity;

	if (DashesLeft != 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(CoyoteTimerHandle);
		CoyoteTimerHandle.Invalidate();
	}

	if (float remainingTime = GetWorld()->GetTimerManager().GetTimerRemaining(DashRollbackHandle); remainingTime > 0.f || GetCharacterMovement()->MovementMode != EMovementMode::MOVE_Falling)
	{
		GetWorldTimerManager().SetTimer(DashRollbackHandle, this, &AMainCharacter::DashRollbackEnded, DashRollback, false);
	}

	GetWorld()->GetTimerManager().ClearTimer(DashTime);
	DashTime.Invalidate();
	Dip(1.8f, 0.6f);
	isDashing = false;
}

void AMainCharacter::DashCamUpdateTLCallback(float val)
{
	DashCamAlpha = val;

	UCameraComponent* camera = GetFirstPersonCameraComponent();

	float NewCamFov = FMath::Lerp(90.f, 100.f, DashCamAlpha);
	camera->SetFieldOfView(NewCamFov);
}


void AMainCharacter::DashRollbackEnded()
{
	DashesLeft = DashesMax;


	GetWorld()->GetTimerManager().ClearTimer(DashRollbackHandle);
	DashRollbackHandle.Invalidate();
}

void AMainCharacter::CustomCrouch()
{
	CrouchKeyHeld = true;
	if (CurrentWeapon != nullptr)
	{
		WeaponSpread = CurrentWeapon->MaxSpread;
	}
	
	switch (MoveMode)
	{
	case ECustomMovementMode::Walking:
		MoveMode = ECustomMovementMode::Crouching;

		// Ensure the timer is cleared by using the timer handle
		GetWorld()->GetTimerManager().ClearTimer(UnCrouchTimerHandle);
		UnCrouchTimerHandle.Invalidate();
		CrouchTL->Play();
		UE_LOG(LogTemplateCharacter, Error, TEXT("You were walking now you crouching"));
		break;
	case ECustomMovementMode::Crouching:
		break;
	case ECustomMovementMode::Sprinting:
		if (GetCharacterMovement()->MovementMode != EMovementMode::MOVE_Falling)
		{
			Sliding();
			UE_LOG(LogTemplateCharacter, Error, TEXT("You were sprinting now you sliding"));
		}
		break;
	case ECustomMovementMode::Sliding:
		break;
	default:
		break;
	}
}

void AMainCharacter::ReleaseCrouch()
{
	CrouchKeyHeld = false;

	CustomUnCrouch();
}

void AMainCharacter::CustomUnCrouch()
{
	switch (MoveMode)
	{
	case ECustomMovementMode::Walking:
		break;
	case ECustomMovementMode::Crouching:

		GetWorldTimerManager().SetTimer(UnCrouchTimerHandle, this, &AMainCharacter::OnCheckCanStand, (1.f / 30.f), true);
		break;
	case ECustomMovementMode::Sprinting:
		break;
	case ECustomMovementMode::Sliding:
		break;
	default:
		break;
	}
}

void AMainCharacter::OnCheckCanStand()
{

	FVector sphereTraceLocation = FVector(GetActorLocation().X, GetActorLocation().Y, (GetActorLocation().Z + CrouchHeight));

	FVector SphereStart = FVector(GetActorLocation().X, GetActorLocation().Y, (GetActorLocation().Z + CrouchHeight));

	float lerpedHeight = FMath::Lerp(0.f, (StandHeight - CrouchHeight), CrouchAlpha);
	float scaledLerpedHeight = lerpedHeight * 1.1f;
	float sphereEndZ = (GetActorLocation().Z + CrouchHeight) + scaledLerpedHeight;
	FVector SphereEnd = FVector(GetActorLocation().X, GetActorLocation().Y, sphereEndZ);
	float sphereRadius = GetCapsuleComponent()->GetScaledCapsuleRadius() * 0.5f;
	FCollisionShape Sphere{ FCollisionShape::MakeSphere(sphereRadius) };
	FCollisionQueryParams Params = FCollisionQueryParams();
	Params.AddIgnoredActor(this);
	FHitResult HitResult;

	bool isStuck = GetWorld()->SweepSingleByChannel(HitResult, SphereStart, SphereEnd, FQuat::Identity, ECollisionChannel::ECC_Visibility, Sphere, Params);
	bool isFalling = GetCharacterMovement()->IsFalling();

	if (!isStuck || isFalling)
	{
		StandUp();
		GetWorld()->GetTimerManager().ClearTimer(UnCrouchTimerHandle);
		UnCrouchTimerHandle.Invalidate();
	}
}

void AMainCharacter::StandUp()
{
	MoveMode = ECustomMovementMode::Walking;
	// sequence 1
	CrouchTL->Reverse();

	// sequence 2
/*	if (SprintToggle)
	{
		ForceStartSprint();
	}*/
}

void AMainCharacter::CheckGrabObject()
{
	if (Magneted)
	{
		ThrowObject();
	}
	else
	{
		GrabObject();
	}
}

void AMainCharacter::ThrowObject()
{
	if (Magneted)
	{

		UPrimitiveComponent* GrabbedComponent = PhysicsHandle->GrabbedComponent;
		PhysicsHandle->ReleaseComponent();


		FVector ForwardVector = GetActorForwardVector();
		FVector Impulse = ForwardVector * 10000.f;

		GrabbedComponent->AddImpulse(Impulse, NAME_None, true);

		Magneted = false;

	}
}

void AMainCharacter::GrabObject()
{
	FVector Start = FirstPersonCameraComponent->GetComponentLocation(); // Используем позицию камеры
	// Получаем направление камеры
	FVector Forward = FirstPersonCameraComponent->GetForwardVector();   // Используем направление камеры
	// Вычисляем конечную точку трассировки
	FVector TraceEnd = Start + (Forward * 1000.f);

	// Настраиваем параметры коллизии
	FCollisionQueryParams Params = FCollisionQueryParams();
	Params.AddIgnoredActor(this);

	// Выполняем трассировку
	FHitResult HitResult;
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, TraceEnd, ECC_Visibility, Params);
	// Отрисовка линии трассировки (для отладки)
	//DrawDebugLine(GetWorld(), Start, TraceEnd, FColor::Blue, false, 5.0f);

	// Обработка попадания
	if (bHit)
	{
		UPrimitiveComponent* HitComponent = HitResult.GetComponent();

		// Проверяем, что PhysicsHandle инициализирован и объект симулирует физику
		if (PhysicsHandle && HitComponent->IsSimulatingPhysics())
		{
			UE_LOG(LogTemplateCharacter, Error, TEXT("Object is sinulating physics && PhysicsHandle != null"), *HitResult.GetActor()->GetName());

			PhysicsHandle->GrabComponentAtLocationWithRotation(HitComponent, NAME_None, HitResult.ImpactPoint, HitComponent->GetComponentRotation());
			Magneted = true;
		}

		// Логирование для отладки
		UE_LOG(LogTemplateCharacter, Error, TEXT("Hit an object, trying to grab: %s"), *HitResult.GetActor()->GetName());
	}
}

void AMainCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);

		
	}
}

void AMainCharacter::StopMove(const FInputActionValue& Value)
{
	CheckStopSprint(0.f);
}

void AMainCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	float LookScaleModifier = 1.f;

	if (CurrentWeapon)
	{
		LookScaleModifier *= FMath::Lerp(1.f, ADSSensitivityScale, CurrentWeapon->ADSAlpha);
	}
	FVector2D LookAxisVector = Value.Get<FVector2D>() * LookScaleModifier;

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AMainCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
		// change coyote time based on speed
		float normalizedSpeed = UKismetMathLibrary::NormalizeToRange(GetVelocity().Length(), 0.f, BaseWalkSpeed);
		float alpha = FMath::Clamp(normalizedSpeed, 0.f, 1.f);
		float lerpedValue = FMath::Lerp(0.25f, 1.f, alpha);
		float time = CoyoteTime * lerpedValue;
		GetWorldTimerManager().SetTimer(CoyoteTimerHandle, this, &AMainCharacter::CoyoteTimePassed, time, true);
	}
}

void AMainCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	LandingDip();

	JumpsLeft = JumpsMax;

	// sequence 1
	// On landing, clear coyote timer
	GetWorld()->GetTimerManager().ClearTimer(CoyoteTimerHandle);
	CoyoteTimerHandle.Invalidate();
	GetWorld()->GetTimerManager().ClearTimer(SprintTimerHandle);
	SprintTimerHandle.Invalidate();

	MakeNoise(1.f, this, GetActorLocation());


	// sequence 2
	if (CrouchKeyHeld && MoveMode == ECustomMovementMode::Sprinting)
	{
		ForceStartSlide();
	}

	GetWorldTimerManager().SetTimer(DashRollbackHandle, this, &AMainCharacter::DashRollbackEnded, DashRollback, false);
	
}

void AMainCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	JumpsLeft = FMath::Clamp(JumpsLeft - 1, 0, JumpsMax);
	Dip(5.f, 1.f);


	if (float remainingTime = GetWorld()->GetTimerManager().GetTimerRemaining(CoyoteTimerHandle); remainingTime > 0.f)
	{
		if (JumpCue != nullptr)
		{
			float normalizedSpeed = UKismetMathLibrary::NormalizeToRange(GetVelocity().Length(), 0.f, BaseWalkSpeed);
			UGameplayStatics::PlaySoundAtLocation(this, JumpCue, GetActorLocation());
			MakeNoise(1.f, this, GetActorLocation());

		}
	}

	// On jump, clear coyote timer
	GetWorld()->GetTimerManager().ClearTimer(CoyoteTimerHandle);
	CoyoteTimerHandle.Invalidate();
}

bool AMainCharacter::CanJumpInternal_Implementation() const
{
	bool canJump = Super::CanJumpInternal_Implementation();
	float remainingTime = GetWorld()->GetTimerManager().GetTimerRemaining(CoyoteTimerHandle);

	bool isTimerActive = GetWorld()->GetTimerManager().IsTimerActive(UnCrouchTimerHandle); // can't jump if there is an obstacle above the player
	bool isSlideTLActive = SlideTL->IsActive();
	bool selected = isSlideTLActive ? SlideTL->GetPlaybackPosition() > 0.25f : true;
	return (canJump || remainingTime > 0.f || JumpsLeft > 0) && (!isTimerActive && selected);
}

void AMainCharacter::CoyoteTimePassed()
{
	JumpsLeft -= 1;
}

void AMainCharacter::Dip(float Speed, float Strength)
{
	// set dip param
	DipTL->SetPlayRate(Speed);
	DipStrength = Strength;
	DipTL->PlayFromStart();
}

void AMainCharacter::DipTlCallback(float val)
{
	// update dip alpha
	DipAlpha = val * DipStrength;

	// update fp_root
	float lerpedZValue = FMath::Lerp(0.f, -10.f, DipAlpha);
	FVector newLocation = FVector(FP_Root->GetRelativeLocation().X, FP_Root->GetRelativeLocation().Y, lerpedZValue);
	FP_Root->SetRelativeLocation(newLocation);
}

void AMainCharacter::LandingDip()
{
	float lastZVelocity = GetCharacterMovement()->GetLastUpdateVelocity().Z;
	float ZVectorLength = FVector(0.f, 0.f, lastZVelocity).Length();
	float jumpZvelocity = GetCharacterMovement()->JumpZVelocity;
	float normalizedVelocity = UKismetMathLibrary::NormalizeToRange(ZVectorLength, 0.f, jumpZvelocity);
	float clampedVelocity = FMath::Clamp(normalizedVelocity, 0.f, 1.f);
	Dip(3.f, clampedVelocity);
	if (LandCue != nullptr)
	{
		float normalizedSpeed = UKismetMathLibrary::NormalizeToRange(GetVelocity().Length(), 0.f, BaseWalkSpeed);
		UGameplayStatics::PlaySoundAtLocation(this, LandCue, GetActorLocation(), clampedVelocity);
	}
}

void AMainCharacter::WalkLeftRightTLCallback(float val)
{
	WalkLeftRightAlpha = val;
}

void AMainCharacter::WalkUpDownTLCallback(float val)
{
	WalkUpDownAlpha = val;
}

void AMainCharacter::WalkRollTLCallback(float val)
{
	WalkRollAlpha = val;
}

void AMainCharacter::WalkTLFootstepCallback()
{
	if (MoveMode != ECustomMovementMode::Sliding && FootstepCue != nullptr)
	{
		float normalizedSpeed = UKismetMathLibrary::NormalizeToRange(GetVelocity().Length(), 0.f, BaseWalkSpeed);
		float volumeMultiplier = FMath::Lerp(0.2f, 1.f, normalizedSpeed);
		float pitchMultiplier = FMath::Lerp(0.8f, 1.f, normalizedSpeed);
		UGameplayStatics::PlaySoundAtLocation(this, FootstepCue, GetActorLocation(), volumeMultiplier, pitchMultiplier);
		if (MoveMode != ECustomMovementMode::Crouching)
		{
			MakeNoise(1.f, this, GetActorLocation());
		}		
	}

	if (MoveMode == ECustomMovementMode::Sprinting)
	{
		Dip(4.f, 0.25f);
	}
}

void AMainCharacter::WalkTLUpdateEvent()
{
	// update walk anim position
	float lerpedWalkAnimPosX = FMath::Lerp(-0.4f, 0.4f, WalkLeftRightAlpha);
	float lerpedWalkAnimPosZ = FMath::Lerp(-0.35f, 0.2f, WalkUpDownAlpha);
	WalkAnimPos = FVector(lerpedWalkAnimPosX, 0.f, lerpedWalkAnimPosZ);

	// update walk anim rotation
	float lerpedWalkAnimRotPitch = FMath::Lerp(1.f, -1.f, WalkRollAlpha);
	WalkAnimRot = FRotator(lerpedWalkAnimRotPitch, 0.f, 0.f);

	// get alpha of walking intensity
	float normalizedSpeed = UKismetMathLibrary::NormalizeToRange(GetVelocity().Length(), 0.f, BaseWalkSpeed);
	if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
		WalkAnimAlpha = 0.f;
	}
	else
	{
		WalkAnimAlpha = FMath::Clamp(normalizedSpeed, 0.f, 1.f); // had to clamp this because when sprinting, this would jump up beyond 1 and the footstep is too fast
	}

	float lerpedWalkAnimAlpha = FMath::Lerp(0.f, 1.65f, WalkAnimAlpha);
	WalkingTL->SetPlayRate(lerpedWalkAnimAlpha);

	// update location lag vars
	GetVelocityVars();

	// update look input vars
	GetLookInputVars(CamRotCurrent);

	// camera animation
	FVector camOffset;
	float camAnimAlpha;
	ProcCamAnim(camOffset, camAnimAlpha);
}

void AMainCharacter::GetVelocityVars()
{
	float velocityDotForwardVec = FVector::DotProduct(GetVelocity(), GetActorForwardVector());
	float velocityDotRightVec = FVector::DotProduct(GetVelocity(), GetActorRightVector());
	float velocityDotUpVec = FVector::DotProduct(GetVelocity(), GetActorUpVector());

	float Y = velocityDotForwardVec / (BaseWalkSpeed * -1.f);
	float X = velocityDotRightVec / BaseWalkSpeed;
	float Z = velocityDotUpVec / GetCharacterMovement()->JumpZVelocity * -1.f;

	FVector resultingVec = FVector(X, Y, Z);
	FVector scaledVec = resultingVec * 2.f;
	FVector ClampedVectorSize = scaledVec.GetClampedToSize(0.f, 4.f);

	float deltaTime = GetWorld()->DeltaTimeSeconds;
	float interpSpeed = (1.f / deltaTime) / 6.f;
	FVector interpedVec = FMath::VInterpTo(LocationLagPos, ClampedVectorSize, deltaTime, interpSpeed);
	LocationLagPos = interpedVec;

	interpSpeed = (1.f / deltaTime) / 12.f;
	FRotator targetRInterp = FRotator((LocationLagPos.Z * -2.f), 0.f, 0.f);
	FRotator interpedRot = FMath::RInterpTo(InAirTilt, targetRInterp, deltaTime, interpSpeed);
	InAirTilt = interpedRot;

	FVector targetVInterp = FVector((LocationLagPos.Z * -0.5f), 0.f, 0.f);
	FVector interpedInAirOffsetVec = FMath::VInterpTo(InAirOffset, targetVInterp, deltaTime, interpSpeed);
	InAirOffset = interpedInAirOffsetVec;
}

void AMainCharacter::GetLookInputVars(FRotator CamRotPrev)
{
	// Step 1: determining how much to offset the viewmodel based
	// on our current camera pitch
	FRotator deltaRotator = UKismetMathLibrary::NormalizedDeltaRotator(GetControlRotation(), GetActorRotation());
	float normalizedPitch = UKismetMathLibrary::NormalizeToRange(deltaRotator.Pitch, -90.f, 90.f);
	float lerpedY = FMath::Lerp(3.f, -3.f, normalizedPitch);
	float lerpedZ = FMath::Lerp(2.f, -2.f, normalizedPitch);
	PitchOffsetPos = FVector(0.f, lerpedY, lerpedZ);

	float normalizedFurther = UKismetMathLibrary::NormalizeToRange(normalizedPitch, 0.f, 0.5f);
	float clampedNormalizedPitch = FMath::Clamp(normalizedFurther, 0.f, 1.f);
	float lerpedClampedNormalizedPitch = FMath::Lerp(15.f, 0.f, clampedNormalizedPitch);
	FVector newRelativeLocation = FVector(lerpedClampedNormalizedPitch, FP_Root->GetRelativeLocation().Y, FP_Root->GetRelativeLocation().Z);
	FP_Root->SetRelativeLocation(newRelativeLocation);


	// Step 2: finding the rotation rate of our camera and smoothing
	// the result to use for our weapon sway
	CamRotCurrent = FirstPersonCameraComponent->GetComponentRotation();
	FRotator deltaCamRot = UKismetMathLibrary::NormalizedDeltaRotator(CamRotCurrent, CamRotPrev);
	float deltaCamRotPitch, deltaCamRotYaw, deltaCamRotRoll;
	UKismetMathLibrary::BreakRotator(deltaCamRot, deltaCamRotRoll, deltaCamRotPitch, deltaCamRotYaw);
	float pitchInverse = deltaCamRotPitch * -1.f;
	float clampedPitchInverse = FMath::Clamp(pitchInverse, -5.f, 5.f);
	float clampedYaw = FMath::Clamp(deltaCamRotYaw, -5.f, 5.f);
	FRotator newRotator = FRotator(0.f, clampedYaw, clampedPitchInverse);
	float deltaSeconds = GetWorld()->DeltaTimeSeconds;
	float weaponWeight = bHasWeapon ? FMath::Clamp(CurrentWeapon->WeaponSwaySpeed, 6.f, 80.f) : 6.f;
	float interpSpeed = (1.f / deltaSeconds) / weaponWeight;
	CamRotRate = UKismetMathLibrary::RInterpTo(CamRotRate, newRotator, deltaSeconds, interpSpeed);


	// Step 3: figuring out the amount to offset our viewmodel by,
	// in order to counteract the rotation of our weapon sway
	float normalizedRoll = UKismetMathLibrary::NormalizeToRange(CamRotRate.Roll, -5.f, 5.f);
	float lerpedRoll = FMath::Lerp(-10.f, 10.f, normalizedRoll);

	float normalizedYaw = UKismetMathLibrary::NormalizeToRange(CamRotRate.Yaw, -5.f, 5.f);
	float lerpedYaw = FMath::Lerp(-6.f, 6.f, normalizedYaw);
	CamRotOffset = FVector(lerpedYaw, 0.f, lerpedRoll);

}

void AMainCharacter::ProcCamAnim(FVector& CamOffsetArg, float& CamAnimAlphaArg)
{
	FTransform spine03Transform = Mesh1P->GetSocketTransform(FName("spine_03"));
	FTransform hand_rTransform = Mesh1P->GetSocketTransform(FName("hand_r"));
	FVector inversedTransformLocation = UKismetMathLibrary::InverseTransformLocation(spine03Transform, hand_rTransform.GetLocation());
	FVector differenceVec = PrevHandLoc - inversedTransformLocation;
	FVector swappedAxesVec = FVector(differenceVec.Y, differenceVec.Z, differenceVec.X);
	CamOffset = swappedAxesVec * FVector(-1.f, 1.f, -1.f);
	FVector multipliedVec = CamOffset * CamStrength;
	PrevHandLoc = inversedTransformLocation;

	UAnimInstance* meshAnimInstance = Mesh1P->GetAnimInstance();
	bool isAnyMontagePlaying = meshAnimInstance->IsAnyMontagePlaying();
	auto currentActiveMontage = meshAnimInstance->GetCurrentActiveMontage();
	bool isMontageActive = meshAnimInstance->Montage_IsActive(currentActiveMontage);
	float lowerSelectFloat = isMontageActive ? 1.f : 0.f;
	float upperSelectFloat = isAnyMontagePlaying ? lowerSelectFloat : 0.f;
	float deltaSeconds = GetWorld()->DeltaTimeSeconds;
	float interpSpeed = (1.f / deltaSeconds) / 24.f;
	FVector interpedVec = UKismetMathLibrary::VInterpTo(CamOffsetCurrent, multipliedVec, deltaSeconds, interpSpeed);
	CamOffsetCurrent = interpedVec.GetClampedToSize(0.f, 10.f);

	interpSpeed = (1.f / deltaSeconds) / 60.f;
	CamAnimAlpha = UKismetMathLibrary::FInterpTo(CamAnimAlpha, upperSelectFloat, deltaSeconds, interpSpeed);

	CamOffsetArg = CamOffsetCurrent;
	CamAnimAlphaArg = CamAnimAlpha;
}


void AMainCharacter::AttachWeapon_Implementation(UTP_WeaponComponent* Weapon)
{
	Weapon->SetOwningWeaponWielder(this);
	if (IWeaponWielderInterface::Execute_GetCurrentWeapon(this) == Weapon)
	{
		return;
	}
	Weapon->SetIsEquippingFalse();
	Weapon->IsEquipping = true;

	CurrentMeleeAnim = 0;

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	Weapon->AttachToComponent(GetMesh1P(), AttachmentRules, FName(TEXT("ik_hand_gun")));

	if (Weapon->FP_Material != nullptr)
	{
		Weapon->SetMaterial(0, Weapon->FP_Material);
	}

	/*if (Weapon->ScopeSightMesh != nullptr)
	{
		if (Weapon->ScopeSightMesh->FP_Material_0 != nullptr)
		{
			Weapon->ScopeSightMesh->SetMaterial(0, Weapon->ScopeSightMesh->FP_Material_0);
		}
		if (Weapon->ScopeSightMesh->FP_Material_1 != nullptr)
		{
			Weapon->ScopeSightMesh->SetMaterial(1, Weapon->ScopeSightMesh->FP_Material_1);
		}
		if (Weapon->ScopeSightMesh->FP_Material_2 != nullptr)
		{
			Weapon->ScopeSightMesh->SetMaterial(2, Weapon->ScopeSightMesh->FP_Material_2);
		}
	}*/

	// ensure current weapon for Character Actor is set to the new one before calling SetCurrentWeapon in anim bp
	SetCurrentWeapon(Weapon);

	// Try and play equip animation if specified
	//IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->WeaponChangeDelegate.AddUnique()
	IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->WeaponChangeDelegate.BindUFunction(Cast<UFPAnimInstance>(GetMesh1P()->GetAnimInstance()), FName("SetCurrentWeapon"));
	IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->WeaponChangeDelegate.Execute(IWeaponWielderInterface::Execute_GetCurrentWeapon(this));

	Weapon->Equip();
	if (GetFPAnimInstance())
	{
		GetFPAnimInstance()->Montage_Play(IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FPEquipAnimation, 1.f);

		FOnMontageBlendingOutStarted BlendOutDelegate;
		BlendOutDelegate.BindUObject(IWeaponWielderInterface::Execute_GetCurrentWeapon(this), &UTP_WeaponComponent::EquipAnimationBlendOut);
		GetFPAnimInstance()->Montage_SetBlendingOutDelegate(BlendOutDelegate, IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FPEquipAnimation);
	}

	Weapon->OnEquipDelegate.Broadcast(this, Weapon);

	// switch bHasWeapon so the animation blueprint can switch to current weapon idle anim
	SetHasWeapon(true);

	APlayerController* PCRef = Cast<APlayerController>(GetController());
	// Set up action bindings
	if (PCRef != nullptr)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PCRef->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PCRef->InputComponent))
		{
			// Fire
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AMainCharacter::PressedFire);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AMainCharacter::ReleasedFire);

			// Reload
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &AMainCharacter::PressedReload);

			// Switch Fire Mode
			EnhancedInputComponent->BindAction(SwitchFireModeAction, ETriggerEvent::Triggered, this, &AMainCharacter::PressedSwitchFireMode);

			// ADS
			EnhancedInputComponent->BindAction(ADSAction, ETriggerEvent::Triggered, this, &AMainCharacter::PressedADS);
			EnhancedInputComponent->BindAction(ADSAction, ETriggerEvent::Completed, this, &AMainCharacter::ReleasedADS);
		}

		Weapon->CanFire = true;
	}
}

void AMainCharacter::DetachWeapon_Implementation()
{
	// Check that the character is valid, and the currently set weapon is this object
	if (!GetHasWeapon() || IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->IsReloading || IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->IsStowing || IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->IsEquipping || GetWorld()->GetTimerManager().GetTimerRemaining(IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FireRateDelayTimerHandle) > 0)
	{
		return;
	}
	RemoveWeaponInputMapping();
	UTP_WeaponComponent* toBeDetached = IWeaponWielderInterface::Execute_GetCurrentWeapon(this);
	toBeDetached->IsStowing = true;
	//toBeDetached->ExitADS(true);
	toBeDetached->ADSTL->SetNewTime(0.f);
	toBeDetached->ADSTL->Stop();

	// Try and play stow animation
	SetHasWeapon(false);
	SetCurrentWeapon(nullptr);

	toBeDetached->WeaponStowDelegate.BindUFunction(Cast<UFPAnimInstance>(GetMesh1P()->GetAnimInstance()), FName("StowCurrentWeapon"));
	toBeDetached->WeaponStowDelegate.Execute();
	toBeDetached->Stow();
}

bool AMainCharacter::InstantDetachWeapon_Implementation()
{
	// Check that the character is valid, and the currently set weapon is this object
	if (!GetHasWeapon() || IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->IsReloading || GetWorld()->GetTimerManager().GetTimerRemaining(IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FireRateDelayTimerHandle) > 0)
	{
		return false;
	}
	RemoveWeaponInputMapping();
	UTP_WeaponComponent* toBeDetached = IWeaponWielderInterface::Execute_GetCurrentWeapon(this);
	toBeDetached->IsStowing = true;
	toBeDetached->ExitADS(true);

	SetHasWeapon(false);
	SetCurrentWeapon(nullptr);

	toBeDetached->WeaponStowDelegate.BindUFunction(Cast<UFPAnimInstance>(GetMesh1P()->GetAnimInstance()), FName("StowCurrentWeapon"));
	toBeDetached->WeaponStowDelegate.Execute();

	// Detach the weapon from the First Person Character
	FDetachmentTransformRules DetachmentRules(EDetachmentRule::KeepRelative, true);
	toBeDetached->DetachFromComponent(DetachmentRules);

	toBeDetached->CanFire = false;
	toBeDetached->IsStowing = false;
	toBeDetached->OnStowDelegate.Broadcast(this, toBeDetached);
	return true;
}

void AMainCharacter::OnWeaponFired_Implementation()
{
	// play FP Anim bp Fire() function for weapon recoil kick
	GetFPAnimInstance()->Fire();

	if (IsValid(CurrentWeapon->FireCamShake))
	{
		GetLocalViewingPlayerController()->ClientStartCameraShake(CurrentWeapon->FireCamShake);
	}
	
	// report noise for AI detection
	MakeNoise(1.f, this, CurrentWeapon->GetComponentLocation());
}

void AMainCharacter::OnWeaponReload_Implementation()
{
	if (CurrentWeapon->FPReloadAnimation != nullptr)
	{
		if (GetFPAnimInstance())
		{
			SetLeftHandIKState(false);
			GetFPAnimInstance()->Montage_Play(CurrentWeapon->FPReloadAnimation, 1.f);
		}
	}
}

void AMainCharacter::OnWeaponStopReloadAnimation_Implementation(float blendTime)
{
	if (CurrentWeapon->FPReloadAnimation != nullptr)
	{
		if (GetFPAnimInstance())
		{
			GetFPAnimInstance()->Montage_Stop(blendTime, CurrentWeapon->FPReloadAnimation);
			CurrentWeapon->Stop();
			CurrentWeapon->SetAnimationMode(CurrentWeapon->DefaultAnimationMode);
		}
	}
}

void AMainCharacter::OnADSTLUpdate_Implementation(float TLValue)
{

	if (!IsValid(CurrentWeapon) || CurrentWeapon->MPC_FP == nullptr)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("ADSTL call back called back"));


	CurrentWeapon->ADSAlpha = TLValue;
	CurrentWeapon->ADSAlphaLerp = FMath::Lerp(0.2f, 1.f, (1.f - CurrentWeapon->ADSAlpha));
	ADSAlpha = CurrentWeapon->ADSAlpha;
	float lerpedFOV = FMath::Lerp(CurrentWeapon->FOV_Base, CurrentWeapon->FOV_ADS, CurrentWeapon->ADSAlpha);
	UCameraComponent* camera = GetFirstPersonCameraComponent();
	camera->SetFieldOfView(lerpedFOV);
	float lerpedIntensity = FMath::Lerp(0.4f, 0.7f, CurrentWeapon->ADSAlpha);
	camera->PostProcessSettings.VignetteIntensity = lerpedIntensity;
	float lerpedFlatFov = FMath::Lerp(90.f, 25.f, CurrentWeapon->ADSAlpha);
	CurrentWeapon->MPC_FP_Instance->SetScalarParameterValue(FName("FOV"), lerpedFlatFov);
	FLinearColor OutColor;
	CurrentWeapon->MPC_FP_Instance->GetVectorParameterValue(FName("Offset"), OutColor);
	float lerpedB = FMath::Lerp(0.f, 30.f, CurrentWeapon->ADSAlpha);
	FLinearColor newColor = FLinearColor(OutColor.R, OutColor.G, lerpedB, OutColor.A);
	CurrentWeapon->MPC_FP_Instance->SetVectorParameterValue(FName("Offset"), newColor);

	float newSpeedMultiplier = FMath::Clamp(CurrentWeapon->ADSAlphaLerp, 0.55f, 1.f);
	GetCharacterMovement()->MaxWalkSpeed = GetBaseWalkSpeed() * newSpeedMultiplier;
	//CurrentWeapon->ScopeSightMesh->OnWeaponADSTLUpdateDelegate.Broadcast(ADSAlpha); // set opacity of scope if applicable
}

void AMainCharacter::RemoveWeaponInputMapping()
{
	APlayerController* PCRef = Cast<APlayerController>(GetController());
	if (PCRef != nullptr)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PCRef->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(FireMappingContext);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PCRef->InputComponent))
		{
			EnhancedInputComponent->ClearActionBindings();
		}
	}
}

void AMainCharacter::PressedFire()
{
	IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->IsWielderHoldingShootButton = true;
	if (GetIsMeleeing() || IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->IsReloading || IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->IsEquipping || IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->IsStowing || GetWorld()->GetTimerManager().GetTimerRemaining(IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FireRateDelayTimerHandle) > 0)
	{
		return;
	}
	if (!CanAct())
	{
		//GetFPAnimInstance()->SetSprintBlendOutTime(GetFPAnimInstance()->InstantSprintBlendOutTime);
		//ForceStopSprint();
	}
	if (GetFPAnimInstance()->Montage_IsPlaying(IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FPReloadAnimation))
	{
		GetFPAnimInstance()->Montage_Stop(0.f, IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FPReloadAnimation);
	}
	if (IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->CurrentMagazineCount <= 0)
	{
		IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->StopFire();
		IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->IsWielderHoldingShootButton = false;
		if (IsValid(IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->DryFireSound))
		{
			UGameplayStatics::SpawnSoundAttached(
				IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->DryFireSound,
				IWeaponWielderInterface::Execute_GetCurrentWeapon(this)
			);
		}

		if (IWeaponWielderInterface::Execute_GetRemainingAmmo(this) > 0)
		{
			IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->Reload();
			return;
		}
		return;
	}

	// Ensure the timer is cleared by using the timer handle
	GetWorld()->GetTimerManager().ClearTimer(IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FireRateDelayTimerHandle);
	IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FireRateDelayTimerHandle.Invalidate();
	IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->RecoilStart();


	if (ADSing)
	{
		if (IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->IsMeleeWeapon)
		{

		}
		else {
			GetFPAnimInstance()->Montage_Play(CurrentWeapon->FPFireADSAnimation, 1.25f);

		}
		

	}
	else
	{
		if (IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->IsMeleeWeapon)
		{

			GetFPAnimInstance()->Montage_Play(CurrentWeapon->FPSlashAnimations[CurrentMeleeAnim], 1.3f);

			CurrentMeleeAnim++;
			if (CurrentMeleeAnim > 3)
			{
				CurrentMeleeAnim = 2;
			}
			GetWorldTimerManager().SetTimer(IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FireRateDelayTimerHandle, IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FireDelay, false);
			return;
			
		}
		else {
			GetFPAnimInstance()->Montage_Play(CurrentWeapon->FPFireAnimation, 1.25f);
		}
	}

	switch (IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FireMode)
	{
	case EFireMode::Single:
		//Character->GetWorldTimerManager().SetTimer(FireRateDelayTimerHandle, this, &UTP_WeaponComponent::SingleFire, FireDelay, false, 0.f);
		IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->SingleFire();
		GetWorldTimerManager().SetTimer(IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FireRateDelayTimerHandle, IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FireDelay, false);
		break;
	case EFireMode::Alternate:
		IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->AlternateFire();
		GetWorldTimerManager().SetTimer(IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FireRateDelayTimerHandle, IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FireDelay, false);
		break;
	case EFireMode::Burst:
		GetWorldTimerManager().SetTimer(IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FireRateDelayTimerHandle, IWeaponWielderInterface::Execute_GetCurrentWeapon(this), &UTP_WeaponComponent::BurstFire, IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FireDelay, true, 0.f);
		break;
	case EFireMode::Auto:
		GetWorldTimerManager().SetTimer(IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FireRateDelayTimerHandle, IWeaponWielderInterface::Execute_GetCurrentWeapon(this), &UTP_WeaponComponent::FullAutoFire, IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->FireDelay, true, 0.f);
		break;
	default:
		break;
	}
}

void AMainCharacter::ReleasedFire()
{
	IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->IsWielderHoldingShootButton = false;
}

void AMainCharacter::CallMelleTraceDraw()
{
	IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->DrawMelee();
}

void AMainCharacter::CallMelleTraceEnd()
{
	
	IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->DrawMeleeEnd();
}

void AMainCharacter::PressedReload()
{
	if (GetIsMeleeing())
	{
		UE_LOG(LogTemp, Error, TEXT("Player was meleeing"));
		return;
	}
	if (!CanAct())
	{
		UE_LOG(LogTemp, Error, TEXT("Player could'nt act"));
		ForceStopSprint();
	}
	UE_LOG(LogTemp, Error, TEXT("Player trying to reload"));

	IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->Reload();
}

void AMainCharacter::PressedSwitchFireMode()
{
	if (!IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->CanSwitchFireMode)
	{
		return;
	}

	UE_LOG(LogTemp, Error, TEXT("switced modes"));

	IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->SwitchFireMode();
}

void AMainCharacter::PressedADS()
{
	if (!IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->IsMeleeWeapon)
	{
		IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->ADS_Held = true;
		//UE_LOG(LogTemp, Error, TEXT("ADSed"));

		ADSing = true;
		EnterADS();
	}
	else
	{
		if (!isParryingActive && !IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->ADS_Held)
		{
			isParryingActive = true;
			IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->ADS_Held = true;

			GetFPAnimInstance()->Montage_Play(CurrentWeapon->FPParryAnimation, 1.5f);
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &AMainCharacter::OnParryEnded);
			if (ParryCue != nullptr)
			{
				UGameplayStatics::PlaySoundAtLocation(this, ParryCue, GetActorLocation());
			}

			GetFPAnimInstance()->Montage_SetEndDelegate(EndDelegate, CurrentWeapon->FPParryAnimation);

		}
	}
	
}

bool AMainCharacter::ParryCheck()
{
	return isParrying;
}

void AMainCharacter::ParryRollbackEnded()
{
	isParryingActive = false;
	GetWorld()->GetTimerManager().ClearTimer(ParryTime);
	ParryTime.Invalidate();
}

void AMainCharacter::OnParryEnded(UAnimMontage* Montage, bool bInterrupted)
{
	GetWorldTimerManager().SetTimer(ParryTime, this, &AMainCharacter::ParryRollbackEnded, 0.1f, false);
}

void AMainCharacter::ParryTimerRestart()
{
	GetWorld()->GetTimerManager().ClearTimer(ParryTime);
	ParryTime.Invalidate();

	GetWorldTimerManager().SetTimer(ParryTime, this, &AMainCharacter::ParryRollbackEnded, 0.01f, false);
}

void AMainCharacter::EnterADS()
{
	if (GetIsMeleeing() || !IsValid(IWeaponWielderInterface::Execute_GetCurrentWeapon(this)) || IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->IsReloading || IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->IsStowing || IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->IsEquipping)
	{
		return;
	}
	if (GetFPAnimInstance()->Montage_IsActive(CurrentWeapon->FPReloadAnimation))
	{
		IWeaponWielderInterface::Execute_OnWeaponStopReloadAnimation(this, 0.f);
	}

	IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->ADSTL->SetPlayRate(FMath::Clamp(IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->ADS_Speed, 0.1f, 10.f));

	GetFPAnimInstance()->SetSprintBlendOutTime(0.25f);
	ForceStopSprint();
	IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->ADSTL->Play();
}

void AMainCharacter::ReleasedADS()
{
	ADSing = false;
	IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->ADS_Held = false;
	IWeaponWielderInterface::Execute_GetCurrentWeapon(this)->ADSTL->Reverse();
}

int32 AMainCharacter::GetRemainingAmmo_Implementation()
{
	EAmmoType ammoType = CurrentWeapon->AmmoType;
	switch (ammoType)
	{
	case EAmmoType::Primary:
		return PrimaryAmmoRemaining;
		break;
	case EAmmoType::Special:
		return SpecialAmmoRemaining;
		break;
	case EAmmoType::Heavy:
		return HeavyAmmoRemaining;
		break;
	default:
		return 0;
	}
}

int32 AMainCharacter::SetRemainingAmmo_Implementation(int32 NewValue)
{
	EAmmoType ammoType = CurrentWeapon->AmmoType;
	switch (ammoType)
	{
	case EAmmoType::Primary:
		PrimaryAmmoRemaining = NewValue;
		return PrimaryAmmoRemaining;
		break;
	case EAmmoType::Special:
		SpecialAmmoRemaining = NewValue;
		return SpecialAmmoRemaining;
		break;
	case EAmmoType::Heavy:
		HeavyAmmoRemaining = NewValue;
		return HeavyAmmoRemaining;
		break;
	default:
		return 0;
	}
}

void AMainCharacter::PressedQuickMelee()
{
	if (!IsValid(CurrentWeapon) || GetIsMeleeing())
	{
		return;
	}
	if (bHasWeapon && CurrentWeapon->GetIsReloading())
	{
		CurrentWeapon->CancelReload(0.25f);
	}
	if (!CanAct())
	{
		GetFPAnimInstance()->SetSprintBlendOutTime(GetFPAnimInstance()->InstantSprintBlendOutTime);
		ForceStopSprint();
	}
	if (ADSAlpha > 0.f)
	{
		//CurrentWeapon->ExitADS(true);
		CurrentWeapon->ADSTL->SetPlayRate(8.f);
		CurrentWeapon->ADSTL->Reverse();
	}

	SetIsMeleeing(true);
	SetLeftHandIKState(false);

	CurrentWeapon->ForceStopFire();
	CurrentWeapon->CancelReload(0.25f);
	GetFPAnimInstance()->Montage_Play(CurrentWeapon->FPMeleeAnimation);
	FOnMontageBlendingOutStarted BlendOutDelegate;
	BlendOutDelegate.BindUObject(this, &AMainCharacter::QuickMeleeAnimationBlendOut);
	GetFPAnimInstance()->Montage_SetBlendingOutDelegate(BlendOutDelegate, CurrentWeapon->FPMeleeAnimation);
}

void AMainCharacter::QuickMeleeAnimationBlendOut(UAnimMontage* animMontage, bool bInterrupted)
{
	StopMeleeing();
}

void AMainCharacter::StopMeleeing()
{
	SetIsMeleeing(false);
	SetLeftHandIKState(true);
}

/*void AMainCharacter::PressedSprint()
{
	if (GetIsMeleeing())
	{
		GetFPAnimInstance()->Montage_Stop(0.25f, CurrentWeapon->FPMeleeAnimation);
	}
	if (IsValid(CurrentWeapon))
	{
		if (CurrentWeapon->GetIsReloading())
		{
			CurrentWeapon->CancelReload(0.25f);
		}
		if (ADSAlpha > 0.f)
		{
			CurrentWeapon->ExitADS(true);
		}
	}

	SprintToggle = !SprintToggle;

	if (SprintToggle)
	{
		if (bHasWeapon)
		{
			CurrentWeapon->ForceStopFire();
		}
		StartSprint();
	}
	else
	{
		StopSprint();
	}
}

void AMainCharacter::ForceStartSprint()
{
	StartSprint();
}
*/
void AMainCharacter::ForceStopSlide()
{
	SlideTL->Stop();
	StopSlide();
}

void AMainCharacter::ForceUnCrouch()
{
	CustomUnCrouch();
}

void AMainCharacter::ForceStartSlide()
{
	Sliding();
}

void AMainCharacter::ForceStopSprint()
{
	StopSprint();
}

void AMainCharacter::CheckStopSprint(float InAxis)
{
	if (InAxis < 0.5f)
	{
		StopSprint();
	}
}

void AMainCharacter::StopSprint()
{
	Sprinting = false;
	GetWorld()->GetTimerManager().ClearTimer(SprintTimerHandle);
	SprintTimerHandle.Invalidate();
	//SprintCharge = 0.f;

	switch (MoveMode)
	{
	case ECustomMovementMode::Walking:
		break;
	case ECustomMovementMode::Crouching:
		break;
	case ECustomMovementMode::Sprinting:
		MoveMode = ECustomMovementMode::Walking;
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		if (CurrentWeapon != nullptr && CurrentWeapon->ADS_Held)
		{
			EnterADS();
		}

		break;
	case ECustomMovementMode::Sliding:
		break;
	default:
		break;
	}
}



void AMainCharacter::StartSlide()
{
	GetWorld()->GetTimerManager().ClearTimer(SprintTimerHandle);
	SprintTimerHandle.Invalidate();
	Sprinting = false;
	Dip(0.8f, 0.6f);

	if (SlideCue != nullptr)
	{
		UGameplayStatics::SpawnSoundAttached(SlideCue, RootComponent);
	}

	FVector lastInput = GetCharacterMovement()->GetLastInputVector();
	FVector lastUpdateVelocity = GetCharacterMovement()->GetLastUpdateVelocity();
	float VelocityVectorLength = (lastUpdateVelocity * FVector(1.f, 1.f, 0.f)).Length();

	SlideDirection = lastInput * VelocityVectorLength;
	MoveMode = ECustomMovementMode::Sliding;
	GetController()->SetIgnoreMoveInput(true);

	float crouchPlayRate = FMath::Lerp(1.f, 0.5f, 1.f);
	CrouchTL->SetPlayRate(crouchPlayRate);

	float slidePlayRate = FMath::Lerp(2.2f, 1.25f, 1.f);
	SlideTL->SetPlayRate(slidePlayRate);
	SlideTL->PlayFromStart();
}

void AMainCharacter::Sliding()
{
	StartSlide();
	GetWorld()->GetTimerManager().ClearTimer(UnCrouchTimerHandle);
	UnCrouchTimerHandle.Invalidate();
	CrouchTL->Play();
}

void AMainCharacter::StopSlide()
{
	MoveMode = ECustomMovementMode::Crouching;
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * 0.65;
	GetController()->SetIgnoreMoveInput(false);
	CrouchTL->SetPlayRate(1.f);

	//SprintCharge = 0.f;
	if (!CrouchKeyHeld)
	{
		ForceUnCrouch();
	}
}

void AMainCharacter::SlideTLCallback(float val)
{
	SlideAlpha = val;
	float A = BaseWalkSpeed * 0.65f;
	float B = BaseWalkSpeed * FMath::Lerp(0.65f, 5.0f, 1.f);
	float newMaxSpeed = FMath::Lerp(A, B, SlideAlpha);
	GetCharacterMovement()->MaxWalkSpeed = newMaxSpeed;
	GetCharacterMovement()->BrakingFrictionFactor = 1.f - SlideAlpha;

	AddMovementInput(SlideDirection, SlideAlpha, true);
}


void AMainCharacter::FinishedSlideDelegate()
{
	StopSlide();
}


// can the character perform actions?
// these actions cannot be performed while sprinting
bool AMainCharacter::CanAct()
{
	if (MoveMode != ECustomMovementMode::Sprinting)
	{
		return true;
	}
	return false;
}
