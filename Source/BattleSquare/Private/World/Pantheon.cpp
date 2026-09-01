// Copyright 2026 Anderson. All Rights Reserved.

#include "World/Pantheon.h"

int32 Pantheon::Count()
{
	return static_cast<int32>(EDeity::Abismo) + 1;
}

const TCHAR* Pantheon::DebugName(EDeity Which)
{
	switch (Which)
	{
	case EDeity::MaeNatureza: return TEXT("mae-natureza");
	case EDeity::Pedra:       return TEXT("pedra");
	case EDeity::Corrente:    return TEXT("corrente");
	case EDeity::Braseiro:    return TEXT("braseiro");
	case EDeity::Abismo:      return TEXT("abismo");
	}

	return TEXT("?");
}
