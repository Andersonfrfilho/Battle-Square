// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldStatusReadout.h"
#include "Meta/PetHealthRules.h"

#include "Meta/PetAttributeProgression.h"
#include "Meta/PetMoveRequirements.h"
#include "Meta/PetProgressionService.h"

#define LOCTEXT_NAMESPACE "WorldStatus"

namespace
{
	/** Distância em METROS, não em unidades da engine. */
	constexpr float UnidadesPorMetro = 100.0f;

	/** Perto o bastante para o jogador se preparar em vez de ser surpreendido. */
	constexpr float DistanciaDeAtencaoUnidades = 1500.0f;
}

TArray<FWorldStatusLine> FWorldStatusReadout::Build(const FWorldStatusSnapshot& Snapshot)
{
	TArray<FWorldStatusLine> Linhas;

	if (!Snapshot.bHasOwnedPet)
	{
		// Dizer que NÃO HÁ pet é informação; o silêncio seria indistinguível
		// de painel quebrado, e foi exatamente essa dúvida que custou uma
		// investigação inteira quando a XP não pousava em ninguém.
		Linhas.Add({ LOCTEXT("SemPet", "Você ainda não tem pet na coleção"), FColor::Orange });
	}
	else
	{
		const FOwnedPetInstance& Pet = Snapshot.OwnedPet;

		Linhas.Add({ FText::Format(
			LOCTEXT("SeuPet", "{Pet} — nível {Nivel} ({Experiencia} de experiência)"),
			FFormatNamedArguments{
				{ TEXT("Pet"), FText::FromString(Pet.Name) },
				{ TEXT("Nivel"), FText::AsNumber(FPetProgressionService::GetLevel(Pet)) },
				{ TEXT("Experiencia"), FText::AsNumber(Pet.Experience) },
			}), FColor::Cyan });

		// Os atributos numa linha só: cinco linhas separadas empurrariam para
		// fora do painel tudo o que vem depois, e o painel tem teto de altura.
		Linhas.Add({ FText::Format(
			LOCTEXT("SeusAtributos", "{Musculatura} {ValorMusculatura} · {Personalidade} {ValorPersonalidade} · {Camuflagem} {ValorCamuflagem} · {Voo} {ValorVoo} · {Subsolo} {ValorSubsolo}"),
			FFormatNamedArguments{
				{ TEXT("Musculatura"), FPetMoveRequirements::GetAttributeLabel(TEXT("musculature")) },
				{ TEXT("ValorMusculatura"), FText::AsNumber(Pet.Musculature) },
				{ TEXT("Personalidade"), FPetMoveRequirements::GetAttributeLabel(TEXT("personality")) },
				{ TEXT("ValorPersonalidade"), FText::AsNumber(Pet.Personality) },
				{ TEXT("Camuflagem"), FPetMoveRequirements::GetAttributeLabel(TEXT("camouflage")) },
				{ TEXT("ValorCamuflagem"), FText::AsNumber(Pet.SkillProficiency[FPetAttributeProgression::Camouflage]) },
				{ TEXT("Voo"), FPetMoveRequirements::GetAttributeLabel(TEXT("flight")) },
				{ TEXT("ValorVoo"), FText::AsNumber(Pet.SkillProficiency[FPetAttributeProgression::Flight]) },
				{ TEXT("Subsolo"), FPetMoveRequirements::GetAttributeLabel(TEXT("underground")) },
				{ TEXT("ValorSubsolo"), FText::AsNumber(Pet.SkillProficiency[FPetAttributeProgression::Underground]) },
			}), FColor::White });

		// A VIDA na tela, e SÓ quando falta: pet cheio não ocupa altura —
		// mesma regra da fundura em terra seca. Quem anda machucado precisa
		// saber ANTES do próximo encontro, e é esta linha que transforma
		// "acho que apanhei muito" em decisão de ir ao Centro de Recuperação.
		if (FPetHealthRules::IsHurt(Pet.HealthPercent))
		{
			Linhas.Add({ FText::Format(
				LOCTEXT("VidaDoPet", "vida: {Percentual}% — o Centro de Recuperação cura de graça"),
				FFormatNamedArguments{
					{ TEXT("Percentual"), FText::AsNumber(Pet.HealthPercent) },
				}),
				Pet.HealthPercent < 35 ? FColor::Red : FColor::Orange });
		}

		// A BIOLOGIA na tela, e só quando ela existe.
		//
		// Sem esta linha, dois pets do mesmo elemento aguentam a lava de
		// formas diferentes e o jogador não tem como saber por quê: ele leria
		// o mesmo elemento nos dois e concluiria que o jogo é inconsistente.
		// Uma linha fixa dizendo "biologia: nenhuma" nos pets antigos seria
		// ruído — a ausência não decide nada, então não ocupa altura.
		const FString Biologia = FString::Join(
			TArray<FString>(TArray<FString>{
				Pet.BiologySkin, Pet.BiologyBuild,
				Pet.BiologyBreathing, Pet.BiologyLimbs }
				.FilterByPredicate([](const FString& Eixo) { return !Eixo.IsEmpty(); })),
			TEXT(" · "));

		if (!Biologia.IsEmpty())
		{
			Linhas.Add({ FText::Format(
				LOCTEXT("SuaBiologia", "biologia: {Eixos}"),
				FFormatNamedArguments{
					{ TEXT("Eixos"), FText::FromString(Biologia) },
				}), FColor(180, 220, 180) });
		}
	}

	// O QUE ELE VESTE, e o que há para vestir.
	//
	// Item que ninguém vê é item que ninguém equipa: sem estas linhas, a bota
	// de lava decide a travessia e o jogador não tem onde ler que ela existe.
	// Some quando não há nada — linha fixa dizendo "nada equipado" ensinaria a
	// ignorá-la justo no dia em que ela passasse a ter conteúdo.
	if (Snapshot.EquippedItemNames.Num() > 0)
	{
		Linhas.Add({ FText::Format(
			LOCTEXT("SeusItens", "vestindo: {Itens}"),
			FFormatNamedArguments{
				{ TEXT("Itens"), FText::FromString(
					FString::Join(Snapshot.EquippedItemNames, TEXT(" · "))) },
			}), FColor(220, 200, 140) });
	}

	if (Snapshot.BackpackLines.Num() > 0)
	{
		Linhas.Add({ FText::Format(
			LOCTEXT("SuaMochila", "mochila: {Itens}"),
			FFormatNamedArguments{
				{ TEXT("Itens"), FText::FromString(
					FString::Join(Snapshot.BackpackLines, TEXT(" · "))) },
			}), FColor(180, 180, 200) });
	}

	if (Snapshot.EncountersAlive <= 0)
	{
		Linhas.Add({ LOCTEXT("MundoVazio", "Nenhum adversário por perto"), FColor::Silver });
		return Linhas;
	}

	// Distância ausente com inimigos vivos não é erro: eles existem e ainda
	// não se sabe onde. Fingir zero diria "colado em você".
	if (Snapshot.DistanceToNearestUnits < 0.0f)
	{
		Linhas.Add({ FText::Format(
			LOCTEXT("AdversariosSemDistancia", "{Quantos} adversário(s) no mundo"),
			FFormatNamedArguments{ { TEXT("Quantos"), FText::AsNumber(Snapshot.EncountersAlive) } }),
			FColor::Silver });
		return Linhas;
	}

	const int32 Metros = FMath::RoundToInt(Snapshot.DistanceToNearestUnits / UnidadesPorMetro);
	const bool bPerto = Snapshot.DistanceToNearestUnits <= DistanciaDeAtencaoUnidades;

	Linhas.Add({ FText::Format(
		LOCTEXT("Adversarios", "{Quantos} adversário(s) — o mais perto a {Metros} m"),
		FFormatNamedArguments{
			{ TEXT("Quantos"), FText::AsNumber(Snapshot.EncountersAlive) },
			{ TEXT("Metros"), FText::AsNumber(Metros) },
		}),
		// A COR é o aviso que se lê sem ler: quem está andando não acompanha
		// um número mudando, mas percebe a linha ficando amarela.
		bPerto ? FColor::Yellow : FColor::Silver });

	return Linhas;
}

#undef LOCTEXT_NAMESPACE
