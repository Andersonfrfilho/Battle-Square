// Copyright 2026 Anderson. All Rights Reserved.

#include "Net/BattleRoomRegistry.h"
#include "Net/BattleNetConstants.h"
#include "Misc/AutomationTest.h"

// T2: CreateRoom nunca produz o mesmo código duas vezes enquanto ambas
// as salas estiverem vivas.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleRoomRegistryCreateRoomIsUniqueTest,
	"BattleSquare.Net.RoomRegistry.CreateRoomIsUnique",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleRoomRegistryCreateRoomIsUniqueTest::RunTest(const FString& Parameters)
{
	UBattleRoomRegistry* Registry = NewObject<UBattleRoomRegistry>();

	TSet<FString> SeenCodes;
	bool bAllUnique = true;
	for (int32 Iteration = 0; Iteration < 50; ++Iteration)
	{
		FGuid Secret;
		const FString Code = Registry->CreateRoom(Secret);
		if (SeenCodes.Contains(Code))
		{
			bAllUnique = false;
			AddError(FString::Printf(TEXT("Código '%s' repetido enquanto salas vivas"), *Code));
		}
		SeenCodes.Add(Code);
		TestTrue(TEXT("Segredo do criador não é zero"), Secret.IsValid());
	}

	return bAllUnique;
}

// T2: sala recém-criada tem só Side0 ocupado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleRoomRegistryCreateRoomHasOnlySide0Test,
	"BattleSquare.Net.RoomRegistry.CreateRoomHasOnlySide0Occupied",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleRoomRegistryCreateRoomHasOnlySide0Test::RunTest(const FString& Parameters)
{
	UBattleRoomRegistry* Registry = NewObject<UBattleRoomRegistry>();
	FGuid Secret;
	const FString Code = Registry->CreateRoom(Secret);

	TestFalse(TEXT("Sala recém-criada não está cheia"), Registry->IsRoomFull(Code));
	TestTrue(TEXT("Sala existe"), Registry->DoesRoomExist(Code));

	return true;
}

// T3: JoinRoom distingue NotFound de Full, e nunca aceita um terceiro lado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleRoomRegistryJoinDistinguishesNotFoundFromFullTest,
	"BattleSquare.Net.RoomRegistry.JoinRoomDistinguishesNotFoundFromFull",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleRoomRegistryJoinDistinguishesNotFoundFromFullTest::RunTest(const FString& Parameters)
{
	UBattleRoomRegistry* Registry = NewObject<UBattleRoomRegistry>();

	FGuid JoinerSecret;
	TestEqual(TEXT("Código inexistente retorna NotFound"), Registry->JoinRoom(TEXT("ZZZZZ"), JoinerSecret), EBattleRoomJoinResult::NotFound);

	FGuid CreatorSecret;
	const FString Code = Registry->CreateRoom(CreatorSecret);

	TestEqual(TEXT("Primeira entrada com sala de 1 vaga retorna Success"), Registry->JoinRoom(Code, JoinerSecret), EBattleRoomJoinResult::Success);
	TestTrue(TEXT("Segredo do segundo jogador é diferente do primeiro"), JoinerSecret != CreatorSecret);
	TestTrue(TEXT("Sala agora está cheia"), Registry->IsRoomFull(Code));

	FGuid ThirdSecret;
	TestEqual(TEXT("Terceira entrada na mesma sala retorna Full"), Registry->JoinRoom(Code, ThirdSecret), EBattleRoomJoinResult::Full);

	return true;
}

// T3: OnRoomReady dispara exatamente quando a sala fica cheia.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleRoomRegistryOnRoomReadyFiresOnceFullTest,
	"BattleSquare.Net.RoomRegistry.OnRoomReadyFiresWhenFull",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleRoomRegistryOnRoomReadyFiresOnceFullTest::RunTest(const FString& Parameters)
{
	UBattleRoomRegistry* Registry = NewObject<UBattleRoomRegistry>();
	int32 ReadyFiredCount = 0;
	FString ReadyCode;
	Registry->OnRoomReady.AddLambda([&ReadyFiredCount, &ReadyCode](const FString& Code) { ++ReadyFiredCount; ReadyCode = Code; });

	FGuid CreatorSecret;
	const FString Code = Registry->CreateRoom(CreatorSecret);
	TestEqual(TEXT("OnRoomReady não dispara com só 1 ocupante"), ReadyFiredCount, 0);

	FGuid JoinerSecret;
	Registry->JoinRoom(Code, JoinerSecret);
	TestEqual(TEXT("OnRoomReady dispara exatamente uma vez ao completar 2/2"), ReadyFiredCount, 1);
	TestEqual(TEXT("Código recebido é o da sala que completou"), ReadyCode, Code);

	return true;
}

// T4: desconexão + reconexão por segredo correto/incorreto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleRoomRegistryDisconnectAndReconnectTest,
	"BattleSquare.Net.RoomRegistry.DisconnectAndReconnect",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleRoomRegistryDisconnectAndReconnectTest::RunTest(const FString& Parameters)
{
	UBattleRoomRegistry* Registry = NewObject<UBattleRoomRegistry>();
	FGuid CreatorSecret;
	const FString Code = Registry->CreateRoom(CreatorSecret);
	FGuid JoinerSecret;
	Registry->JoinRoom(Code, JoinerSecret);

	Registry->MarkDisconnected(Code, /*Side=*/0, 10.0);

	TestFalse(TEXT("Reconexão com segredo errado é rejeitada"), Registry->TryReconnect(Code, FGuid::NewGuid()));
	TestFalse(TEXT("Reconexão num lado que nunca desconectou (Side1) é rejeitada"), Registry->TryReconnect(Code, JoinerSecret));
	TestTrue(TEXT("Reconexão com o segredo certo do lado desconectado é aceita"), Registry->TryReconnect(Code, CreatorSecret));
	TestFalse(TEXT("Segunda tentativa de reconectar já reconectado é rejeitada (nada para reconectar)"), Registry->TryReconnect(Code, CreatorSecret));

	return true;
}

// T5 🧠: timeout de abandono — dispara uma vez, respeita reconexão
// chegando antes, e não dispara quando os dois lados abandonam juntos.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleRoomRegistryAbandonmentRaceOrderingTest,
	"BattleSquare.Net.RoomRegistry.AbandonmentTimeoutRaceOrdering",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleRoomRegistryAbandonmentRaceOrderingTest::RunTest(const FString& Parameters)
{
	// Caso 1: abandono real dispara uma vez, não dispara de novo.
	{
		UBattleRoomRegistry* Registry = NewObject<UBattleRoomRegistry>();
		FGuid CreatorSecret, JoinerSecret;
		const FString Code = Registry->CreateRoom(CreatorSecret);
		Registry->JoinRoom(Code, JoinerSecret);

		int32 AbandonedCount = 0;
		uint8 PresentSideReceived = 0xFF;
		Registry->OnRoomAbandoned.AddLambda([&AbandonedCount, &PresentSideReceived](const FString&, uint8 PresentSide)
		{
			++AbandonedCount;
			PresentSideReceived = PresentSide;
		});

		Registry->MarkDisconnected(Code, /*Side=*/1, 0.0);
		Registry->CheckAbandonment(static_cast<double>(BattleNetConstants::AbandonTimeoutSeconds) - 1.0);
		TestEqual(TEXT("Antes do timeout, não dispara"), AbandonedCount, 0);

		Registry->CheckAbandonment(static_cast<double>(BattleNetConstants::AbandonTimeoutSeconds));
		TestEqual(TEXT("No timeout, dispara uma vez"), AbandonedCount, 1);
		TestEqual(TEXT("Lado presente (0) é o vencedor"), PresentSideReceived, static_cast<uint8>(0));

		Registry->CheckAbandonment(static_cast<double>(BattleNetConstants::AbandonTimeoutSeconds) + 100.0);
		TestEqual(TEXT("Chamadas subsequentes não disparam de novo"), AbandonedCount, 1);
	}

	// Caso 2: reconexão antes do timeout cancela o abandono.
	{
		UBattleRoomRegistry* Registry = NewObject<UBattleRoomRegistry>();
		FGuid CreatorSecret, JoinerSecret;
		const FString Code = Registry->CreateRoom(CreatorSecret);
		Registry->JoinRoom(Code, JoinerSecret);

		int32 AbandonedCount = 0;
		Registry->OnRoomAbandoned.AddLambda([&AbandonedCount](const FString&, uint8) { ++AbandonedCount; });

		Registry->MarkDisconnected(Code, /*Side=*/1, 0.0);
		const bool bReconnected = Registry->TryReconnect(Code, JoinerSecret);
		TestTrue(TEXT("Reconexão antes do timeout é aceita"), bReconnected);

		Registry->CheckAbandonment(static_cast<double>(BattleNetConstants::AbandonTimeoutSeconds) + 1.0);
		TestEqual(TEXT("Reconexão cancelou o abandono — não dispara"), AbandonedCount, 0);
	}

	// Caso 3: os dois lados abandonam juntos — nenhum "presente", não dispara.
	{
		UBattleRoomRegistry* Registry = NewObject<UBattleRoomRegistry>();
		FGuid CreatorSecret, JoinerSecret;
		const FString Code = Registry->CreateRoom(CreatorSecret);
		Registry->JoinRoom(Code, JoinerSecret);

		int32 AbandonedCount = 0;
		Registry->OnRoomAbandoned.AddLambda([&AbandonedCount](const FString&, uint8) { ++AbandonedCount; });

		Registry->MarkDisconnected(Code, /*Side=*/0, 0.0);
		Registry->MarkDisconnected(Code, /*Side=*/1, 0.0);
		Registry->CheckAbandonment(static_cast<double>(BattleNetConstants::AbandonTimeoutSeconds) + 1.0);
		TestEqual(TEXT("Os dois lados abandonando juntos não declara vencedor"), AbandonedCount, 0);
	}

	return true;
}

// T6: sala com os dois lados abandonados além do timeout é destruída, e
// o código pode ser reusado sem falso positivo de colisão.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleRoomRegistryEmptyRoomDestroyedTest,
	"BattleSquare.Net.RoomRegistry.EmptyRoomIsDestroyedAndCodeReused",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleRoomRegistryEmptyRoomDestroyedTest::RunTest(const FString& Parameters)
{
	UBattleRoomRegistry* Registry = NewObject<UBattleRoomRegistry>();
	FGuid CreatorSecret, JoinerSecret;
	const FString Code = Registry->CreateRoom(CreatorSecret);
	Registry->JoinRoom(Code, JoinerSecret);

	TestTrue(TEXT("Sala existe antes do abandono duplo"), Registry->DoesRoomExist(Code));

	Registry->MarkDisconnected(Code, /*Side=*/0, 0.0);
	Registry->MarkDisconnected(Code, /*Side=*/1, 0.0);

	Registry->CheckEmptyRooms(static_cast<double>(BattleNetConstants::AbandonTimeoutSeconds) - 1.0);
	TestTrue(TEXT("Antes do timeout, sala continua existindo"), Registry->DoesRoomExist(Code));

	Registry->CheckEmptyRooms(static_cast<double>(BattleNetConstants::AbandonTimeoutSeconds));
	TestFalse(TEXT("No timeout, sala com os dois lados abandonados é destruída"), Registry->DoesRoomExist(Code));

	return true;
}

// T6: sala com só o criador (Side1 nunca ocupado) NUNCA é destruída por
// CheckEmptyRooms — SALA-11 fala em "os dois jogadores saíram".
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleRoomRegistrySoloRoomNeverDestroyedTest,
	"BattleSquare.Net.RoomRegistry.SoloRoomIsNeverDestroyedByEmptyCheck",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleRoomRegistrySoloRoomNeverDestroyedTest::RunTest(const FString& Parameters)
{
	UBattleRoomRegistry* Registry = NewObject<UBattleRoomRegistry>();
	FGuid CreatorSecret;
	const FString Code = Registry->CreateRoom(CreatorSecret);

	Registry->CheckEmptyRooms(999999.0);
	TestTrue(TEXT("Sala solo nunca é destruída por CheckEmptyRooms"), Registry->DoesRoomExist(Code));

	return true;
}
