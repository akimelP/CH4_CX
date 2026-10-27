#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CXChatWidget.generated.h"

// 사용할 UI 클래스 전방 선언
class UScrollBox;
class UEditableTextBox;

UCLASS()
class CX_API UCXChatWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    // 위젯 생성 시 호출
    virtual void NativeConstruct() override;

    // 채팅 메시지 표시 영역
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UScrollBox> ChatScrollBox;

    // 채팅 입력창
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> ChatInputBox;

    // Enter 입력 처리 함수
    UFUNCTION()
    void OnChatTextCommitted(
        const FText& Text,
        ETextCommit::Type CommitMethod
    );

public:
    // 서버에서 전달받은 메시지를 채팅창에 표시
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void AddChatMessage(const FString& Message);

    // 채팅 입력창에 키보드 포커스를 설정
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void FocusChatInput();
};