#include "CXPlayerState.h"
#include "Net/UnrealNetwork.h"

// PlayerState 생성자
ACXPlayerState::ACXPlayerState()
{
    // PlayerState의 네트워크 복제 활성화
    bReplicates = true;
}

// 복제할 변수와 복제 조건 등록
void ACXPlayerState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Coins는 해당 플레이어에게만 복제
    DOREPLIFETIME_CONDITION(
        ACXPlayerState,
        Coins,
        COND_OwnerOnly
    );

    // BonusMaxSpeed도 해당 플레이어에게만 복제
    DOREPLIFETIME_CONDITION(
        ACXPlayerState,
        BonusMaxSpeed,
        COND_OwnerOnly
    );
}

// 서버에서만 코인을 증가시킴
void ACXPlayerState::AddCoins(int32 Amount)
{
    // 서버 권한이 없거나 잘못된 수량이면 처리하지 않음
    if (!HasAuthority() || Amount <= 0)
    {
        return;
    }

    // 코인 증가
    Coins += Amount;

    // 코인이 변경됐으므로 UI에 알림
    OnPlayerStatusChanged.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("Coins Added: %d / Total: %d"), Amount, Coins);
}

// 서버에서만 코인을 차감함
bool ACXPlayerState::SpendCoins(int32 Amount)
{
    // 서버 권한이 없거나 잘못된 수량이면 실패
    if (!HasAuthority() || Amount <= 0)
    {
        return false;
    }

    // 보유 코인이 부족하면 차감하지 않음
    if (Coins < Amount)
    {
        return false;
    }

    // 코인 차감
    Coins -= Amount;

    // 코인이 차감됐으므로 UI에 알림
    OnPlayerStatusChanged.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("Coins Spent: %d / Remaining: %d"), Amount, Coins);

    return true;
}

// 현재 보유한 코인 반환
int32 ACXPlayerState::GetCoins() const
{
    return Coins;
}

// 서버에서만 추가 최대 속도를 증가시킴
void ACXPlayerState::AddBonusMaxSpeed(float Amount)
{
    // 서버 권한이 없거나 증가량이 올바르지 않으면 종료
    if (!HasAuthority() || !FMath::IsFinite(Amount) || Amount <= 0.0f)
    {
        return;
    }

    // 추가 최대 속도 증가
    BonusMaxSpeed += Amount;

    // 가속력 보너스가 변경됐으므로 UI에 알림
    OnPlayerStatusChanged.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("BonusMaxSpeed Added: %.1f / Total: %.1f"),
        Amount, BonusMaxSpeed);
}

// 현재 추가 최대 속도 반환
float ACXPlayerState::GetBonusMaxSpeed() const
{
    return BonusMaxSpeed;
}

// 클라이언트에서 코인 변경을 수신하면 UI에 알림
void ACXPlayerState::OnRep_Coins()
{
    OnPlayerStatusChanged.Broadcast();
}

// 클라이언트에서 가속력 보너스 변경을 수신하면 UI에 알림
void ACXPlayerState::OnRep_BonusMaxSpeed()
{
    OnPlayerStatusChanged.Broadcast();
}