// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/PetTypeIdentity.h"

namespace
{
	const FString EscolaFisica = TEXT("Fisica");
	const FString EscolaNatural = TEXT("Natural");
	const FString EscolaPsiquica = TEXT("Psiquica");

	const FString ElementoFogo = TEXT("Fogo");
	const FString ElementoAgua = TEXT("Agua");
	const FString ElementoPlanta = TEXT("Planta");
	const FString ElementoTerra = TEXT("Terra");

	/**
	 * Os nomes de um eixo só que já foram assinados, e o par que cada um vira.
	 *
	 * Não é conveniência: é compatibilidade com dado que não se pode reescrever.
	 * Um pet assinado como "Magico" continua sendo o mesmo pet — o que muda é
	 * o jogo passar a saber que ele é psíquico de fogo.
	 */
	bool TraduzirNomeAntigo(const FString& Nome, FPetTypeIdentity& Fora)
	{
		struct FLegado { const TCHAR* Antigo; const TCHAR* Escola; const TCHAR* Elemento; };
		static const FLegado Tabela[] = {
			{ TEXT("Fogo"),     TEXT("Natural"),  TEXT("Fogo")   },
			{ TEXT("Agua"),     TEXT("Natural"),  TEXT("Agua")   },
			{ TEXT("Planta"),   TEXT("Natural"),  TEXT("Planta") },
			// Inseto é corpo, e o corpo dele é do mundo da folha.
			{ TEXT("Inseto"),   TEXT("Fisica"),   TEXT("Planta") },
			// Caverna é a matéria em estado puro.
			{ TEXT("Caverna"),  TEXT("Fisica"),   TEXT("Terra")  },
			{ TEXT("Psiquico"), TEXT("Psiquica"), TEXT("Agua")   },
			{ TEXT("Magico"),   TEXT("Psiquica"), TEXT("Fogo")   },
		};

		for (const FLegado& Entrada : Tabela)
		{
			if (Nome.Equals(Entrada.Antigo, ESearchCase::IgnoreCase))
			{
				Fora.School = Entrada.Escola;
				Fora.Element = Entrada.Elemento;
				return true;
			}
		}
		return false;
	}

	/** Casa sem diferenciar caixa e devolve a grafia CANÔNICA. */
	bool Reconhecer(const TArray<FString>& Conhecidos, const FString& Bruto, FString& Fora)
	{
		for (const FString& Conhecido : Conhecidos)
		{
			if (Bruto.Equals(Conhecido, ESearchCase::IgnoreCase))
			{
				Fora = Conhecido;
				return true;
			}
		}
		return false;
	}
}

const TArray<FString>& FPetTypeIdentity::AllSchools()
{
	static const TArray<FString> Escolas = { EscolaFisica, EscolaNatural, EscolaPsiquica };
	return Escolas;
}

const TArray<FString>& FPetTypeIdentity::AllElements()
{
	static const TArray<FString> Elementos = { ElementoFogo, ElementoAgua, ElementoPlanta, ElementoTerra };
	return Elementos;
}

FString FPetTypeIdentity::ToTypeString() const
{
	return IsValid() ? FString::Printf(TEXT("%s/%s"), *School, *Element) : FString();
}

FPetTypeIdentity FPetTypeIdentity::Parse(const FString& PetType)
{
	FPetTypeIdentity Identidade;

	FString Esquerda;
	FString Direita;
	if (PetType.Split(TEXT("/"), &Esquerda, &Direita))
	{
		// Reconhecer os dois lados SEPARADAMENTE, e não aceitar o par inteiro
		// por confiança: "Natural/Telepatia" precisa falhar no elemento em vez
		// de virar um tipo que existe e não bate com nada.
		Reconhecer(AllSchools(), Esquerda.TrimStartAndEnd(), Identidade.School);
		Reconhecer(AllElements(), Direita.TrimStartAndEnd(), Identidade.Element);
		return Identidade;
	}

	TraduzirNomeAntigo(PetType.TrimStartAndEnd(), Identidade);
	return Identidade;
}
