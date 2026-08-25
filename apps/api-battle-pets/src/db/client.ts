// Copyright 2026 Anderson. All Rights Reserved.

import { drizzle } from 'drizzle-orm/postgres-js';
import postgres from 'postgres';

import { environment } from '../config/environment';

const queryClient = postgres(environment.DATABASE_URL);
export const db = drizzle(queryClient);
