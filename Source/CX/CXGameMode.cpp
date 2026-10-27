#include "CXGameMode.h"
#include "CXGameState.h"
#include "CXPlayerController.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"
#include "CXPlayerState.h"

ACXGameMode::ACXGameMode()
{
    // 기존 플레이어 컨트롤러 사용
    PlayerControllerClass = ACXPlayerController::StaticClass();

    // 게임 상태 정보를 공유할 GameState 지정
    GameStateClass = ACXGameState::StaticClass();
}

// 플레이어 접속 시 서버에서 실행
void ACXGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    // 첫 플레이어가 접속하면 대기 카운트다운 시작
    if (!bWaitingCountdownStarted)
    {
        StartWaitingCountdown();
    }
}

// 대기 카운트다운 시작
void ACXGameMode::StartWaitingCountdown()
{
    ACXGameState* CXGS = GetGameState<ACXGameState>();

    // GameState가 없으면 실행하지 않음
    if (!IsValid(CXGS))
    {
        return;
    }

    // 중복 실행 방지
    bWaitingCountdownStarted = true;

    // 게임 상태를 Waiting으로 설정
    CXGS->SetGamePhase(ECXGamePhase::Waiting);

    // 초기 대기 시간 설정
    CXGS->SetRemainingTime(WaitingDuration);

    UE_LOG(LogTemp, Warning,
        TEXT("[GAME] Waiting Started: %d seconds"),
        WaitingDuration);

    // 대기 시간이 없다면 즉시 게임 진행
    if (WaitingDuration <= 0)
    {
        StartPlayingCountdown();
        return;
    }

    // 서버에서 1초마다 남은 시간 감소
    GetWorldTimerManager().SetTimer(
        WaitingTimerHandle,
        this,
        &ACXGameMode::UpdateWaitingCountdown,
        1.0f,
        true
    );
}

// 대기 시간 1초 감소
void ACXGameMode::UpdateWaitingCountdown()
{
    ACXGameState* CXGS = GetGameState<ACXGameState>();

    if (!IsValid(CXGS))
    {
        return;
    }

    // 현재 남은 시간에서 1초 감소
    const int32 NewTime = CXGS->GetRemainingTime() - 1;

    CXGS->SetRemainingTime(NewTime);

    UE_LOG(LogTemp, Log,
        TEXT("[GAME] Waiting: %d"),
        CXGS->GetRemainingTime());

    // 대기 시간이 남아 있다면 계속 진행
    if (NewTime > 0)
    {
        return;
    }

    // 대기 타이머 종료
    GetWorldTimerManager().ClearTimer(WaitingTimerHandle);

    // 대기 종료 후 게임 진행 카운트다운 시작
    StartPlayingCountdown();
}


// 게임 진행 카운트다운 시작
void ACXGameMode::StartPlayingCountdown()
{
    ACXGameState* CXGS = GetGameState<ACXGameState>();

    if (!IsValid(CXGS))
    {
        return;
    }

    // 게임 상태를 Playing으로 변경
    CXGS->SetGamePhase(ECXGamePhase::Playing);

    // 게임 진행 시간 설정
    CXGS->SetRemainingTime(PlayingDuration);

    UE_LOG(LogTemp, Warning,
        TEXT("[GAME] Playing Started: %d seconds"),
        PlayingDuration);

    // 진행 시간이 없다면 즉시 우승자 판정
    if (PlayingDuration <= 0)
    {
        FinishGame();
        return;
    }

    // 서버에서 1초마다 진행 시간 감소
    GetWorldTimerManager().SetTimer(
        PlayingTimerHandle,
        this,
        &ACXGameMode::UpdatePlayingCountdown,
        1.0f,
        true
    );
}

// 게임 진행 시간 1초 감소
void ACXGameMode::UpdatePlayingCountdown()
{
    ACXGameState* CXGS = GetGameState<ACXGameState>();

    if (!IsValid(CXGS))
    {
        return;
    }

    // 남은 시간 1초 감소
    const int32 NewTime = CXGS->GetRemainingTime() - 1;
    CXGS->SetRemainingTime(NewTime);

    UE_LOG(LogTemp, Log,
        TEXT("[GAME] Playing: %d"),
        CXGS->GetRemainingTime());

    // 시간이 남았다면 계속 진행
    if (NewTime > 0)
    {
        return;
    }

    // 진행 타이머 종료
    GetWorldTimerManager().ClearTimer(PlayingTimerHandle);

    // 게임을 종료하고 우승자 판정
    FinishGame();
}


// 게임 종료 시 가장 많은 코인을 가진 플레이어 선정
void ACXGameMode::FinishGame()
{
    ACXGameState* CXGS = GetGameState<ACXGameState>();

    // 서버와 GameState 확인
    if (!HasAuthority() || !IsValid(CXGS))
    {
        return;
    }

    // 중복 종료 방지
    if (CXGS->GetGamePhase() == ECXGamePhase::Finished)
    {
        return;
    }

    // 가장 높은 코인 수와 우승자 목록
    int32 HighestCoins = -1;
    TArray<ACXPlayerState*> Winners;

    // GameState에 등록된 모든 플레이어 검사
    for (APlayerState* PlayerState : CXGS->PlayerArray)
    {
        ACXPlayerState* CXPS = Cast<ACXPlayerState>(PlayerState);

        if (!IsValid(CXPS))
        {
            continue;
        }

        const int32 PlayerCoins = CXPS->GetCoins();

        // 기존 최고 기록보다 높으면 우승자 목록 교체
        if (PlayerCoins > HighestCoins)
        {
            HighestCoins = PlayerCoins;
            Winners.Empty();
            Winners.Add(CXPS);
        }
        // 최고 기록과 같으면 공동 우승자로 추가
        else if (PlayerCoins == HighestCoins)
        {
            Winners.Add(CXPS);
        }
    }

    // GameState에 우승자 목록 저장
    CXGS->SetWinners(Winners);

    // 게임 종료 상태로 변경
    CXGS->SetGamePhase(ECXGamePhase::Finished);
    CXGS->SetRemainingTime(0);

    UE_LOG(LogTemp, Warning,
        TEXT("[GAME] Finished! Highest Coins: %d / Winners: %d"),
        HighestCoins,
        Winners.Num());
}

// 기존 차량 스폰 위치 선택 기능 유지
AActor* ACXGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    TArray<APlayerStart*> AvailablePlayerStarts;

    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        APlayerStart* PlayerStart = *It;

        if (IsPlayerStartAvailable(PlayerStart))
        {
            AvailablePlayerStarts.Add(PlayerStart);
        }
    }

    checkf(
        AvailablePlayerStarts.IsEmpty() == false,
        TEXT("There is no available Vehicle PlayerStart.")
    );

    const int32 RandomIndex =
        FMath::RandHelper(AvailablePlayerStarts.Num());

    return Cast<AActor>(AvailablePlayerStarts[RandomIndex]);
}

// 기존 차량 스폰 위치 충돌 검사 기능 유지
bool ACXGameMode::IsPlayerStartAvailable(const APlayerStart* PlayerStart) const
{
    if (IsValid(PlayerStart) == false || IsValid(GetWorld()) == false)
    {
        return false;
    }

    const FTransform PlayerStartTransform =
        PlayerStart->GetActorTransform();

    const FVector CheckLocation =
        PlayerStartTransform.TransformPosition(VehicleSpawnCheckOffset);

    const FQuat CheckRotation =
        PlayerStartTransform.GetRotation();

    const FCollisionShape CheckShape =
        FCollisionShape::MakeBox(VehicleSpawnCheckHalfExtent);

    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);

    const FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(VehiclePlayerStartCheck),
        false
    );

    const bool bVehicleExists =
        GetWorld()->OverlapAnyTestByObjectType(
            CheckLocation,
            CheckRotation,
            ObjectQueryParams,
            CheckShape,
            QueryParams
        );

    return !bVehicleExists;
}
