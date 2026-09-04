// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/TheftRules.h"

TheftRules::ETheftOption TheftRules::OptionFor(
	PetOwnershipQuery::EOwnerRelation Relacao, bool bTreinadorVulneravel)
{
	// Só o pet de OUTRO se rouba. Selvagem e o próprio não são roubo — e
	// tratar os dois como "nothing" (não "protected") é a verdade: não há
	// vulnerabilidade que faça você roubar o que já é seu.
	if (Relacao != PetOwnershipQuery::EOwnerRelation::OtherAccount)
	{
		return ETheftOption::Nothing;
	}

	// É de outro, mas o treinador tem de estar vulnerável (decisão 22). Sem
	// isso, vencer rende prêmio e ranking, nunca o pet.
	return bTreinadorVulneravel ? ETheftOption::CanSteal : ETheftOption::Protected;
}

bool TheftRules::TransfersOwnership(
	PetOwnershipQuery::EOwnerRelation Relacao,
	bool bTreinadorVulneravel, bool bEscolheuRoubar)
{
	// As DUAS condições, e é o ponto: pode roubar E escolheu. Sem a escolha, a
	// posse não muda mesmo com a vitória — o vencedor decide, o roubo não é
	// automático.
	return OptionFor(Relacao, bTreinadorVulneravel) == ETheftOption::CanSteal
		&& bEscolheuRoubar;
}
