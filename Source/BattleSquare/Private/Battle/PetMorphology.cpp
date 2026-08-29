// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetMorphology.h"

#include "Battle/PetAppearance.h"

namespace
{
	/** As primitivas da engine têm 100uu de lado ou diâmetro. */
	constexpr float UnidadesDaPrimitiva = 100.0f;

	/**
	 * Onde a pata encosta no corpo, como FRAÇÃO do semieixo — nunca em
	 * unidades fixas.
	 *
	 * Com corpo de tamanho variável, um afastamento fixo sai para fora da
	 * elipse assim que o bicho estreita, e aí a conta da superfície de baixo
	 * não tem raiz real: a pata iria parar no centro do corpo. Em fração, a
	 * soma normalizada é sempre a mesma (0,48² + 0,50² = 0,48) e nunca chega
	 * a 1, seja qual for o porte ou a idade.
	 */
	constexpr float FracaoDaPataX = 0.48f;
	constexpr float FracaoDaPataY = 0.50f;

	/** Quanto a pata entra no corpo, para não haver fresta na junta. */
	constexpr float EncaixeDaPataNoCorpo = 4.0f;

	/** Folga mínima entre o ponto mais baixo da barriga e o chão. */
	constexpr float MargemDaBarriga = 4.0f;

	/** Perna de referência: o comprimento do bicho adulto neutro de hoje. */
	constexpr float ReferenciaDaPerna = 30.0f;

	/** Folga entre a parte mais alta do bicho e a barra de vida. */
	constexpr float FolgaDaBarra = 16.0f;

	/**
	 * Cada traço tem seu PRÓPRIO fluxo de bits, tirado da mesma semente.
	 *
	 * Fatiar um hash de 32 bits daria oito traços e acabou; pior, mexer na
	 * faixa de um traço mudaria os bits de todos os seguintes, e um ajuste de
	 * perna redesenharia a cara de todos os bichos do jogo. Com um fluxo por
	 * índice, traço novo entra no fim sem tocar em quem já existe.
	 */
	enum class ETraco : int32
	{
		Robustez = 0,
		Alongamento = 1,
		Pescoco = 2,
		Cauda = 3,
		Adorno = 4,
		Tom = 5
	};

	/** FNV-1a: hash puro, sem estado, sem relógio, igual em toda máquina. */
	uint32 SemearPor(const FString& Semente)
	{
		uint32 Hash = 2166136261u;
		for (const TCHAR Caractere : Semente)
		{
			Hash ^= static_cast<uint32>(Caractere);
			Hash *= 16777619u;
		}
		return Hash;
	}

	/** Finalizador do murmur3: espalha os bits para o fluxo não ter padrão. */
	uint32 Espalhar(uint32 Valor)
	{
		Valor ^= Valor >> 16;
		Valor *= 0x85EBCA6Bu;
		Valor ^= Valor >> 13;
		Valor *= 0xC2B2AE35u;
		Valor ^= Valor >> 16;
		return Valor;
	}

	/** O traço, entre 0 e 1. */
	float Traco(uint32 Semente, ETraco Qual)
	{
		const uint32 Fluxo = Espalhar(Semente ^ (static_cast<uint32>(Qual) * 0x9E3779B9u));
		return static_cast<float>(Fluxo >> 8) / static_cast<float>(1u << 24);
	}

	float Entre(float Minimo, float Maximo, float Fracao)
	{
		return Minimo + (Maximo - Minimo) * Fracao;
	}

	/**
	 * Quanto o bicho já cresceu, de 0 a 1.
	 *
	 * O adulto não fica no meio: ele é o corpo de referência, e o Evoluido é
	 * um passo mais curto do que o que separa filhote de adulto. Crescer da
	 * infância à vida adulta muda mais a forma do que evoluir muda.
	 */
	float Crescimento(EPetGrowthStage Fase)
	{
		switch (Fase)
		{
			case EPetGrowthStage::Filhote: return 0.0f;
			case EPetGrowthStage::Evoluido: return 1.0f;
			default: return 0.58f;
		}
	}
}

float FPetMorphology::BodySemiAxisXUnits() const
{
	return BodyScale.X * UnidadesDaPrimitiva * 0.5f;
}

float FPetMorphology::BodySemiAxisYUnits() const
{
	return BodyScale.Y * UnidadesDaPrimitiva * 0.5f;
}

float FPetMorphology::BodySemiAxisZUnits() const
{
	return BodyScale.Z * UnidadesDaPrimitiva * 0.5f;
}

float FPetMorphology::HeadRadiusUnits() const
{
	return HeadScale * UnidadesDaPrimitiva * 0.5f;
}

float FPetMorphology::BodyUnderSurfaceAtLegUnits() const
{
	const float Normalizado =
		FMath::Square(LegSpreadXUnits / BodySemiAxisXUnits())
		+ FMath::Square(LegSpreadYUnits / BodySemiAxisYUnits());

	const float Queda = BodySemiAxisZUnits() * FMath::Sqrt(FMath::Max(0.0f, 1.0f - Normalizado));
	return BodyCenterUnits - Queda;
}

float FPetMorphology::BodyLowestPointUnits() const
{
	return BodyCenterUnits - BodySemiAxisZUnits();
}

float FPetMorphology::LegHeightUnits() const
{
	return BodyUnderSurfaceAtLegUnits() + EncaixeDaPataNoCorpo;
}

float FPetMorphology::CrownUnits() const
{
	const float TopoDoTronco = BodyCenterUnits + BodySemiAxisZUnits();

	// Teto generoso de propósito: a base do adorno encosta na cabeça e ele
	// sobe no máximo o próprio comprimento. Errar para cima aqui só afasta a
	// barra; errar para baixo a enfia na cabeça do bicho.
	const float TopoDaCabeca = HeadCenterUnits + HeadRadiusUnits()
		+ CrestScale.Z * UnidadesDaPrimitiva;

	return FMath::Max(TopoDoTronco, TopoDaCabeca) + FolgaDaBarra;
}

FPetMorphology FPetMorphology::FromSeed(
	const FString& Seed,
	const FPetAppearance& Appearance,
	EPetGrowthStage Stage)
{
	FPetMorphology Corpo;

	// A forma e o tombo do adorno começam nos do tipo: o gerador MODULA o que
	// o tipo pediu, não substitui. Tipo é informação; porte é decoração.
	Corpo.CrestScale = Appearance.CrestScale;
	Corpo.CrestRotation = Appearance.CrestRotation;
	Corpo.AccentColor = Appearance.AccentColor;

	if (Seed.IsEmpty())
	{
		return Corpo;
	}

	const uint32 Semente = SemearPor(Seed);
	const float Idade = Crescimento(Stage);

	// Os dois eixos latentes. Todo o resto sai destes dois mais um ajuste
	// próprio — é isso que faz o bicho ter um porte em vez de ter medidas.
	const float Robustez = Traco(Semente, ETraco::Robustez);
	const float Alongamento = Traco(Semente, ETraco::Alongamento);

	// A idade é um MULTIPLICADOR sobre o mesmo plano, nunca um plano à parte:
	// os traços são os mesmos em toda fase, e por isso o filhote e o adulto
	// são reconhecivelmente o mesmo bicho.
	const float TamanhoGeral = Entre(0.60f, 1.16f, Idade);

	// Tronco. Comprido é também mais BAIXO: é o que separa a doninha do
	// cavalo esticado, e é o que impede o corpo comprido de virar tijolo.
	Corpo.BodyScale = FVector(
		Entre(0.56f, 0.92f, Alongamento),
		Entre(0.40f, 0.64f, Robustez),
		Entre(0.40f, 0.60f, Robustez) * Entre(1.0f, 0.82f, Alongamento)) * TamanhoGeral;

	const float SemiX = Corpo.BodySemiAxisXUnits();
	const float SemiY = Corpo.BodySemiAxisYUnits();
	const float SemiZ = Corpo.BodySemiAxisZUnits();

	Corpo.LegSpreadXUnits = SemiX * FracaoDaPataX;
	Corpo.LegSpreadYUnits = SemiY * FracaoDaPataY;

	// Perna: curta no atarracado, curta no comprido, longa no esguio — e
	// curta no filhote, que é metade do que faz um filhote parecer filhote.
	// Grossa acompanha o porte: perna fina em bicho pesado não sustenta o
	// que o olho vê.
	Corpo.LegThicknessScale = Entre(0.085f, 0.150f, Robustez);

	const float Queda = SemiZ * FMath::Sqrt(FMath::Max(0.0f,
		1.0f - FMath::Square(FracaoDaPataX) - FMath::Square(FracaoDaPataY)));

	const float PernaDesejada = ReferenciaDaPerna * TamanhoGeral
		* Entre(1.0f, 0.45f, Robustez)
		* Entre(1.0f, 0.70f, Alongamento)
		* Entre(0.70f, 1.10f, Idade);

	// O PISO da perna sai da barriga, não de um número escolhido: o meio do
	// tronco pende mais que a beirada onde a pata encosta, e é ele que raspa
	// o chão. Sem este limite, o bicho comprido e baixo arrasta.
	const float PernaMinima = (SemiZ - Queda) + MargemDaBarriga;

	Corpo.LegClearanceUnits = FMath::Max(PernaDesejada, PernaMinima);
	Corpo.BodyCenterUnits = Corpo.LegClearanceUnits + Queda;

	// Cabeça: maior no atarracado, MENOR quando o pescoço é longo, e bem
	// maior no filhote. Cabeça grande é o sinal de "novo" mais forte que
	// existe, e é por isso que o filhote não é o adulto encolhido.
	const float Pescoco = Traco(Semente, ETraco::Pescoco);
	const float CabecaPedida = Entre(0.28f, 0.46f, Robustez)
		* Entre(1.06f, 0.86f, Pescoco)
		* Entre(1.50f, 0.88f, Idade)
		* TamanhoGeral;

	// E nunca maior que o tronco que a carrega — senão vira boneco de mola.
	// O teto AFROUXA na infância porque no filhote a cabeça do tamanho do
	// tronco está certa: apertá-la pelo limite do adulto devolveria um adulto
	// pequeno, que é justamente o que esta fase não é.
	const float RaioMaximo = FMath::Min(SemiY, SemiZ) * Entre(1.40f, 0.95f, Idade);
	Corpo.HeadScale = FMath::Min(CabecaPedida, RaioMaximo * 2.0f / UnidadesDaPrimitiva);

	// A cabeça é posta SOBRE a superfície do tronco, achada pela direção —
	// não num (x,z) escolhido. Assim ela encosta em qualquer porte e em
	// qualquer idade, e o pescoço longo a afasta sem nunca soltá-la do corpo.
	const float AnguloDoPescoco = FMath::DegreesToRadians(
		Entre(14.0f, 52.0f, Pescoco) * Entre(0.75f, 1.10f, Idade));
	const float Cosseno = FMath::Cos(AnguloDoPescoco);
	const float Seno = FMath::Sin(AnguloDoPescoco);
	const float AteASuperficie = 1.0f / FMath::Sqrt(
		FMath::Square(Cosseno / SemiX) + FMath::Square(Seno / SemiZ));

	// No máximo 0,85 do raio para fora: a face de trás da cabeça continua
	// dentro do tronco, e é por construção, não por sorte.
	const float Afastamento = AteASuperficie
		+ Corpo.HeadRadiusUnits() * Entre(0.28f, 0.85f, Pescoco) * Entre(0.68f, 1.0f, Idade);
	Corpo.HeadForwardUnits = Cosseno * Afastamento;
	Corpo.HeadCenterUnits = Corpo.BodyCenterUnits + Seno * Afastamento;

	// Focinho preso ao raio da cabeça, nunca a um deslocamento fixo: foi
	// offset fixo contra superfície curva que enterrou peça no bicho três
	// vezes neste projeto. E ele é curto no filhote — a outra metade do que
	// faz um filhote parecer filhote.
	const float RaioDaCabeca = Corpo.HeadRadiusUnits();
	Corpo.SnoutScale = Corpo.HeadScale * Entre(0.28f, 0.45f, Idade);
	Corpo.SnoutLocation = FVector(
		RaioDaCabeca * Entre(0.86f, 1.02f, Idade), 0.0f, -RaioDaCabeca * 0.22f);

	// Cauda: mais longa em bicho comprido, que é o que equilibra a silhueta.
	const float Cauda = Traco(Semente, ETraco::Cauda);
	const float Grossura = Entre(0.11f, 0.22f, Robustez) * TamanhoGeral;
	Corpo.TailScale = FVector(Grossura, Grossura,
		Entre(0.28f, 0.62f, Cauda) * Entre(0.90f, 1.25f, Alongamento)
			* Entre(0.68f, 1.16f, Idade) * TamanhoGeral);
	Corpo.TailLocation = FVector(-SemiX * 1.03f, 0.0f, Corpo.BodyCenterUnits + SemiZ * 0.34f);
	Corpo.TailRotation = FRotator(Entre(14.0f, 46.0f, Cauda), 0.0f, 0.0f);

	// Adorno: cresce com a idade e com o sorteio, mas nunca passa da cabeça
	// que o sustenta. O teto é aplicado nos TRÊS eixos de uma vez — apertar
	// só a altura acharia a folha do tipo Planta e devolveria outra coisa no
	// lugar dela.
	const float Adorno = Traco(Semente, ETraco::Adorno);
	const float Aumento = Entre(0.75f, 1.45f, Adorno) * Entre(0.42f, 1.28f, Idade);
	const float MaiorEixo = Appearance.CrestScale.GetAbsMax() * Aumento * UnidadesDaPrimitiva;
	const float TetoDoAdorno = RaioDaCabeca * 2.2f;
	const float Aperto = (MaiorEixo > TetoDoAdorno && MaiorEixo > KINDA_SMALL_NUMBER)
		? TetoDoAdorno / MaiorEixo : 1.0f;
	Corpo.CrestScale = Appearance.CrestScale * (Aumento * Aperto);
	Corpo.CrestRotation.Roll = Appearance.CrestRotation.Roll + Entre(-14.0f, 14.0f, Adorno);

	// Tom: mais claro ou mais escuro DENTRO da família do tipo, e puxado para
	// o pálido no filhote, para o carregado no evoluído. Ambas as pontas
	// mexem nos três canais na mesma proporção, então a ordem entre eles não
	// muda — um pet de Água continua lendo como azul em toda idade.
	const float Tom = Traco(Semente, ETraco::Tom);
	const float TomComIdade = FMath::Lerp(Tom, Entre(0.88f, 0.08f, Idade), 0.55f);
	const FLinearColor Escuro = Appearance.AccentColor * 0.70f;
	const FLinearColor Claro = Appearance.AccentColor
		+ (FLinearColor::White - Appearance.AccentColor) * 0.38f;
	Corpo.AccentColor = FMath::Lerp(Escuro, Claro, TomComIdade);
	Corpo.AccentColor.A = 1.0f;

	return Corpo;
}
