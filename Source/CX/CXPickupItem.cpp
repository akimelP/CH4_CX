#include "CXPickupItem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CXPawn.h"
#include "CXPlayerState.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"

ACXPickupItem::ACXPickupItem()
{
    // 매 프레임 메시를 회전시키기 위해 Tick 활성화
    PrimaryActorTick.bCanEverTick = true;

    // 서버에서 아이템 생성 및 제거 상태를 복제
    bReplicates = true;

    // 아이템의 네트워크 업데이트 빈도를 초당 1회로 설정
    NetUpdateFrequency = 1.0f;

    // 차량 접근을 감지할 구체 충돌 영역 생성
    PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
    RootComponent = PickupSphere;

    // 충돌 반경 설정
    PickupSphere->InitSphereRadius(150.0f);

    // 물리적으로 막지 않고 겹침만 감지
    PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    // 다른 충돌은 무시하고 차량과의 겹침만 감지
    PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    PickupSphere->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
    PickupSphere->SetGenerateOverlapEvents(true);

    // 아이템 외형을 표시할 메시 생성
    PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
    PickupMesh->SetupAttachment(PickupSphere);

    // 메시 자체는 충돌하지 않도록 설정
    PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 충돌 영역에 다른 액터가 들어오면 OnPickupOverlap 호출
    PickupSphere->OnComponentBeginOverlap.AddDynamic(
        this,
        &ACXPickupItem::OnPickupOverlap
    );
}

void ACXPickupItem::BeginPlay()
{
    Super::BeginPlay();
}

// 매 프레임 아이템 메시를 회전
void ACXPickupItem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 외형 메시만 회전하고 충돌 영역은 유지
    if (IsValid(PickupMesh))
    {
        const FRotator DeltaRotation(
            0.0f,
            RotationSpeed * DeltaTime,
            0.0f
        );

        PickupMesh->AddLocalRotation(DeltaRotation);
    }
}

// 차량이 코인에 닿으면 서버에서 코인을 지급
void ACXPickupItem::OnPickupOverlap(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult
)
{
    // 서버에서만 아이템 획득 처리
    if (!HasAuthority())
    {
        return;
    }

    // 이미 획득한 아이템이면 중복 처리하지 않음
    if (bIsCollected)
    {
        return;
    }

    // 충돌한 액터가 차량인지 확인
    ACXPawn* Vehicle = Cast<ACXPawn>(OtherActor);

    if (!IsValid(Vehicle))
    {
        return;
    }

    // Playing 상태가 아니면 아이템 획득 차단
    if (!Vehicle->IsGamePlaying())
    {
        return;
    }

    // 차량을 조종하는 PlayerController 가져오기
    AController* Controller = Vehicle->GetController();

    if (!IsValid(Controller))
    {
        return;
    }

    // 해당 플레이어의 PlayerState 가져오기
    ACXPlayerState* CXPS = Controller->GetPlayerState<ACXPlayerState>();

    if (!IsValid(CXPS))
    {
        return;
    }

    // 아이템 종류에 따라 다른 보상 지급
    switch (PickupType)
    {
    case ECXPickupType::Coin:
        // 코인 지급
        CXPS->AddCoins(CoinValue);
        break;

    case ECXPickupType::Speed:
        // 서버에서 플레이어의 가속력 보너스 증가
        CXPS->AddBonusMaxSpeed(SpeedValue);

        // 누적된 보너스를 실제 차량 엔진에 적용
        Vehicle->ApplyAccelerationBonus(CXPS->GetBonusMaxSpeed());

        break;
    }

    // 중복 획득 방지
    bIsCollected = true;

    // 서버에서 즉시 충돌 비활성화
    PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 모든 플레이어에게 획득 이펙트 전송
    MulticastPlayPickupEffect(GetActorLocation());

    // 서버와 클라이언트에서 아이템 외형 숨기기
    SetActorHiddenInGame(true);

    // 0.3초 후 실제 아이템 액터 제거
    GetWorldTimerManager().SetTimer(
        DestroyTimerHandle,
        this,
        &ACXPickupItem::FinishPickup,
        0.3f,
        false
    );
}

// 모든 플레이어에게 획득 파티클과 사운드 재생
void ACXPickupItem::MulticastPlayPickupEffect_Implementation(FVector PickupLocation)
{
    // 모든 플레이어 화면에서 아이템 외형 숨기기
    SetActorHiddenInGame(true);

    // 클라이언트에서도 아이템 충돌 비활성화
    PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 파티클이 지정되어 있으면 획득 위치에 재생
    if (PickupEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(),
            PickupEffect,
            PickupLocation
        );
    }

    // 사운드가 지정되어 있으면 획득 위치에 재생
    if (PickupSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            PickupSound,
            PickupLocation
        );
    }
}

// 이펙트 전송 후 아이템 액터 제거
void ACXPickupItem::FinishPickup()
{
    if (HasAuthority())
    {
        Destroy();
    }
}