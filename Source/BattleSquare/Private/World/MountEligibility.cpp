// Copyright 2026 Anderson. All Rights Reserved.

#include "World/MountEligibility.h"

#define LOCTEXT_NAMESPACE "MountEligibility"

bool MountEligibility::CanMount(bool bRecordSaysMountable)
{
	return bRecordSaysMountable;
}

FText MountEligibility::RefusalReason()
{
	return LOCTEXT("NaoMontavel", "este pet não pode ser montado");
}

#undef LOCTEXT_NAMESPACE
