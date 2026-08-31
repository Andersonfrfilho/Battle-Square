// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Environment/ScenaryClimate.h"
#include "Environment/WorldWeather.h"

/**
 * Em que fase a lua está.
 *
 * Oito, e não quatro, porque o quarto crescente e a gibosa crescente não se
 * parecem: quem olha o céu reconhece a foice fina, o meio-disco e o disco
 * quase cheio como coisas diferentes, e nomear só os quartos jogaria fora
 * três quartos das noites.
 */
enum class EMoonPhase : uint8
{
	/** Lua nova: o lado iluminado está virado para o outro lado. */
	New,
	/** Foice crescente. */
	WaxingCrescent,
	/** Quarto crescente: meio disco, crescendo. */
	FirstQuarter,
	/** Gibosa crescente: quase cheia. */
	WaxingGibbous,
	/** Cheia. */
	Full,
	/** Gibosa minguante. */
	WaningGibbous,
	/** Quarto minguante: meio disco, minguando. */
	LastQuarter,
	/** Foice minguante. */
	WaningCrescent
};

/**
 * Que eclipse está acontecendo agora.
 *
 * Um enum, e não dois booleanos, porque os dois não podem acontecer juntos:
 * o eclipse lunar exige a lua CHEIA e o solar exige a lua NOVA, e ela não
 * pode estar nos dois lugares da órbita ao mesmo tempo. Dois booleanos
 * deixariam escrever no código um estado que o céu não sabe produzir.
 */
enum class ESkyEclipse : uint8
{
	/** Nenhum. */
	None,
	/** Eclipse lunar — e é ele que faz a LUA VERMELHA. */
	Lunar,
	/** Eclipse solar: a lua nova passa na frente do sol, de dia. */
	Solar
};

/**
 * O céu noturno, e o que ele decide.
 *
 * Puro e fora do ator, como o relógio (`WorldTimeOfDay`) e o clima
 * (`WorldWeather`): onde a lua está na terça-feira é aritmética, e aritmética
 * se verifica sem levantar mundo e sem esperar quatro semanas de jogo para a
 * lua cheia chegar.
 *
 * **A lua vermelha e o eclipse lunar são o MESMO fenômeno.** A lua fica
 * vermelha porque está eclipsada: a atmosfera da Terra filtra o azul e só o
 * vermelho chega até ela. Então não são dois sistemas com duas chances de
 * discordar — é um, e `IsBloodMoon` é literalmente uma pergunta sobre o
 * eclipse. O eclipse solar é a mesma conta com a lua no outro lado da órbita.
 *
 * **Duas voltas, e só duas, explicam tudo aqui:**
 *
 * 1. A volta da FASE (mês sinódico, 29,5 dias) — onde a lua está em relação
 *    ao sol. Ela decide a fase, quanto do disco está aceso, e a que hora a
 *    lua nasce: a cheia está do lado oposto ao sol, então ela sobe quando ele
 *    se põe. Por isso não existe tabela de horários — o atraso É a fase.
 *
 * 2. A volta do NÓ (mês draconiano, 27,2 dias) — onde a lua cruza o caminho
 *    do sol. Sem ela haveria eclipse todo mês, porque toda lua cheia ficaria
 *    exatamente atrás da Terra.
 *
 * Os dois períodos não se dividem, e é justamente aí que está a graça: as
 * duas voltas batem juntas só de vez em quando, então o eclipse vem raro e em
 * intervalos desiguais — **sem sorteio nenhum**. Um `Rand` daria raridade e
 * tiraria a única coisa que faz o jogador aprender a esperar: o padrão.
 */
namespace WorldNightSky
{
	/**
	 * A volta da fase, em dias de jogo.
	 *
	 * Oito, e não os 29,5 do céu de verdade. Um dia daqui dura vinte minutos
	 * de relógio de parede: com o mês fiel, uma lunação levaria dez horas de
	 * jogo e ninguém veria a lua mudar de fase — a lua teria fase no código e
	 * nenhuma no produto.
	 *
	 * **O que se preserva não é o tamanho, é a RAZÃO entre as duas voltas.**
	 * 8,3 / 7,6528 = 1,08456, que é exatamente 29,5 / 27,2. É a razão que
	 * produz o ritmo do eclipse — o compasso lento em que as duas voltas batem
	 * juntas — e é por isso que ela é a parte que não se mexe.
	 *
	 * **E o mês NÃO é um número redondo de dias, de propósito.** Com oito
	 * exatos, toda lua cheia cairia à meia-noite e toda lua nova ao meio-dia
	 * do mesmo jeito, para sempre; a lua nova ficaria presa no alto ao
	 * meio-dia e o eclipse solar seria possível todo mês ou nenhum. Medido:
	 * com 8,0 não acontece **um único** eclipse solar em duzentos dias. A
	 * vírgula é o que faz a fase escorregar pelas horas do dia.
	 */
	constexpr float SynodicMonthDays = 8.3f;

	/** A volta do nó. É ela que faz o eclipse ser raro. */
	constexpr float DraconicMonthDays = 7.6528f;

	/**
	 * Onde a lua está no primeiro dia: cheia, e longe de qualquer nó.
	 *
	 * Cheia porque a primeira noite de quem abre o jogo não pode ser um céu
	 * preto e vazio — a estreia do recurso seria a ausência dele. Longe do nó
	 * porque um eclipse na primeira noite ensinaria que eclipse é rotina, que
	 * é o oposto do que ele deve significar.
	 *
	 * Com estes dois valores a primeira **temporada de eclipses** cai entre os
	 * dias 4 e 12 — um solar raspando, um lunar parcial, outro solar mais
	 * fundo — e depois vem um vão de quarenta e cinco dias sem nada. O eclipse
	 * total mesmo, de profundidade 0,97, só no dia 108. Nada disso está
	 * escrito numa tabela: é o que as duas voltas produzem sozinhas.
	 *
	 * Ao todo o céu passa cerca de **1% do tempo** em eclipse.
	 */
	constexpr float PhaseAtDayZero = 0.5f;
	constexpr float NodeHalfCycleAtDayZero = 0.80f;

	/**
	 * Quão perto do nó a lua precisa estar para haver eclipse, em fração da
	 * meia-volta do nó.
	 *
	 * Estreito de propósito: alargar isto até o eclipse ser comum tira dele
	 * exatamente o que o torna um acontecimento.
	 */
	constexpr float EclipseNodeWindow = 0.08f;

	/** Quão perto de cheia (ou nova) a lua precisa estar, em fração do mês. */
	constexpr float EclipsePhaseWindow = 0.035f;

	/**
	 * De quanto em quanto tempo um cometa volta, e por quantos dias ele fica.
	 *
	 * Quarenta e um dias, que não é múltiplo de nenhuma das duas voltas da
	 * lua: assim o cometa não chega sempre na mesma fase, e as duas coisas
	 * mais raras do céu podem coincidir uma vez na vida em vez de nunca.
	 *
	 * O dia exato da chegada sai da SEMENTE do mundo, não do calendário: o
	 * período diz de quanto em quanto tempo ele volta, e a semente diz em que
	 * ponto do período. Sem isso o cometa seria um horário de ônibus.
	 */
	constexpr float CometPeriodDays = 41.0f;
	constexpr float CometVisibleDays = 2.0f;

	/**
	 * A elevação do sol em que a primeira estrela aparece, e a em que todas
	 * já estão visíveis.
	 *
	 * Zero é o sol na linha do horizonte; -18 é o crepúsculo astronômico, o
	 * ponto abaixo do qual o céu não escurece mais. Sai da elevação do sol, e
	 * não de uma segunda tabela de horas, porque quem decide se é noite já é
	 * o relógio (L-032).
	 */
	constexpr float FirstStarSunElevationDegrees = 0.0f;
	constexpr float FullStarSunElevationDegrees = -18.0f;

	/** Quantos dias de jogo já correram. */
	BATTLESQUARE_API float ElapsedDays(float ElapsedHours);

	/**
	 * Onde a lua está na volta da fase: 0 é nova, 0,5 é cheia, 1 é nova outra vez.
	 *
	 * Este número é a ÚNICA fonte de tudo o que a lua faz. A fase com nome, o
	 * quanto do disco está aceso, a hora em que ela nasce e a possibilidade de
	 * eclipse saem todos dele por função.
	 */
	BATTLESQUARE_API float MoonPhaseFraction(float ElapsedHours);

	/** A fase com nome. */
	BATTLESQUARE_API EMoonPhase PhaseOf(float PhaseFraction);

	/** Quanto do disco está aceso, de 0 (nova) a 1 (cheia). */
	BATTLESQUARE_API float MoonLitFraction(float PhaseFraction);

	/**
	 * Quantas horas a lua atrasa em relação ao sol.
	 *
	 * É a fase vezes o dia: a lua nova nasce com o sol, a cheia nasce quando
	 * ele se põe. Não é ajuste — é o que "fase" significa.
	 */
	BATTLESQUARE_API float MoonRiseLagHours(float PhaseFraction);

	/**
	 * A altura da lua acima do horizonte, em graus.
	 *
	 * Usa a MESMA conta que sobe o sol, com o atraso da fase descontado da
	 * hora. Uma segunda conta para a lua daria dois céus com curvaturas
	 * diferentes, e o defeito apareceria como a lua cortando o horizonte na
	 * hora errada.
	 */
	BATTLESQUARE_API float MoonElevationDegrees(float Hour, float PhaseFraction);

	/** De que lado do céu a lua está, em graus. */
	BATTLESQUARE_API float MoonAzimuthDegrees(float Hour, float PhaseFraction);

	/** A rotação inteira da luz do luar nesta hora. */
	BATTLESQUARE_API FRotator MoonRotation(float Hour, float PhaseFraction);

	/** A lua está acima do horizonte? */
	BATTLESQUARE_API bool IsMoonUp(float Hour, float PhaseFraction);

	/**
	 * Quanto o luar ilumina, de 0 a 1.
	 *
	 * O disco aceso vezes a altura dela no céu: lua nova não ilumina nada, e
	 * lua cheia a pino ilumina o máximo. É por isso que uma noite de lua
	 * cheia se enxerga e uma de lua nova não — sem nenhum valor por fase.
	 */
	BATTLESQUARE_API float MoonBrightness(float Hour, float PhaseFraction);

	/** Quão perto do nó a lua está: 1 exatamente no nó, 0 fora da janela. */
	BATTLESQUARE_API float NodeProximity(float ElapsedHours);

	/**
	 * Que eclipse está acontecendo agora, se algum.
	 *
	 * Cada um exige no céu o corpo que ele apaga: o lunar quer a lua no céu,
	 * o solar quer o sol. São dois testes porque são dois fenômenos — o solar
	 * é de dia por definição, e não por consequência de a lua nova subir junto
	 * com o sol.
	 */
	BATTLESQUARE_API ESkyEclipse EclipseAt(float Hour, float ElapsedHours);

	/**
	 * Quão fundo o eclipse está, de 0 a 1.
	 *
	 * Existe para o eclipse ENTRAR e SAIR em vez de piscar: total só quando as
	 * duas voltas estão as duas no centro da janela. Um booleano faria a lua
	 * ficar vermelha de um quadro para o outro.
	 */
	BATTLESQUARE_API float EclipseDepth(float Hour, float ElapsedHours);

	/**
	 * Está havendo lua vermelha?
	 *
	 * É a mesma pergunta que "há eclipse lunar?", e é de propósito que ela seja
	 * a mesma: a lua fica vermelha PORQUE está eclipsada.
	 */
	BATTLESQUARE_API bool IsBloodMoon(float Hour, float ElapsedHours);

	/** A cor da lua agora: pérola sempre, sangue quando eclipsada. */
	BATTLESQUARE_API FLinearColor MoonColor(float Hour, float ElapsedHours);

	/**
	 * Quanto do sol a lua está tapando, de 0 a 1.
	 *
	 * Zero fora do eclipse solar. Quem acende a cena multiplica o brilho do
	 * sol por (1 menos isto) — e é assim que o meio-dia escurece sem um modo
	 * separado para eclipse.
	 */
	BATTLESQUARE_API float SolarEclipseCoverage(float Hour, float ElapsedHours);

	/**
	 * Quanto de escuridão há no céu, de 0 (sol no horizonte) a 1 (crepúsculo
	 * astronômico vencido).
	 *
	 * Estrela, aurora e a cor do luar bebem TODAS daqui, e nenhuma delas do
	 * brilho do sol: `SunBrightness` tem piso de 0,02 para a noite nunca ficar
	 * cega, e normalizar por um valor com piso faria a estrela mais fraca
	 * justamente na noite mais escura.
	 *
	 * É pública porque quem acende a cena precisa da MESMA rampa: com duas
	 * contas do que é "escuro", as estrelas apareceriam numa hora e a cor da
	 * noite mudaria noutra (L-032).
	 */
	BATTLESQUARE_API float SkyDarkness(float Hour);

	/**
	 * Quão visíveis as estrelas estão, de 0 a 1.
	 *
	 * A escuridão abre e a nuvem fecha. Céu encoberto de madrugada não tem
	 * estrela, e isso é o que faz o tempo limpo VALER alguma coisa de noite.
	 */
	BATTLESQUARE_API float StarBrightness(float Hour, EWeather Weather);

	/** Há cometa no céu neste dia? */
	BATTLESQUARE_API bool CometVisible(uint32 Seed, float ElapsedHours);

	/**
	 * De que lado do céu o cometa está, em graus, e a que altura.
	 *
	 * Fixos durante a passagem toda: cometa que muda de lugar a cada quadro é
	 * um bug com cauda. Quem decide é a semente com o número da passagem, e
	 * não a hora.
	 */
	BATTLESQUARE_API float CometAzimuthDegrees(uint32 Seed, float ElapsedHours);
	BATTLESQUARE_API float CometElevationDegrees(uint32 Seed, float ElapsedHours);

	/**
	 * A força da aurora boreal aqui, de 0 a 1.
	 *
	 * Só em clima FRIO — a aurora é a assinatura da geleira, e é ela que dá
	 * motivo para atravessar a ilha até lá de noite. Cresce com a escuridão,
	 * pela elevação do sol, como as estrelas.
	 *
	 * **Toda noite, e não algumas.** Sorteá-la por noite faria quem foi até a
	 * geleira para vê-la voltar de mãos vazias e concluir que ela não existe —
	 * o mesmo motivo pelo qual nenhum peso de encontro é zero.
	 */
	BATTLESQUARE_API float AuroraStrength(EScenaryClimate Climate, float Hour);

	/** Nomes para o painel. Desenvolvimento, não texto de jogador. */
	BATTLESQUARE_API const TCHAR* PhaseDebugName(EMoonPhase Phase);
	BATTLESQUARE_API const TCHAR* EclipseDebugName(ESkyEclipse Eclipse);
}
