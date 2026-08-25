// Copyright 2026 Anderson. All Rights Reserved.

import { existsSync } from 'node:fs';
import { platform } from 'node:os';
import { tmpdir } from 'node:os';

// AD-018: o temporário em texto plano precisa ficar em RAM, não em disco.
// Verificado por observação (não presumido): /dev/shm é tmpfs real no
// Linux, existe por padrão. macOS NÃO tem tmpfs padrão — /tmp ali é
// disco comum (APFS). Falha ALTO nesse caso, em vez de fingir a mesma
// garantia — é a mesma lição de L-004/L-007 aplicada aqui.

let hasWarnedAboutMissingRamDisk = false;

export function resolveTempDirectory(): { path: string; isRamBacked: boolean } {
  if (platform() === 'linux' && existsSync('/dev/shm')) {
    return { path: '/dev/shm', isRamBacked: true };
  }

  if (!hasWarnedAboutMissingRamDisk) {
    console.warn(
      '[mirror-tempdir] AVISO: nenhum diretório RAM-backed detectado nesta plataforma ' +
        `(${platform()}). O temporário em texto plano do espelho local vai usar ${tmpdir()}, ` +
        'que é disco comum, NÃO memória. A mitigação do AD-018 fica reduzida a ' +
        '"apagado rapidamente", não "nunca tocou disco". Aceitável em dev; produção deve ' +
        'rodar em Linux com /dev/shm disponível.',
    );
    hasWarnedAboutMissingRamDisk = true;
  }

  return { path: tmpdir(), isRamBacked: false };
}
