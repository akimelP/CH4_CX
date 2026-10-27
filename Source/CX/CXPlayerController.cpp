#include "CXPlayerController.h"
#include "CXPawn.h"
#include "CXUI.h"
#include "EnhancedInputSubsystems.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Blueprint/UserWidget.h"
#include "CX.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Net/UnrealNetwork.h"
#include "CXChatWidget.h"
#include "CXPlayerState.h"

void ACXPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() == false)
	{
		return;
	}
	
	// only spawn UI on local player controllers
	if (IsLocalPlayerController())
	{
		if (SVirtualJoystick::ShouldDisplayTouchInterface())
		{
			// spawn the mobile controls widget
			MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

			if (MobileControlsWidget)
			{
				// add the controls to the player screen
				MobileControlsWidget->AddToPlayerScreen(0);

			} else {

				UE_LOG(LogCX, Error, TEXT("Could not spawn mobile controls widget."));

			}
		}
		
		// spawn the UI widget and add it to the viewport
		VehicleUI = CreateWidget<UCXUI>(this, VehicleUIClass);

		if (VehicleUI)
		{
			VehicleUI->AddToViewport();

		} else {

			UE_LOG(LogCX, Error, TEXT("Could not spawn vehicle UI widget."));

		}

		// 로컬 플레이어의 채팅창 생성
		if (ChatWidgetClass)
		{
			ChatWidget = CreateWidget<UCXChatWidget>(this, ChatWidgetClass);

			if (ChatWidget)
			{
				// 게임 화면에 채팅창 표시
				ChatWidget->AddToViewport();
			}
		}

		// 로컬 플레이어의 게임 상태 UI 생성
		if (GameStatusWidgetClass)
		{
			GameStatusWidget = CreateWidget<UUserWidget>(
				this,
				GameStatusWidgetClass
			);

			if (IsValid(GameStatusWidget))
			{
				// 기존 차량 및 채팅 UI 위에 게임 상태 표시
				GameStatusWidget->AddToViewport(1);
			}
		}
	}
}

void ACXPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

void ACXPlayerController::Tick(float Delta)
{
	Super::Tick(Delta);

	if (IsValid(VehiclePawn) && IsValid(VehicleUI))
	{
		VehicleUI->UpdateSpeed(VehiclePawn->GetChaosVehicleMovement()->GetForwardSpeed());
		VehicleUI->UpdateGear(VehiclePawn->GetChaosVehicleMovement()->GetCurrentGear());
	}
}

void ACXPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 현재 조종하는 차량 가져오기
	VehiclePawn = CastChecked<ACXPawn>(InPawn);

	// 차량 파괴 이벤트 연결
	VehiclePawn->OnDestroyed.AddDynamic(
		this,
		&ACXPlayerController::OnPawnDestroyed
	);

	// 서버에서만 가속력 보너스 적용
	if (!HasAuthority())
	{
		return;
	}

	// 현재 플레이어의 PlayerState 가져오기
	ACXPlayerState* CXPS = GetPlayerState<ACXPlayerState>();

	if (!IsValid(CXPS))
	{
		return;
	}

	// 저장된 보너스를 새 차량에 적용
	VehiclePawn->ApplyAccelerationBonus(
		CXPS->GetBonusMaxSpeed()
	);
}

void ACXPlayerController::OnPawnDestroyed(AActor* DestroyedPawn)
{
	// find the player start
	TArray<AActor*> ActorList;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), ActorList);

	if (ActorList.Num() > 0)
	{
		// spawn a vehicle at the player start
		const FTransform SpawnTransform = ActorList[0]->GetActorTransform();

		if (ACXPawn* RespawnedVehicle = GetWorld()->SpawnActor<ACXPawn>(VehiclePawnClass, SpawnTransform))
		{
			// possess the vehicle
			Possess(RespawnedVehicle);
		}
	}
}

void ACXPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, VehiclePawn);
}

// 서버에서 채팅 메시지를 수신
void ACXPlayerController::ServerSendChatMessage_Implementation(
	const FString& Message
)
{
	// 메시지 앞뒤 공백 제거
	const FString CleanMessage = Message.TrimStartAndEnd();

	// 빈 메시지 또는 200자 초과 메시지 차단
	if (CleanMessage.IsEmpty() || CleanMessage.Len() > 200)
	{
		return;
	}

	// 서버에서 플레이어 번호 확인
	int32 PlayerNumber = 1;

	// 서버에 접속한 PlayerController 순회
	for (FConstPlayerControllerIterator It =
		GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ACXPlayerController* PC = Cast<ACXPlayerController>(It->Get());

		if (!IsValid(PC))
		{
			continue;
		}

		// 현재 메시지를 보낸 플레이어를 찾으면 반복 종료
		if (PC == this)
		{
			break;
		}

		// 다른 플레이어를 지나갈 때마다 번호 증가
		++PlayerNumber;
	}

	// 메시지 앞에 플레이어 번호 추가
	const FString FormattedMessage = FString::Printf(
		TEXT("[Player %d] %s"),
		PlayerNumber,
		*CleanMessage
	);

	// 모든 플레이어에게 메시지 전달
	for (FConstPlayerControllerIterator It =
		GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ACXPlayerController* PC = Cast<ACXPlayerController>(It->Get());

		if (IsValid(PC))
		{
			PC->ClientReceiveChatMessage(FormattedMessage);
		}
	}
}

// 클라이언트가 서버에서 전달한 채팅 메시지를 수신
void ACXPlayerController::ClientReceiveChatMessage_Implementation(
	const FString& Message
)
{
	// 메시지 수신 여부 로그 출력
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[CLIENT CHAT] %s"),
		*Message
	);

	// 채팅 위젯이 존재하면 메시지를 화면에 추가
	if (IsValid(ChatWidget))
	{
		ChatWidget->AddChatMessage(Message);
	}
}

// 채팅 입력 모드 활성화
void ACXPlayerController::OpenChat()
{
	// 로컬 플레이어의 채팅 위젯 확인
	if (!IsLocalController() || !IsValid(ChatWidget))
	{
		return;
	}

	// 채팅 입력창으로 입력을 집중
	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);

	// 채팅창에 키보드 포커스 설정
	ChatWidget->FocusChatInput();

	// 마우스 커서 표시
	bShowMouseCursor = true;
}

// 채팅 종료 후 게임 입력으로 복귀
void ACXPlayerController::CloseChat()
{
	if (!IsLocalController())
	{
		return;
	}

	// 차량 조작 입력으로 복귀
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	// 마우스 커서 숨기기
	bShowMouseCursor = false;
}

// 서버에서 자신의 PlayerState에 코인 100원 지급
void ACXPlayerController::ServerAddTestCoins_Implementation()
{
	// 서버 권한 확인
	if (!HasAuthority())
	{
		return;
	}

	// 자신의 PlayerState 가져오기
	ACXPlayerState* CXPS = GetPlayerState<ACXPlayerState>();

	if (!IsValid(CXPS))
	{
		return;
	}

	// 코인 100원 증가
	CXPS->AddCoins(100);

	UE_LOG(LogTemp, Warning, TEXT("Test Coins: %d"), CXPS->GetCoins());
}

// 서버에서 자신의 추가 최대 속도를 10 증가
void ACXPlayerController::ServerAddTestBonusSpeed_Implementation()
{
	// 서버에서만 처리
	if (!HasAuthority())
	{
		return;
	}

	// 자신의 PlayerState 가져오기
	ACXPlayerState* CXPS = GetPlayerState<ACXPlayerState>();

	if (!IsValid(CXPS))
	{
		return;
	}

	// 추가 최대 속도 10 증가
	CXPS->AddBonusMaxSpeed(10.0f);

	UE_LOG(LogTemp, Warning, TEXT("Test BonusSpeed: %.1f"),
		CXPS->GetBonusMaxSpeed());
}

// 서버에서 자신의 코인을 5원 차감하는 테스트
void ACXPlayerController::ServerSpendTestCoins_Implementation()
{
	// 서버 권한 확인
	if (!HasAuthority())
	{
		return;
	}

	// 자신의 PlayerState 가져오기
	ACXPlayerState* CXPS = GetPlayerState<ACXPlayerState>();

	if (!IsValid(CXPS))
	{
		return;
	}

	// 코인 5원 차감 시도
	const bool bSuccess = CXPS->SpendCoins(5);

	// 차감 성공 여부와 남은 코인 출력
	UE_LOG(LogTemp, Warning, TEXT("Spend Coins: %s / Remaining: %d"),
		bSuccess ? TEXT("Success") : TEXT("Failed"),
		CXPS->GetCoins());
}