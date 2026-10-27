#include "CXChatWidget.h"
#include "Components/EditableTextBox.h"
#include "CXPlayerController.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"

// 위젯이 생성될 때 이벤트 연결
void UCXChatWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (ChatInputBox)
    {
        // 입력이 확정되면 OnChatTextCommitted 함수 실행
        ChatInputBox->OnTextCommitted.AddUniqueDynamic(
            this,
            &UCXChatWidget::OnChatTextCommitted
        );
    }
}

// 채팅 입력 완료 이벤트
void UCXChatWidget::OnChatTextCommitted(
    const FText& Text,
    ETextCommit::Type CommitMethod
)
{
    // Enter로 입력을 완료한 경우에만 처리
    if (CommitMethod != ETextCommit::OnEnter)
    {
        return;
    }

    // 현재 위젯을 소유한 PlayerController
    ACXPlayerController* PC = GetOwningPlayer<ACXPlayerController>();

    if (!IsValid(PC))
    {
        return;
    }

    // 입력 메시지의 앞뒤 공백 제거
    const FString Message = Text.ToString().TrimStartAndEnd();

    // 내용이 있을 때만 서버로 전송
    if (!Message.IsEmpty())
    {
        PC->ServerSendChatMessage(Message);
    }

    // 입력창 비우기
    ChatInputBox->SetText(FText::GetEmpty());

    // 채팅 종료 후 차량 조작으로 복귀
    PC->CloseChat();
}

// 채팅 메시지를 화면에 추가
void UCXChatWidget::AddChatMessage(const FString& Message)
{
    // 채팅 메시지 영역이 유효한지 확인
    if (!IsValid(ChatScrollBox))
    {
        return;
    }

    // 새로운 텍스트 위젯 생성
    UTextBlock* NewMessage = NewObject<UTextBlock>(this);

    if (!IsValid(NewMessage))
    {
        return;
    }

    // 텍스트 위젯에 채팅 메시지 설정
    NewMessage->SetText(FText::FromString(Message));

    // 긴 메시지는 자동 줄바꿈
    NewMessage->SetAutoWrapText(true);

    // 스크롤 박스에 메시지 추가
    ChatScrollBox->AddChild(NewMessage);

    // 가장 최근 메시지가 보이도록 아래로 이동
    ChatScrollBox->ScrollToEnd();
}

// 채팅 입력창 활성화
void UCXChatWidget::FocusChatInput()
{
    if (IsValid(ChatInputBox))
    {
        // 입력창에 키보드 포커스 설정
        ChatInputBox->SetKeyboardFocus();
    }
}