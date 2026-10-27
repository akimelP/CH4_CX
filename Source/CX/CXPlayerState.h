#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CXPlayerState.generated.h"

// PlayerState 값이 변경되었음을 UI에 알리는 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerStatusChanged);

UCLASS()
class CX_API ACXPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    // PlayerState 생성자
    ACXPlayerState();

    // 서버에서 설정한 변수를 클라이언트로 복제
    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps
    ) const override;

    // 서버에서 플레이어의 코인을 증가시키는 함수
    UFUNCTION(BlueprintCallable, Category = "Coins")
    void AddCoins(int32 Amount);

    // 서버에서 플레이어의 코인을 차감하는 함수
    // 차감에 성공하면 true 반환
    UFUNCTION(BlueprintCallable, Category = "Coins")
    bool SpendCoins(int32 Amount);

    // 현재 보유한 코인을 반환하는 함수
    UFUNCTION(BlueprintPure, Category = "Coins")
    int32 GetCoins() const;

    // 서버에서 추가 최대 속도를 증가시키는 함수
    UFUNCTION(BlueprintCallable, Category = "Speed")
    void AddBonusMaxSpeed(float Amount);

    // 현재 추가 최대 속도를 반환하는 함수
    UFUNCTION(BlueprintPure, Category = "Speed")
    float GetBonusMaxSpeed() const;

    // 블루프린트 UI가 값 변경 알림을 받을 수 있도록 공개
    UPROPERTY(BlueprintAssignable, Category = "PlayerState|UI")
    FOnPlayerStatusChanged OnPlayerStatusChanged;

protected:
    // 코인이 복제되면 OnRep_Coins 함수 호출
    UPROPERTY(ReplicatedUsing = OnRep_Coins, BlueprintReadOnly, Category = "Coins")
    int32 Coins = 0;

    // 가속력 보너스가 복제되면 OnRep_BonusMaxSpeed 함수 호출
    UPROPERTY(ReplicatedUsing = OnRep_BonusMaxSpeed, BlueprintReadOnly, Category = "Speed")
    float BonusMaxSpeed = 0.0f;

    // 코인의 복제된 값이 갱신되면 호출
    UFUNCTION()
    void OnRep_Coins();

    // 가속력 보너스의 복제된 값이 갱신되면 호출
    UFUNCTION()
    void OnRep_BonusMaxSpeed();
};