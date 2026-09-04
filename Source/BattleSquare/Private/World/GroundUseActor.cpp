// Copyright 2026 Anderson. All Rights Reserved.

#include "World/GroundUseActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Environment/ScenaryPalette.h"
#include "UObject/ConstructorHelpers.h"

namespace UsoDoSolo
{
	/**
	 * A TABELA: forma, cor e proporção de cada uso.
	 *
	 * Num lugar só. Espalhada pelos chamadores, ela viraria três aparências
	 * diferentes para o mesmo pomar na terceira edição (L-032).
	 */
	struct FAparencia
	{
		const TCHAR* Malha = nullptr;
		EScenaryRole Papel = EScenaryRole::GroundCover;

		/** Altura como fração da meia-extensão. Baixo é chão; alto é marco. */
		float Esbeltez = 0.2f;
	};

	const TCHAR* Cubo = ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive::Cube);
	const TCHAR* Cilindro = ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive::Cylinder);
	const TCHAR* Cone = ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive::Cone);
	const TCHAR* Esfera = ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive::Sphere);

	/**
	 * O poço SECO não é o poço cheio.
	 *
	 * O traçado guarda `bYieldsWater` e diz por quê: "um poço seco desenhado
	 * igual a um cheio é uma promessa que a carta não cumpre". Dez dos doze
	 * poços desta ilha são secos — desenhá-los todos iguais mandaria o jogador
	 * atravessar a ilha atrás de água que não existe.
	 *
	 * Fica ao lado da tabela, e não dentro dela, porque não é o USO que muda: é
	 * o estado de uma mancha daquele uso.
	 */
	FAparencia AparenciaDoPoco(bool bDaAgua)
	{
		return bDaAgua
			? FAparencia{ Cilindro, EScenaryRole::FreshWater, 0.5f }
			: FAparencia{ Cilindro, EScenaryRole::Rock, 0.5f };
	}

	FAparencia AparenciaDe(EGroundUse Uso)
	{
		switch (Uso)
		{
		// MATA: o que cresce. Copa arredondada, verde de mato.
		case EGroundUse::Bosque:
			return { Esfera, EScenaryRole::CanopyTree, 1.2f };
		case EGroundUse::ClareiraFechada:
			return { Cilindro, EScenaryRole::Undergrowth, 0.08f };

		// TRABALHO: o que se cultiva e se cria. Baixo e largo — é chão usado.
		case EGroundUse::Fazenda:
			return { Cubo, EScenaryRole::GroundCover, 0.12f };
		case EGroundUse::Criadouro:
			return { Cubo, EScenaryRole::DeadWood, 0.35f };
		case EGroundUse::Pomar:
			return { Esfera, EScenaryRole::ForestTree, 0.9f };
		case EGroundUse::PomarSelvagem:
			return { Esfera, EScenaryRole::Undergrowth, 0.9f };

		// GENTE: onde se para. Formas construídas, alto o bastante para achar.
		case EGroundUse::Loja:
			return { Cubo, EScenaryRole::Accent, 0.8f };
		case EGroundUse::Acampamento:
			return { Cone, EScenaryRole::DeadWood, 0.8f };
		case EGroundUse::Deck:
			return { Cubo, EScenaryRole::DeadWood, 0.1f };
		case EGroundUse::Poco:
			return { Cilindro, EScenaryRole::Rock, 0.5f };

		// SAGRADO: o que se vê de longe. Os mais altos do mapa, de propósito —
		// um templo que não se avista do caminho não orienta ninguém.
		case EGroundUse::Templo:
			return { Cilindro, EScenaryRole::MountainSnow, 2.2f };
		case EGroundUse::Ruina:
			return { Cilindro, EScenaryRole::Rock, 1.1f };
		case EGroundUse::Cemiterio:
			return { Cubo, EScenaryRole::Rock, 0.6f };
		case EGroundUse::CemiterioEsquecido:
			return { Cubo, EScenaryRole::DeadWood, 0.45f };

		// ESCONDIDO: baixo e apagado, do tamanho de umas barracas. O que se
		// avista de longe não é mercado-negro.
		case EGroundUse::MercadoNegro:
			return { Cubo, EScenaryRole::DeadWood, 0.5f };

		default: break;
		}

		// `Nenhum` não tem linha, e é o único que pode não ter.
		return {};
	}
}

const TCHAR* AGroundUseActor::MeshPathFor(EGroundUse Uso)
{
	const UsoDoSolo::FAparencia Aparencia = UsoDoSolo::AparenciaDe(Uso);
	return Aparencia.Malha ? Aparencia.Malha : TEXT("");
}

const TCHAR* AGroundUseActor::UseDebugName(EGroundUse Uso)
{
	switch (Uso)
	{
	case EGroundUse::Bosque:             return TEXT("bosque");
	case EGroundUse::ClareiraFechada:    return TEXT("clareira fechada");
	case EGroundUse::Fazenda:            return TEXT("fazenda");
	case EGroundUse::Criadouro:          return TEXT("criadouro");
	case EGroundUse::Loja:               return TEXT("loja");
	case EGroundUse::Acampamento:        return TEXT("acampamento");
	case EGroundUse::Pomar:              return TEXT("pomar");
	case EGroundUse::PomarSelvagem:      return TEXT("pomar selvagem");
	case EGroundUse::Deck:               return TEXT("deck");
	case EGroundUse::Poco:               return TEXT("poco");
	case EGroundUse::Templo:             return TEXT("templo");
	case EGroundUse::Ruina:              return TEXT("ruina");
	case EGroundUse::Cemiterio:          return TEXT("cemiterio");
	case EGroundUse::CemiterioEsquecido: return TEXT("cemiterio esquecido");
	case EGroundUse::MercadoNegro:       return TEXT("mercado-negro");
	default: break;
	}
	return TEXT("nenhum");
}

AGroundUseActor::AGroundUseActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundUseBody"));
	SetRootComponent(Body);

	// MALHA E COR NO CONSTRUTOR, mesmo antes de saber qual uso este ator vai
	// vestir. Componente que nasce sem asset e espera uma configuração passa em
	// todo teste de lógica e não existe na tela — três vezes neste projeto. A
	// configuração TROCA a malha; ela não é a primeira a atribuir uma.
	ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(UsoDoSolo::Cubo);
	if (Cubo.Succeeded())
	{
		Body->SetStaticMesh(Cubo.Object);
	}

	ScenaryPalette::PaintComponent(Body, EScenaryRole::GroundCover);

	// O uso do solo é MARCO, não parede: ele diz onde as coisas ficam. Colidir
	// poria 79 obstáculos sólidos no meio dos caminhos que o traçado abriu.
	Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetCollisionResponseToAllChannels(ECR_Overlap);
	Body->SetCanEverAffectNavigation(false);
}

bool AGroundUseActor::ConfigureFor(const FGroundUsePatch& Mancha)
{
	Use = Mancha.Use;
	Deity = Mancha.Deity;
	bYieldsWater = Mancha.bYieldsWater;

	const UsoDoSolo::FAparencia Aparencia = (Mancha.Use == EGroundUse::Poco)
		? UsoDoSolo::AparenciaDoPoco(Mancha.bYieldsWater)
		: UsoDoSolo::AparenciaDe(Mancha.Use);

	if (!Aparencia.Malha || !Body)
	{
		return false;
	}

	UStaticMesh* Malha = LoadObject<UStaticMesh>(nullptr, Aparencia.Malha);
	if (!Malha)
	{
		return false;
	}

	Body->SetStaticMesh(Malha);
	ScenaryPalette::PaintComponent(Body, Aparencia.Papel);

	// O tamanho sai da MEIA-EXTENSÃO da mancha, que é o que o traçado mediu.
	// Um tamanho fixo faria o bosque de uma clareira grande caber num vaso.
	//
	// A malha básica da engine tem 100 unidades de lado, então a escala é a
	// extensão dividida por isso — e não a extensão crua, que daria uma peça
	// cem vezes maior que o lugar que ela ocupa.
	constexpr float LadoDaMalhaBasica = 100.0f;
	const float Largura = (Mancha.HalfExtentUnits * 2.0f) / LadoDaMalhaBasica;

	Body->SetWorldScale3D(FVector(Largura, Largura, Largura * Aparencia.Esbeltez));

	return true;
}
