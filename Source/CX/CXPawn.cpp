#include "CXPawn.h"
#include "CXWheelFront.h"
#include "CXWheelRear.h"
#include "CXPlayerState.h"
#include "CXGameState.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "CX.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"


#define LOCTEXT_NAMESPACE "VehiclePawn"

ACXPawn::ACXPawn()
{
	// construct the front camera boom
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// construct the back camera boom
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	// Configure the car mesh
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	// get the Chaos Wheeled movement component
	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

}

void ACXPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// steering 
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ACXPawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ACXPawn::Steering);

		// throttle 
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ACXPawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ACXPawn::Throttle);

		// 좌클릭을 시작하면 공격 실행 (한 번 클릭에 한 번)
		EnhancedInputComponent->BindAction(
			AttackAction,
			ETriggerEvent::Started,
			this,
			&ACXPawn::Attack
		);

		// break 
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ACXPawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopBrake);

		// handbrake 
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopHandbrake);

		// look around 
		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &ACXPawn::LookAround);

		// toggle camera 
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACXPawn::ToggleCamera);

		// reset the vehicle 
		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &ACXPawn::ResetVehicle);
	}
	else
	{
		UE_LOG(LogCX, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ACXPawn::Tick(float Delta)
{
	Super::Tick(Delta);

	// 게임 상태에 따라 차량 조작과 핸드브레이크 제어
	if (!IsGamePlaying())
	{
		// Waiting / Finished에서는 가속과 조향 차단
		ChaosVehicleMovement->SetThrottleInput(0.0f);
		ChaosVehicleMovement->SetSteeringInput(0.0f);

		// 게임 진행 중이 아니라면 핸드브레이크 강제 잠금
		ChaosVehicleMovement->SetHandbrakeInput(true);
	}
	else
	{
		// Playing에서는 플레이어가 누르고 있을 때만 핸드브레이크 적용
		ChaosVehicleMovement->SetHandbrakeInput(bHandbrakePressed);
	}

	// 공중에 떠 있을 때 차량 회전 감쇠
	const bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	// 카메라를 차량 정면 방향으로 부드럽게 복귀
	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);

	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));
}

void ACXPawn::Steering(const FInputActionValue& Value)
{
	// route the input
	DoSteering(Value.Get<float>());
}

void ACXPawn::Throttle(const FInputActionValue& Value)
{
	// route the input
	DoThrottle(Value.Get<float>());
}

void ACXPawn::Brake(const FInputActionValue& Value)
{
	// route the input
	DoBrake(Value.Get<float>());
}

void ACXPawn::StartBrake(const FInputActionValue& Value)
{
	// route the input
	DoBrakeStart();
}

void ACXPawn::StopBrake(const FInputActionValue& Value)
{
	// route the input
	DoBrakeStop();
}

void ACXPawn::StartHandbrake(const FInputActionValue& Value)
{
	// route the input
	DoHandbrakeStart();
}

void ACXPawn::StopHandbrake(const FInputActionValue& Value)
{
	// route the input
	DoHandbrakeStop();
}

void ACXPawn::LookAround(const FInputActionValue& Value)
{
	// route the input
	DoLookAround(Value.Get<float>());
}

void ACXPawn::ToggleCamera(const FInputActionValue& Value)
{
	// route the input
	DoToggleCamera();
}

void ACXPawn::ResetVehicle(const FInputActionValue& Value)
{
	// route the input
	DoResetVehicle();
}

// Playing 상태에서만 차량 조향 허용
void ACXPawn::DoSteering(float SteeringValue)
{
	// 게임 진행 중이 아니면 조향 입력을 0으로 유지
	if (!IsGamePlaying())
	{
		ChaosVehicleMovement->SetSteeringInput(0.0f);
		return;
	}

	// 게임 진행 중에는 정상적으로 조향
	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

// Playing 상태에서만 차량 가속 허용
void ACXPawn::DoThrottle(float ThrottleValue)
{
	// 게임 진행 중이 아니면 가속 입력을 0으로 유지
	if (!IsGamePlaying())
	{
		ChaosVehicleMovement->SetThrottleInput(0.0f);
		return;
	}

	// 게임 진행 중에는 정상적으로 가속
	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);

	// 기존 브레이크 입력 초기화 기능 유지
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void ACXPawn::DoBrake(float BrakeValue)
{
	// add the input
	ChaosVehicleMovement->SetBrakeInput(BrakeValue);

	// reset the throttle input
	ChaosVehicleMovement->SetThrottleInput(0.0f);
}

void ACXPawn::DoBrakeStart()
{
	// call the Blueprint hook for the brake lights
	BrakeLights(true);
}

void ACXPawn::DoBrakeStop()
{
	// call the Blueprint hook for the brake lights
	BrakeLights(false);

	// reset brake input to zero
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

// 플레이어가 핸드브레이크를 누른 상태 저장
void ACXPawn::DoHandbrakeStart()
{
	bHandbrakePressed = true;

	// Playing 중에만 플레이어의 핸드브레이크 입력 적용
	if (IsGamePlaying())
	{
		ChaosVehicleMovement->SetHandbrakeInput(true);
		BrakeLights(true);
	}
}

// 플레이어가 핸드브레이크를 놓은 상태 저장
void ACXPawn::DoHandbrakeStop()
{
	bHandbrakePressed = false;

	// Waiting / Finished에서는 게임 규칙에 의한 잠금 유지
	if (!IsGamePlaying())
	{
		ChaosVehicleMovement->SetHandbrakeInput(true);
		return;
	}

	// Playing에서는 핸드브레이크 해제
	ChaosVehicleMovement->SetHandbrakeInput(false);
	BrakeLights(false);
}

void ACXPawn::DoLookAround(float YawDelta)
{
	// rotate the spring arm
	BackSpringArm->AddLocalRotation(FRotator(0.0f, YawDelta, 0.0f));
}

void ACXPawn::DoToggleCamera()
{
	// toggle the active camera flag
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void ACXPawn::DoResetVehicle()
{
	// Playing 상태가 아니면 차량 위치 초기화 차단
	if (!IsGamePlaying())
	{
		return;
	}

	// reset to a location slightly above our current one
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	// reset to our yaw. Ignore pitch and roll
	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;

	// teleport the actor to the reset spot and reset physics
	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
}

// PlayerState의 보너스에 따라 차량 엔진 토크 증가
void ACXPawn::ApplyAccelerationBonus(float BonusPercent)
{
	// 차량 이동 컴포넌트 확인
	if (!IsValid(ChaosVehicleMovement))
	{
		return;
	}

	// 잘못된 보너스 값 방지
	if (!FMath::IsFinite(BonusPercent) || BonusPercent < 0.0f)
	{
		return;
	}

	// 기본 엔진 토크 750에 보너스 비율 적용
	const float BaseTorque = 750.0f;
	// 보너스 3당 엔진 토크 1% 증가
	const float NewTorque = BaseTorque * (1.0f + BonusPercent / 300.0f);

	// 차량 엔진의 최대 토크 변경
	ChaosVehicleMovement->SetMaxEngineTorque(NewTorque);

	// 적용 결과 확인
	UE_LOG(LogTemp, Log,
		TEXT("Acceleration Bonus: %.1f%% / Engine Torque: %.1f"),
		BonusPercent, NewTorque);
}

// 현재 게임이 Playing 상태인지 확인
bool ACXPawn::IsGamePlaying() const
{
	// 서버와 클라이언트에 공유되는 GameState 가져오기
	const ACXGameState* CXGS = GetWorld()
		? GetWorld()->GetGameState<ACXGameState>()
		: nullptr;

	// GameState가 없거나 Playing이 아니면 false
	if (!IsValid(CXGS))
	{
		return false;
	}

	return CXGS->GetGamePhase() == ECXGamePhase::Playing;
}

// Playing 상태에서만 좌클릭 공격 허용
void ACXPawn::Attack(const FInputActionValue& Value)
{
	// 게임 진행 중이 아니면 공격 요청하지 않음
	if (!IsGamePlaying())
	{
		return;
	}

	// 게임 진행 중에만 서버에 공격 요청
	ServerAttack();
}

// 서버에서 공격 판정 및 데미지 처리
void ACXPawn::ServerAttack_Implementation()
{
	// 서버에서 Playing 상태일 때만 공격 처리
	if (!HasAuthority() || !IsGamePlaying())
	{
		return;
	}

	// 차량의 전방 방향
	const FVector Forward = GetActorForwardVector();

	// 앞 범퍼 중앙에서 공격 시작
	const FVector Start = GetActorLocation()
		+ Forward * 250.0f
		+ FVector(0.0f, 0.0f, 50.0f);

	// 전방 500cm 공격
	const FVector End = Start + Forward * 500.0f;

	// 자기 차량은 충돌 대상에서 제외
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	// Vehicle 충돌 채널만 검사
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Vehicle);

	FHitResult HitResult;

	// 서버에서 Line Trace 실행
	const bool bHit = GetWorld()->LineTraceSingleByObjectType(
		HitResult,
		Start,
		End,
		ObjectParams,
		Params
	);

	// 공격 범위를 빨간색 선으로 표시
	DrawDebugLine(
		GetWorld(),
		Start,
		bHit ? HitResult.ImpactPoint : End,
		FColor::Red,
		false,
		2.0f,
		0,
		4.0f
	);

	if (!bHit)
	{
		return;
	}

	// 맞은 액터가 상대 차량인지 확인
	ACXPawn* HitVehicle = Cast<ACXPawn>(HitResult.GetActor());

	if (!IsValid(HitVehicle) || HitVehicle == this)
	{
		return;
	}

	// 상대 차량에게 데미지 전달
	UGameplayStatics::ApplyDamage(
		HitVehicle,
		1.0f,
		GetController(),
		this,
		UDamageType::StaticClass()
	);

	// 상대 차량에 명중하면 모든 플레이어에게 피격 파티클 표시
	HitVehicle->MulticastPlayHitEffect(HitResult.ImpactPoint);

	UE_LOG(LogTemp, Warning,
		TEXT("[ATTACK] Hit Vehicle: %s"),
		*HitVehicle->GetName());
}

// 상대 차량에게 공격받으면 코인 5원 강탈 처리
float ACXPawn::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	// 부모 클래스의 데미지 처리
	const float ActualDamage = Super::TakeDamage(
		DamageAmount,
		DamageEvent,
		EventInstigator,
		DamageCauser
	);

	// 서버에서만 코인 변경
	if (!HasAuthority())
	{
		return ActualDamage;
	}

	// 공격 차량 확인
	ACXPawn* Attacker = Cast<ACXPawn>(DamageCauser);

	if (!IsValid(Attacker) || Attacker == this ||
		!IsValid(EventInstigator) ||
		Attacker->GetController() != EventInstigator)
	{
		return ActualDamage;
	}

	// 피해 차량의 PlayerState 가져오기
	AController* VictimController = GetController();

	if (!IsValid(VictimController))
	{
		return ActualDamage;
	}

	ACXPlayerState* VictimPS =
		VictimController->GetPlayerState<ACXPlayerState>();

	// 공격 차량의 PlayerState 가져오기
	ACXPlayerState* AttackerPS =
		EventInstigator->GetPlayerState<ACXPlayerState>();

	// 양쪽 PlayerState 확인
	if (!IsValid(VictimPS) || !IsValid(AttackerPS))
	{
		return ActualDamage;
	}

	// 상대방의 코인이 5원 미만이면 아무것도 하지 않음
	if (VictimPS->GetCoins() < 5)
	{
		return ActualDamage;
	}

	// 피해자의 코인 5원 차감
	if (VictimPS->SpendCoins(5))
	{
		// 차감 성공 시 공격자에게 5원 지급
		AttackerPS->AddCoins(5);

		UE_LOG(LogTemp, Warning,
			TEXT("[COIN STEAL] Attacker +5 / Victim -5"));
	}

	return ActualDamage;
}

// 모든 플레이어의 화면에서 피격 파티클 재생
void ACXPawn::MulticastPlayHitEffect_Implementation(FVector HitLocation)
{
	// 파티클이 설정되지 않았다면 실행하지 않음
	if (!IsValid(HitEffect))
	{
		return;
	}

	// 공격이 맞은 위치에 파티클 생성
	UGameplayStatics::SpawnEmitterAtLocation(
		GetWorld(),
		HitEffect,
		HitLocation
	);
}

#undef LOCTEXT_NAMESPACE