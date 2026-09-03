// Copyright 2026 Anderson. All Rights Reserved.

#include "World/NpcDialogue.h"

#include "Dom/JsonObject.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	/**
	 * OS FATOS que o modelo pode citar — e são as MESMAS dicas dos moradores:
	 * cada uma aponta mecânica com teste. Uma segunda lista de fatos aqui
	 * divergiria da primeira (L-032); por isso os fatos do digesto são as
	 * falas que o morador já diz.
	 */
	const TCHAR* FatosDoMundo[] = {
		TEXT("o Centro de Recuperacao cura de graca, em toda vila que o tem"),
		TEXT("o patio atras da Escola treina atributos e destrava golpes"),
		TEXT("a Arena de cada vila tem um campeao fixo, que se aprende"),
		TEXT("fazenda, criadouro e pomar pagam trabalho; o pet certo rende mais"),
		TEXT("pet capturado se vende no Mercado; a cidade grande paga menos"),
		TEXT("ponte destruida nao passa; rio fundo se atravessa nadando"),
		TEXT("quem cai em batalha acorda no Centro de Recuperacao mais perto"),
	};
}

FString NpcDialogue::BuildDigestJson(
	const VillageResidents::FResident& Morador, int32 Meetings,
	const VillageResidents::FPlayerDeeds& Feitos, const FString& PlayerSays)
{
	const TSharedRef<FJsonObject> Raiz = MakeShared<FJsonObject>();

	const TSharedRef<FJsonObject> Npc = MakeShared<FJsonObject>();
	Npc->SetStringField(TEXT("name"), Morador.Name);
	Npc->SetStringField(TEXT("storyStage"), Morador.TipLine);
	Npc->SetNumberField(TEXT("meetings"), Meetings);
	Npc->SetNumberField(TEXT("homeStartHour"), Morador.HomeStartHour);
	Raiz->SetObjectField(TEXT("npc"), Npc);

	const TSharedRef<FJsonObject> Deeds = MakeShared<FJsonObject>();
	Deeds->SetBoolField(TEXT("beatChampion"), Feitos.bBeatChampion);
	Deeds->SetBoolField(TEXT("soldAPet"), Feitos.bHasSoldAPet);
	Deeds->SetBoolField(TEXT("hasWaterPet"), Feitos.bHasWaterPet);
	Raiz->SetObjectField(TEXT("deeds"), Deeds);

	TArray<TSharedPtr<FJsonValue>> Fatos;
	for (const TCHAR* Fato : FatosDoMundo)
	{
		Fatos.Add(MakeShared<FJsonValueString>(Fato));
	}
	Raiz->SetArrayField(TEXT("facts"), Fatos);

	Raiz->SetStringField(TEXT("playerSays"), PlayerSays);

	// CONDENSADO, não pretty-print: isto é formato de fio, e foi o espaço do
	// pretty (`"campo": true`) que fez a primeira asserção do teste falhar.
	FString Json;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Escritor =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
	FJsonSerializer::Serialize(Raiz, Escritor);
	return Json;
}

FString NpcDialogue::RestrictedReply(
	const VillageResidents::FResident& Morador, ESettlementKind Kind,
	int32 DoorIndex, int32 Meetings, const VillageResidents::FPlayerDeeds& Feitos)
{
	// O modo restrito ADMITE o limite e devolve o que sabe: a história do
	// estágio de agora, reagindo aos feitos como a visita reage. Improvisar
	// aqui seria fingir o modo dinâmico sem o modelo — a mentira mais cara.
	return FString::Printf(TEXT("disso eu nao sei falar... mas escuta: %s"),
		*VillageResidents::StoryLineReacting(
			Morador, Kind, DoorIndex, FMath::Max(2, Meetings), Feitos));
}
