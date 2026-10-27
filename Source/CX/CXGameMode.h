
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CXGameMode.generated.h"

class APlayerStart;

UCLASS(abstract)
class ACXGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ACXGameMode();

protected:
    // 기존 차량 스폰 위치 선택
    virtual AActor* ChoosePlayerStart_Implementation(
        AController* Player
    ) override;

    // 새로운 플레이어가 접속하면 호출
    virtual void PostLogin(APlayerController* NewPlayer) override;

    // 게임 시작 전 대기 시간 (초)
    UPROPERTY(EditDefaultsOnly, Category = "Game Rules")
    int32 WaitingDuration = 30;

    // 대기 카운트다운 타이머
    FTimerHandle WaitingTimerHandle;

    // 첫 플레이어 접속 시 대기 시작
    void StartWaitingCountdown();

    // 1초마다 대기 시간 감소
    void UpdateWaitingCountdown();

    // 게임 종료 및 우승자 판정
    void FinishGame();

    // 게임 진행 시간 (초)
    UPROPERTY(EditDefaultsOnly, Category = "Game Rules")
    int32 PlayingDuration = 120;

    // 게임 진행 카운트다운 타이머
    FTimerHandle PlayingTimerHandle;

    // 게임 진행 카운트다운 시작
    void StartPlayingCountdown();

    // 1초마다 게임 진행 시간 감소
    void UpdatePlayingCountdown();

private:
    // 기존 차량 스폰 가능 여부 검사
    bool IsPlayerStartAvailable(const APlayerStart* PlayerStart) const;

    // 대기 카운트다운 중복 시작 방지
    bool bWaitingCountdownStarted = false;

    // 기존 차량 스폰 충돌 검사 범위
    UPROPERTY(EditDefaultsOnly)
    FVector VehicleSpawnCheckHalfExtent = FVector(250.0, 120.0, 80.0);

    // 기존 차량 스폰 충돌 검사 위치 보정
    UPROPERTY(EditDefaultsOnly)
    FVector VehicleSpawnCheckOffset = FVector(0.0, 0.0, 80.0);
};
