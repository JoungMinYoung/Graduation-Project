// Copyright Epic Games, Inc. All Rights Reserved.

#include "GraduationProjectGameMode.h"
#include "GraduationProjectCharacter.h"
#include "UObject/ConstructorHelpers.h"

AGraduationProjectGameMode::AGraduationProjectGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
