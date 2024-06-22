// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <string>
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Parse_WAV_To_Beatmap.generated.h"

using namespace std;

USTRUCT(BlueprintType)
struct FBeatmap {
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<double> Time_Since_Beginning_Arr;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> Notes_To_Spawn_Arr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int Highscore;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Song_Img_Path;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Song_WAV_Path;
};

/**
 * 
 */
UCLASS()
class OOP_API UMyBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
    UFUNCTION(BlueprintCallable, Category = "Custom Blueprints")
    static FBeatmap Parse_WAV_To_Beatmap(FString wav_file, double Instant_Rate, double Variance_Modifier, double C_Base);
    

};
