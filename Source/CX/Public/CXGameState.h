
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CXGameState.generated.h"

class ACXPlayerState;

// 게임 진행 단계
UENUM(BlueprintType)
enum class ECXGamePhase : uint8
{
    Waiting,    // 대기 중
    Playing,    // 게임 진행 중
    Finished    // 게임 종료
};

UCLASS()
class CX_API ACXGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ACXGameState();

    // 서버에서 복제할 변수 등록
    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps
    ) const override;

    // 현재 게임 상태 반환
    UFUNCTION(BlueprintPure, Category = "Game State")
    ECXGamePhase GetGamePhase() const { return CurrentPhase; }

    // 남은 시간 반환
    UFUNCTION(BlueprintPure, Category = "Game State")
    int32 GetRemainingTime() const { return RemainingTime; }

    // 우승자 목록을 블루프린트에서 사용할 수 있도록 배열로 반환
    UFUNCTION(BlueprintPure, Category = "Game State")
    TArray<ACXPlayerState*> GetWinners() const
    {
        TArray<ACXPlayerState*> Result;

        for (ACXPlayerState* Winner : Winners)
        {
            Result.Add(Winner);
        }

        return Result;
    }

    // 서버에서 게임 상태 변경
    void SetGamePhase(ECXGamePhase NewPhase);

    // 서버에서 남은 시간 변경
    void SetRemainingTime(int32 NewTime);

    // 서버에서 우승자 목록 변경
    void SetWinners(const TArray<ACXPlayerState*>& NewWinners);

protected:

    // 모든 플레이어에게 복제할 현재 게임 상태
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    ECXGamePhase CurrentPhase = ECXGamePhase::Waiting;

    // 모든 플레이어에게 복제할 남은 시간
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    int32 RemainingTime = 0;

    // 공동 우승을 지원하는 우승자 목록
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    TArray<TObjectPtr<ACXPlayerState>> Winners;
};
