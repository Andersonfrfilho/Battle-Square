// Copyright 2026 Anderson. All Rights Reserved.

import { environment } from './config/environment';
import { startSyncLoop } from './mirror/sync-loop';

console.log(`worker-pet-sync iniciando — alvo: ${environment.API_BATTLE_PETS_URL}, intervalo: ${environment.SYNC_INTERVAL_SECONDS}s`);
startSyncLoop();
