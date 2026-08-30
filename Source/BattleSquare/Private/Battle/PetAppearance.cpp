// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetAppearance.h"

namespace
{
	/** Nomes de tipo como aparecem no catálogo de pets e em PetSkills.json. */
	const FString TipoFogo = TEXT("Fogo");
	const FString TipoAgua = TEXT("Agua");
	const FString TipoPlanta = TEXT("Planta");
	const FString TipoCachorro = TEXT("Dog");

	// Tipos acrescentados em 30/08/2026.
	const FString TipoPsiquico = TEXT("Psiquico");
	const FString TipoMagico = TEXT("Magico");
	const FString TipoInseto = TEXT("Inseto");
	const FString TipoCaverna = TEXT("Caverna");
}

FPetAppearance FPetAppearance::ForType(const FString& PetType)
{
	FPetAppearance Aparencia;

	// Cachorro é o ÚNICO tipo do espelho de fixture que não é gato, e até aqui
	// saía idêntico a um: o sistema de tipos existia sem nunca aparecer na
	// tela, que é o modo de falhar que este projeto já pagou três vezes.
	//
	// A orelha CAI em vez de apontar — é o que distingue os dois de longe, e é
	// o único par de tipos que os dados de hoje conseguem pôr lado a lado.
	if (PetType.Equals(TipoCachorro, ESearchCase::IgnoreCase))
	{
		Aparencia.AccentColor = FLinearColor(0.62f, 0.42f, 0.22f);
		Aparencia.CrestShape = EPetCrestShape::OrelhaCaida;
		Aparencia.CrestScale = FVector(0.13f, 0.13f, 0.34f);
		Aparencia.CrestRotation = FRotator(0.0f, 0.0f, -72.0f);
		return Aparencia;
	}

	if (PetType.Equals(TipoFogo, ESearchCase::IgnoreCase))
	{
		Aparencia.AccentColor = FLinearColor(0.95f, 0.35f, 0.05f);
		Aparencia.CrestShape = EPetCrestShape::Chama;
		Aparencia.CrestScale = FVector(0.18f, 0.18f, 0.46f);
		Aparencia.CrestRotation = FRotator(0.0f, 0.0f, -10.0f);
		return Aparencia;
	}

	if (PetType.Equals(TipoAgua, ESearchCase::IgnoreCase))
	{
		Aparencia.AccentColor = FLinearColor(0.10f, 0.70f, 0.90f);
		Aparencia.CrestShape = EPetCrestShape::Barbatana;
		// Achatada no eixo de frente: barbatana é lâmina, não espinho.
		Aparencia.CrestScale = FVector(0.06f, 0.30f, 0.30f);
		Aparencia.CrestRotation = FRotator(0.0f, 0.0f, -55.0f);
		return Aparencia;
	}

	if (PetType.Equals(TipoPlanta, ESearchCase::IgnoreCase))
	{
		Aparencia.AccentColor = FLinearColor(0.20f, 0.78f, 0.25f);
		Aparencia.CrestShape = EPetCrestShape::Folha;
		Aparencia.CrestScale = FVector(0.32f, 0.07f, 0.38f);
		Aparencia.CrestRotation = FRotator(0.0f, 0.0f, -34.0f);
		return Aparencia;
	}

	// --- OS QUATRO OCULTOS, e o inseto ---------------------------------
	//
	// A paleta agora tem DUAS famílias, e isso é informação, não arranjo: as
	// matizes quentes e verdes são do mundo natural; violeta, magenta e índigo
	// são do que não é natural. Antes de saber QUAL tipo é, o jogador já sabe
	// com que espécie de coisa está lidando.
	//
	// É a extensão de "cor tem dono": com três tipos a matiz bastava sozinha;
	// com sete, ela agrupa e a SILHUETA identifica.

	if (PetType.Equals(TipoInseto, ESearchCase::IgnoreCase))
	{
		// Chartreuse: natural, e o único ponto do verde-amarelo que não é nem
		// a Planta nem o ouro do pet sem tipo.
		Aparencia.AccentColor = FLinearColor(0.61f, 0.76f, 0.12f);
		Aparencia.CrestShape = EPetCrestShape::Antena;
		Aparencia.CrestScale = FVector(0.05f, 0.05f, 0.52f);
		Aparencia.CrestRotation = FRotator(0.0f, 0.0f, -18.0f);
		return Aparencia;
	}

	if (PetType.Equals(TipoPsiquico, ESearchCase::IgnoreCase))
	{
		Aparencia.AccentColor = FLinearColor(0.88f, 0.23f, 0.59f);
		Aparencia.CrestShape = EPetCrestShape::Orbe;
		// PEQUENO e redondo: o orbe é o oposto do chifre — não ameaça, paira.
		Aparencia.CrestScale = FVector(0.15f, 0.15f, 0.15f);
		Aparencia.CrestRotation = FRotator(0.0f, 0.0f, 0.0f);
		return Aparencia;
	}

	if (PetType.Equals(TipoMagico, ESearchCase::IgnoreCase))
	{
		Aparencia.AccentColor = FLinearColor(0.48f, 0.31f, 0.85f);
		Aparencia.CrestShape = EPetCrestShape::Ponta;
		// Apontando para BAIXO: nada na natureza cresce assim, e é justamente
		// isso que a forma precisa dizer.
		Aparencia.CrestScale = FVector(0.12f, 0.12f, 0.42f);
		Aparencia.CrestRotation = FRotator(0.0f, 0.0f, 168.0f);
		return Aparencia;
	}

	if (PetType.Equals(TipoCaverna, ESearchCase::IgnoreCase))
	{
		Aparencia.AccentColor = FLinearColor(0.29f, 0.36f, 0.77f);
		Aparencia.CrestShape = EPetCrestShape::Cristal;
		// Baixo e largo: peso, não alcance. É a silhueta que menos se parece
		// com todas as outras, que são altas e finas.
		Aparencia.CrestScale = FVector(0.20f, 0.20f, 0.22f);
		Aparencia.CrestRotation = FRotator(0.0f, 0.0f, -22.0f);
		return Aparencia;
	}

	return Aparencia;
}
