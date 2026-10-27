#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXPickupItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UParticleSystem;
class USoundBase;

// 아이템 종류 구분
UENUM(BlueprintType)
enum class ECXPickupType : uint8
{
    Coin,   // 코인
    Speed   // 추가 최대 속도
};

UCLASS()
class CX_API ACXPickupItem : public AActor
{
    GENERATED_BODY()

public:
    ACXPickupItem();

    // 모든 플레이어에게 아이템 획득 이펙트와 사운드 재생
    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayPickupEffect(FVector PickupLocation);

protected:
    virtual void BeginPlay() override;

    virtual void Tick(float DeltaTime) override;

    // 차량이 아이템에 닿았는지 감지하는 충돌 영역
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
    TObjectPtr<USphereComponent> PickupSphere;

    // 아이템의 외형을 표시하는 메시
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
    TObjectPtr<UStaticMeshComponent> PickupMesh;

    // 코인 획득 시 지급할 금액 (블루프린트에서 설정)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
    int32 CoinValue = 100;

    // 블루프린트에서 아이템 종류 선택
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
    ECXPickupType PickupType = ECXPickupType::Coin;

    // 속도 아이템 획득 시 가속력 보너스 3 증가
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
    float SpeedValue = 3.0f;

    // 아이템 회전 속도 (초당 90도)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup|Rotation")
    float RotationSpeed = 90.0f;

    // 아이템 획득 시 재생할 파티클
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Effects")
    TObjectPtr<UParticleSystem> PickupEffect;

    // 아이템 획득 시 재생할 사운드
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Effects")
    TObjectPtr<USoundBase> PickupSound;

    // 아이템 중복 획득 방지
    bool bIsCollected = false;

    // 획득 효과가 전달될 시간을 확보한 뒤 아이템 제거
    FTimerHandle DestroyTimerHandle;

    // 아이템을 최종 제거하는 함수
    void FinishPickup();

    // 차량이 아이템의 충돌 영역에 들어오면 호출
    UFUNCTION()
    void OnPickupOverlap(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );
};