
#include "CXGameState.h"
#include "CXPlayerState.h"
#include "Net/UnrealNetwork.h"

ACXGameState::ACXGameState()
{
    // GameState 정보를 클라이언트에 복제
    bReplicates = true;
}

// 복제할 변수 등록
void ACXGameState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // 현재 단계, 남은 시간, 우승자 목록을 모든 클라이언트에 복제
    DOREPLIFETIME(ACXGameState, CurrentPhase);
    DOREPLIFETIME(ACXGameState, RemainingTime);
    DOREPLIFETIME(ACXGameState, Winners);
}

// 서버에서 게임 상태 설정
void ACXGameState::SetGamePhase(ECXGamePhase NewPhase)
{
    if (!HasAuthority())
    {
        return;
    }

    CurrentPhase = NewPhase;
}

// 서버에서 남은 시간 설정
void ACXGameState::SetRemainingTime(int32 NewTime)
{
    if (!HasAuthority())
    {
        return;
    }

    // 남은 시간이 음수가 되지 않도록 제한
    RemainingTime = FMath::Max(0, NewTime);
}

// 서버에서 우승자 목록 설정
void ACXGameState::SetWinners(
    const TArray<ACXPlayerState*>& NewWinners
)
{
    if (!HasAuthority())
    {
        return;
    }

    // 기존 우승자 목록 초기화
    Winners.Empty();

    // 서버에서 계산한 우승자 등록
    for (ACXPlayerState* Winner : NewWinners)
    {
        if (IsValid(Winner))
        {
            Winners.Add(Winner);
        }
    }
}
