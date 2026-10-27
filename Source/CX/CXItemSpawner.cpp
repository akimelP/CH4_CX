
#include "CXItemSpawner.h"
#include "CXPickupItem.h"
#include "TimerManager.h"

ACXItemSpawner::ACXItemSpawner()
{
    // 매 프레임 실행할 필요가 없으므로 Tick 비활성화
    PrimaryActorTick.bCanEverTick = false;
}

void ACXItemSpawner::BeginPlay()
{
    Super::BeginPlay();

    // 아이템 생성은 서버에서만 처리
    if (!HasAuthority())
    {
        return;
    }

    // 게임 시작 시 기본 코인과 속도 아이템 생성
    SpawnInitialItems();
}

// 원형 트랙 위 랜덤 생성 위치 계산
FVector ACXItemSpawner::GetRandomSpawnLocation() const
{
    // 스포너 위치를 원형 트랙의 중심으로 사용
    const FVector Center = GetActorLocation();

    // 랜덤 각도와 반지름 선택
    const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
    const float Radius = FMath::FRandRange(InnerRadius, OuterRadius);

    // 원형 트랙 위 좌표 계산
    const float X = Center.X + FMath::Cos(Angle) * Radius;
    const float Y = Center.Y + FMath::Sin(Angle) * Radius;
    const float Z = Center.Z + SpawnHeight;

    return FVector(X, Y, Z);
}

// 아이템 1개 생성 및 파괴 이벤트 등록
ACXPickupItem* ACXItemSpawner::SpawnItem(
    TSubclassOf<ACXPickupItem> ItemClass
)
{
    // 서버에서만 아이템 생성
    if (!HasAuthority() || !ItemClass)
    {
        return nullptr;
    }

    // 원형 트랙에서 랜덤 위치 선택
    const FVector SpawnLocation = GetRandomSpawnLocation();

    // 충돌 여부와 관계없이 아이템 생성
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ACXPickupItem* SpawnedItem = GetWorld()->SpawnActor<ACXPickupItem>(
        ItemClass,
        SpawnLocation,
        FRotator::ZeroRotator,
        SpawnParams
    );

    // 아이템 생성 실패 시 종료
    if (!IsValid(SpawnedItem))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[ITEM SPAWNER] Item Spawn Failed!"));

        return nullptr;
    }

    // 아이템이 파괴되면 스포너의 함수가 호출되도록 등록
    SpawnedItem->OnDestroyed.AddDynamic(
        this,
        &ACXItemSpawner::OnSpawnedItemDestroyed
    );

    return SpawnedItem;
}

// 게임 시작 시 기본 아이템 생성
void ACXItemSpawner::SpawnInitialItems()
{
    // 시작 코인 클래스 확인
    if (!StartCoinClass)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[ITEM SPAWNER] StartCoinClass is not set!"));
    }
    else
    {
        // 설정한 개수만큼 시작 코인 생성
        for (int32 i = 0; i < StartCoinCount; ++i)
        {
            SpawnItem(StartCoinClass);
        }

        UE_LOG(LogTemp, Log,
            TEXT("[ITEM SPAWNER] Initial Coins: %d"),
            StartCoinCount);
    }

    // 속도 아이템 클래스 확인
    if (!SpeedPickupClass)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[ITEM SPAWNER] SpeedPickupClass is not set!"));
    }
    else
    {
        // 설정한 개수만큼 속도 아이템 생성
        for (int32 i = 0; i < SpeedPickupCount; ++i)
        {
            SpawnItem(SpeedPickupClass);
        }

        UE_LOG(LogTemp, Log,
            TEXT("[ITEM SPAWNER] Initial Speed Pickups: %d"),
            SpeedPickupCount);
    }
}

// 아이템이 파괴되면 서버에서 호출
void ACXItemSpawner::OnSpawnedItemDestroyed(AActor* DestroyedActor)
{
    // 서버에서만 재생성 처리
    if (!HasAuthority() || !DestroyedActor)
    {
        return;
    }

    // 파괴된 아이템의 실제 블루프린트 클래스 보관
    TSubclassOf<ACXPickupItem> ItemClass =
        DestroyedActor->GetClass();

    // 재생성할 클래스 확인
    if (!ItemClass)
    {
        return;
    }

    UE_LOG(LogTemp, Log,
        TEXT("[ITEM SPAWNER] Item Destroyed: %s"),
        *GetNameSafe(ItemClass.Get()));

    // 2초 후 같은 종류의 아이템을 재생성하도록 예약
    FTimerDelegate RespawnDelegate = FTimerDelegate::CreateUObject(
        this,
        &ACXItemSpawner::RespawnItem,
        ItemClass
    );

    FTimerHandle RespawnTimerHandle;

    GetWorldTimerManager().SetTimer(
        RespawnTimerHandle,
        RespawnDelegate,
        RespawnDelay,
        false
    );
}

// 파괴된 아이템과 같은 종류의 아이템 재생성
void ACXItemSpawner::RespawnItem(
    TSubclassOf<ACXPickupItem> ItemClass
)
{
    // 서버에서만 재생성
    if (!HasAuthority() || !ItemClass)
    {
        return;
    }

    // 새로운 랜덤 위치에 동일한 종류의 아이템 생성
    ACXPickupItem* NewItem = SpawnItem(ItemClass);

    if (IsValid(NewItem))
    {
        UE_LOG(LogTemp, Log,
            TEXT("[ITEM SPAWNER] Item Respawned: %s"),
            *NewItem->GetName());
    }
}
