// Fill out your copyright notice in the Description page of Project Settings.


#include "PunchAbilityNotifyState.h"
#include "../MainCharacter.h"
#include "Kismet/GameplayStatics.h"

void UPunchAbilityNotifyState::BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	NotifyBegin(BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.SequenceAsset, BranchingPointPayload.NotifyEvent ? BranchingPointPayload.NotifyEvent->GetDuration() : 0.f);
}

void UPunchAbilityNotifyState::BranchingPointNotifyTick(FBranchingPointNotifyPayload& BranchingPointPayload, float FrameDeltaTime)
{
	AMainCharacter* MeleeChar = Cast<AMainCharacter>(UGameplayStatics::GetPlayerCharacter(BranchingPointPayload.SkelMeshComponent->GetWorld(), 0));

	if (MeleeChar && !MeleeChar->CurrentWeapon->bMeleeBlocked) {
		MeleeChar->DrawAbilityMelee();
		NotifyTick(BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.SequenceAsset, FrameDeltaTime);
	}
	else {
		NotifyEnd(BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.SequenceAsset);
	}
}

void UPunchAbilityNotifyState::BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	AMainCharacter* MeleeChar = Cast<AMainCharacter>(UGameplayStatics::GetPlayerCharacter(BranchingPointPayload.SkelMeshComponent->GetWorld(), 0));
	if (MeleeChar) {
		MeleeChar->CurrentWeapon->MeleeTracePrevious.Empty();
		MeleeChar->CurrentWeapon->MeleeTraceCurrent.Empty();
		MeleeChar->CurrentWeapon->bMeleeBlocked = false;
	}

	NotifyEnd(BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.SequenceAsset);
}