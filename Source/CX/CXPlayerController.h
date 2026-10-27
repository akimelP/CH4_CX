#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CXPlayerController.generated.h"

class UInputMappingContext;
class ACXPawn;
class UCXUI;
class UCXChatWidget;
class UUserWidget;

/**
 *  Vehicle Player Controller class
 *  Handles input mapping and user interface
 */
UCLASS(abstract)
class ACXPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the optional steering wheel input mapping context will be registered */
	UPROPERTY(EditAnywhere, Category = "Input|Steering Wheel Controls")
	bool bUseSteeringWheelControls = false;

	/** Optional Input Mapping Context to be used for steering wheel input.
	 *  This is added alongside the default Input Mapping Context and does not block other forms of input.
	 */
	UPROPERTY(EditAnywhere, Category = "Input|Steering Wheel Controls", meta = (EditCondition = "bUseSteeringWheelControls"))
	UInputMappingContext* SteeringWheelInputMappingContext;

	/** Type of vehicle to automatically respawn when it's destroyed */
	UPROPERTY(EditAnywhere, Category="Vehicle|Respawn")
	TSubclassOf<ACXPawn> VehiclePawnClass;

	UPROPERTY(Replicated)
	TObjectPtr<ACXPawn> VehiclePawn;

	/** Type of the UI to spawn */
	UPROPERTY(EditAnywhere, Category="Vehicle|UI")
	TSubclassOf<UCXUI> VehicleUIClass;

	/** Pointer to the UI widget */
	TObjectPtr<UCXUI> VehicleUI;

	// 생성할 채팅 위젯 블루프린트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Chat")
	TSubclassOf<UCXChatWidget> ChatWidgetClass;

	// 실제 생성된 채팅 위젯 객체
	UPROPERTY()
	TObjectPtr<UCXChatWidget> ChatWidget;

	// 게임 상태 위젯 블루프린트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Game Status")
	TSubclassOf<UUserWidget> GameStatusWidgetClass;

	// 실제 생성된 게임 상태 위젯
	UPROPERTY()
	TObjectPtr<UUserWidget> GameStatusWidget;
		
protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input setup */
	virtual void SetupInputComponent() override;

public:

	/** Update vehicle UI on tick */
	virtual void Tick(float Delta) override;

protected:

	/** Pawn setup */
	virtual void OnPossess(APawn* InPawn) override;

	/** Handles pawn destruction and respawning */
	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedPawn);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

public:
	// 클라이언트가 입력한 채팅 메시지를 서버로 전달
	UFUNCTION(Server, Reliable)
	void ServerSendChatMessage(const FString& Message);

	// 서버가 해당 플레이어에게 채팅 메시지를 전달
	UFUNCTION(Client, Reliable)
	void ClientReceiveChatMessage(const FString& Message);

	// Enter 키로 채팅 입력 모드 활성화
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void OpenChat();

	// 채팅 종료 후 차량 조작 모드로 복귀
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void CloseChat();

	// 서버가 플레이어에게 부여한 채팅 표시용 번호
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chat")
	int32 ChatPlayerNumber = 0;

	// 테스트용: 서버에서 자신의 코인을 100원 증가
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Test")
	void ServerAddTestCoins();

	// 테스트용: 서버에서 추가 최대 속도를 10 증가
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Test")
	void ServerAddTestBonusSpeed();

	// 테스트용: 서버에서 자신의 코인 5원 차감
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Test")
	void ServerSpendTestCoins();
};
