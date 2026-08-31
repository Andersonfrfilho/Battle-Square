// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Em que parte do dia o mundo está.
 *
 * Quatro, e não dois, porque o crepúsculo é onde mora metade do interesse: um
 * mundo que só alterna claro e escuro tem duas caras, e um que passa pelo
 * amanhecer e pelo entardecer tem quatro — e são justamente as duas faixas
 * curtas que fazem alguém parar de andar para olhar.
 */
enum class EDayPhase : uint8
{
	/** Amanhecer: o sol cruza o horizonte subindo. */
	Dawn,
	/** Dia claro. */
	Day,
	/** Entardecer: o sol cruza o horizonte descendo. */
	Dusk,
	/** Noite. */
	Night
};

/**
 * Quando uma criatura anda por aí.
 *
 * Não é enfeite de ficha: é o que decide a FREQUÊNCIA com que ela aparece a
 * cada hora. Sem isto, o ciclo do dia seria só iluminação — bonito e sem
 * consequência —, e o jogador não teria motivo nenhum para esperar anoitecer.
 */
enum class EPetActivity : uint8
{
	/** Diurno: aparece de dia. */
	Diurnal,
	/** Tardio: amanhecer e entardecer são a hora dele. */
	Crepuscular,
	/** Noturno. */
	Nocturnal
};

/**
 * O relógio do mundo, e o que ele decide.
 *
 * Puro e fora do ator de propósito, pelo mesmo motivo do gelo da serra
 * (`ScenaryClimate`): o que o sol faz às cinco da manhã é aritmética, e
 * aritmética se verifica sem levantar mundo, sem carregar malha e sem esperar
 * vinte minutos de jogo para a noite chegar.
 *
 * `ScenaryClimate` responde ONDE; este arquivo responde QUANDO. São as duas
 * perguntas que o mundo faz sobre si mesmo, e por isso moram lado a lado.
 *
 * **A hora é a única fonte.** A fase, a altura do sol, a cor da luz e a
 * frequência de cada bicho saem todas dela por função — nenhuma delas guarda
 * um segundo relógio, e nenhuma compara a hora com um limite por conta
 * própria (L-032/L-033). O sintoma de duas fontes seria o mais confuso
 * possível: a tela escurecendo enquanto o painel diz que é meio-dia.
 */
namespace WorldTimeOfDay
{
	/** Horas que um dia tem. Não é ajuste de jogo — é o que "hora" significa. */
	constexpr float HoursPerDay = 24.0f;

	/**
	 * Quanto dura um dia inteiro, em segundos de relógio de parede.
	 *
	 * Vinte minutos. Um dia real deixaria a noite fora de qualquer sessão, e
	 * um dia de dois minutos transformaria o céu num pisca-pisca: a fase
	 * mudaria mais rápido do que se leva para atravessar a ilha, e ninguém
	 * conseguiria PLANEJAR caçar um bicho noturno.
	 */
	constexpr float DefaultSecondsPerDay = 1200.0f;

	/**
	 * A hora em que cada fase começa.
	 *
	 * Estão aqui, juntas e visíveis, porque a fase e a altura do sol precisam
	 * concordar: noite declarada com o sol no céu é o defeito que este bloco
	 * existe para tornar impossível de escrever sem perceber.
	 */
	constexpr float DawnStartHour = 5.0f;
	constexpr float DayStartHour = 7.0f;
	constexpr float DuskStartHour = 17.0f;
	constexpr float NightStartHour = 19.0f;

	/** A hora do dia, de 0 a 24, depois de tantos segundos de jogo. */
	BATTLESQUARE_API float HourAt(float ElapsedSeconds, float SecondsPerDay = DefaultSecondsPerDay);

	/** Em que fase esta hora cai. */
	BATTLESQUARE_API EDayPhase PhaseAtHour(float Hour);

	/**
	 * A altura do sol acima do horizonte, em graus, de -90 a 90.
	 *
	 * Negativo quer dizer sol abaixo da linha — é assim que a noite acontece
	 * pela MESMA conta que faz o meio-dia, sem um modo separado para escuro.
	 */
	BATTLESQUARE_API float SunElevationDegrees(float Hour);

	/**
	 * A inclinação para o componente de luz direcional.
	 *
	 * A luz aponta para BAIXO quando o sol está em cima, então este número é
	 * o oposto da elevação. Existe para ninguém precisar lembrar do sinal na
	 * hora de girar o sol — errar o sinal ilumina a cena de baixo para cima,
	 * e o defeito parece ser do material.
	 */
	BATTLESQUARE_API float SunPitchDegrees(float Hour);

	/**
	 * De que lado do céu o sol está, em graus, dando uma volta por dia.
	 *
	 * Sem isto o sol subiria e desceria pelo MESMO lado, e o amanhecer e o
	 * entardecer chegariam da mesma direção — a sombra da montanha cairia
	 * duas vezes no mesmo lugar, que é o tipo de erro que ninguém sabe
	 * nomear mas todo mundo sente como "estranho".
	 *
	 * Nasce no leste às 6, cruza o sul ao meio-dia, se põe no oeste às 18.
	 */
	BATTLESQUARE_API float SunAzimuthDegrees(float Hour);

	/** A rotação inteira da luz do sol nesta hora — inclinação e lado juntos. */
	BATTLESQUARE_API FRotator SunRotation(float Hour);

	/** Está de noite? Responde pela elevação, não por uma segunda tabela. */
	BATTLESQUARE_API bool IsNight(float Hour);

	/**
	 * Quanto o sol ilumina, de 0 a 1.
	 *
	 * Zero na noite fechada. Quem acende a cena multiplica a intensidade de
	 * pleno sol por isto, em vez de guardar um valor por fase.
	 */
	BATTLESQUARE_API float SunBrightness(float Hour);

	/**
	 * A cor da luz nesta hora.
	 *
	 * Quente e baixa no amanhecer e no entardecer, branca ao meio-dia, azul
	 * fria de noite. A cor é o que conta a hora antes de qualquer relógio na
	 * tela: ninguém lê número, mas todo mundo reconhece luz de fim de tarde.
	 */
	BATTLESQUARE_API FLinearColor SunColor(float Hour);

	/**
	 * Com que peso, de 0 a 100, um bicho desta atividade aparece nesta fase.
	 *
	 * Inteiro, e não float como o resto deste arquivo, pelo mesmo motivo da
	 * umidade em `ScenaryClimate`: este número decide sorteio, e sorteio que
	 * depende de arredondamento de float é sorteio que muda de resultado
	 * conforme quem converteu.
	 *
	 * **Nenhum peso é zero.** Um bicho impossível de encontrar fora da hora
	 * dele vira uma parede: quem joga de noite nunca completaria a coleção
	 * diurna. Raro convida a voltar noutro horário; impossível só frustra.
	 */
	BATTLESQUARE_API int32 EncounterWeightPercent(EPetActivity Activity, EDayPhase Phase);

	/** A atividade pelo nome escrito — `Diurnal`, `Crepuscular`, `Nocturnal`. */
	BATTLESQUARE_API EPetActivity ActivityFromName(FName Name);

	/**
	 * A hora de uma ESPÉCIE, tirada do id de catálogo dela.
	 *
	 * Derivada, e não escrita numa segunda tabela do `.ini`: o id já é o que
	 * decide o corpo do bicho (`FPetMorphology::FromSeed`), e a espécie que
	 * tem um corpo estável precisa ter uma hora estável pelo mesmo motivo —
	 * o bicho que era noturno ontem e diurno hoje não é uma espécie, é um
	 * sorteio. Tabela paralela concordaria com o espelho de pets até a
	 * primeira edição (L-032).
	 *
	 * O fluxo é semeado por `id + "|atividade"`, e não pelo id cru, para não
	 * andar de mãos dadas com uma proporção do corpo: independentes de
	 * propósito, senão todo bicho noturno sairia com o mesmo focinho.
	 *
	 * Id vazio é diurno, como em `ActivityFromName(NAME_None)`: pet sem id
	 * ainda é um pet, e o mundo do meio-dia é onde ele aparece.
	 */
	BATTLESQUARE_API EPetActivity ActivityForSpecies(const FString& CatalogId);

	/**
	 * Qual espécie do catálogo aparece AGORA — sorteio pesado pela fase.
	 *
	 * Mora aqui, junto da tabela de pesos, porque a pergunta é uma só: quem
	 * anda por aí nesta hora. Espalhar o sorteio pelo `GameMode` deixaria a
	 * regra sem teste, e ela é justamente a que dá consequência ao ciclo do
	 * dia — sem ela, anoitecer é só a luz mudando de cor.
	 *
	 * Devolve `INDEX_NONE` só para catálogo vazio. Peso total zero não
	 * acontece: nenhum peso da tabela é zero, e é essa invariante que
	 * garante que sempre há alguém para encontrar.
	 */
	BATTLESQUARE_API int32 PickSpeciesForPhase(
		const TArray<FString>& CatalogIds, EDayPhase Phase, FRandomStream& Stream);

	/** O nome da fase, para o painel. Desenvolvimento, não texto de jogador. */
	BATTLESQUARE_API const TCHAR* PhaseDebugName(EDayPhase Phase);
}
