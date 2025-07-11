// Fill out your copyright notice in the Description page of Project Settings.


#include "ParryAnimNotify.h"
#include "../MainCharacter.h"
#include "Kismet/GameplayStatics.h"

void UParryAnimNotify::BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	NotifyBegin(BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.SequenceAsset, BranchingPointPayload.NotifyEvent ? BranchingPointPayload.NotifyEvent->GetDuration() : 0.f);
	AMainCharacter* ParryChar = Cast<AMainCharacter>(UGameplayStatics::GetPlayerCharacter(BranchingPointPayload.SkelMeshComponent->GetWorld(), 0));

	if (ParryChar)
	{
		ParryChar->isParrying = true;
	}
	else
	{
		NotifyEnd(BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.SequenceAsset);
	}
}

void UParryAnimNotify::BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	AMainCharacter* ParryChar = Cast<AMainCharacter>(UGameplayStatics::GetPlayerCharacter(BranchingPointPayload.SkelMeshComponent->GetWorld(), 0));

	if (ParryChar)
	{
		ParryChar->isParrying = false;
	}

	NotifyEnd(BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.SequenceAsset);
	
}
