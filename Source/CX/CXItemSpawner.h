
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXItemSpawner.generated.h"

class ACXPickupItem;

UCLASS()
class CX_API ACXItemSpawner : public AActor
{
    GENERATED_BODY()

public:
    ACXItemSpawner();

protected:
    virtual void BeginPlay() override;

    // 원형 트랙의 안쪽 반지름 (cm)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawn")
    float InnerRadius = 3870.0f;

    // 원형 트랙의 바깥쪽 반지름 (cm)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawn")
    float OuterRadius = 5140.0f;

    // 트랙 표면에서 아이템을 띄울 높이 (cm)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawn")
    float SpawnHeight = 100.0f;

    // 시작 코인 블루프린트
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawn|Coin")
    TSubclassOf<ACXPickupItem> StartCoinClass;

    // 게임 시작 시 생성할 코인 개수
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawn|Coin")
    int32 StartCoinCount = 15;

    // 속도 아이템 블루프린트
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawn|Speed")
    TSubclassOf<ACXPickupItem> SpeedPickupClass;

    // 게임 시작 시 생성할 속도 아이템 개수
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawn|Speed")
    int32 SpeedPickupCount = 3;

    // 아이템 파괴 후 재생성까지 걸리는 시간
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawn|Respawn")
    float RespawnDelay = 2.0f;

    // 원형 트랙 위 랜덤 생성 위치 계산
    FVector GetRandomSpawnLocation() const;

    // 특정 종류의 아이템 1개 생성 및 파괴 이벤트 등록
    ACXPickupItem* SpawnItem(TSubclassOf<ACXPickupItem> ItemClass);

    // 게임 시작 시 기본 아이템 생성
    void SpawnInitialItems();

    // 생성된 아이템이 파괴되면 호출
    UFUNCTION()
    void OnSpawnedItemDestroyed(AActor* DestroyedActor);

    // 파괴된 아이템과 같은 종류를 다시 생성
    void RespawnItem(TSubclassOf<ACXPickupItem> ItemClass);
};
