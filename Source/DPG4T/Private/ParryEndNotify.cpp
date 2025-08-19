// Fill out your copyright notice in the Description page of Project Settings.


#include "ParryEndNotify.h"
#include "../MainCharacter.h"
#include "Kismet/GameplayStatics.h"

void UParryEndNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (!MeshComp) return;

	AMainCharacter* Char = Cast<AMainCharacter>(UGameplayStatics::GetPlayerCharacter(MeshComp->GetWorld(), 0));

	if (Char) {
		Char->PlayParryToIdle();
	}
}
