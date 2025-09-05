// Fill out your copyright notice in the Description page of Project Settings.


#include "PunchAbilityDamageApply.h"
#include "../MainCharacter.h"
#include "Kismet/GameplayStatics.h"

void UPunchAbilityDamageApply::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (!MeshComp) return;

	AMainCharacter* MeleeChar = Cast<AMainCharacter>(UGameplayStatics::GetPlayerCharacter(MeshComp->GetWorld(), 0));

	if (MeleeChar && !MeleeChar->CurrentWeapon->bMeleeBlocked) {
		MeleeChar->DrawMeleeEnd();
	}
}
